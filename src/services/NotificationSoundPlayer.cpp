#include "services/NotificationSoundPlayer.h"

#include <QApplication>
#include <QAudioOutput>
#include <QBuffer>
#include <QDataStream>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QUrl>
#include <QVector>

#include <cmath>

namespace {
struct ToneSegment {
    double frequency;
    int milliseconds;
};

QByteArray builtInWave(const QString &soundId)
{
    QVector<ToneSegment> segments;
    if (soundId == QStringLiteral("builtin://bright")) {
        segments = {{1046.5, 110}, {0.0, 45}, {1318.5, 190}};
    } else if (soundId == QStringLiteral("builtin://gentle")) {
        segments = {{523.3, 170}, {659.3, 170}, {784.0, 280}};
    } else if (soundId == QStringLiteral("builtin://concise")) {
        segments = {{740.0, 260}};
    } else {
        segments = {{880.0, 130}, {0.0, 55}, {659.3, 210}};
    }

    constexpr int sampleRate = 44100;
    constexpr double pi = 3.14159265358979323846;
    QByteArray pcm;
    QDataStream pcmStream(&pcm, QIODevice::WriteOnly);
    pcmStream.setByteOrder(QDataStream::LittleEndian);
    for (const ToneSegment &segment : segments) {
        const int sampleCount = sampleRate * segment.milliseconds / 1000;
        const int attackSamples = qMax(1, sampleRate * 12 / 1000);
        const int releaseSamples = qMax(1, sampleRate * 90 / 1000);
        for (int index = 0; index < sampleCount; ++index) {
            const double attack = qMin(1.0,
                static_cast<double>(index) / attackSamples);
            const double release = qMin(1.0,
                static_cast<double>(sampleCount - index - 1)
                    / releaseSamples);
            const double envelope = qMax(0.0, qMin(attack, release));
            const double wave = segment.frequency <= 0.0
                ? 0.0
                : std::sin(2.0 * pi * segment.frequency * index
                           / sampleRate);
            pcmStream << static_cast<qint16>(wave * envelope * 12000.0);
        }
    }

    QByteArray wave;
    QDataStream stream(&wave, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData("RIFF", 4);
    stream << static_cast<quint32>(36 + pcm.size());
    stream.writeRawData("WAVE", 4);
    stream.writeRawData("fmt ", 4);
    stream << static_cast<quint32>(16);
    stream << static_cast<quint16>(1);
    stream << static_cast<quint16>(1);
    stream << static_cast<quint32>(sampleRate);
    stream << static_cast<quint32>(sampleRate * 2);
    stream << static_cast<quint16>(2);
    stream << static_cast<quint16>(16);
    stream.writeRawData("data", 4);
    stream << static_cast<quint32>(pcm.size());
    stream.writeRawData(pcm.constData(), pcm.size());
    return wave;
}
}

NotificationSoundPlayer::NotificationSoundPlayer(QObject *parent)
    : QObject(parent),
      player_(new QMediaPlayer(this)),
      audioOutput_(new QAudioOutput(this)),
      builtInBuffer_(new QBuffer(this))
{
    player_->setAudioOutput(audioOutput_);
    limitTimer_.setSingleShot(true);
    repeatDelayTimer_.setSingleShot(true);
    fadeAnimation_.setDuration(1800);
    fadeAnimation_.setEasingCurve(QEasingCurve::InOutQuad);
    connect(&limitTimer_, &QTimer::timeout,
            this, &NotificationSoundPlayer::finishRound);
    connect(&repeatDelayTimer_, &QTimer::timeout,
            this, &NotificationSoundPlayer::startRound);
    connect(&fadeAnimation_, &QVariantAnimation::valueChanged,
            this, [this](const QVariant &value) {
                audioOutput_->setVolume(value.toReal());
            });
    connect(&fadeAnimation_, &QVariantAnimation::finished,
            this, &NotificationSoundPlayer::completeFadeOut);
    connect(player_, &QMediaPlayer::mediaStatusChanged,
            this, &NotificationSoundPlayer::handleMediaStatusChanged);
    connect(player_, &QMediaPlayer::errorOccurred,
            this, &NotificationSoundPlayer::handlePlaybackError);
}

void NotificationSoundPlayer::play(const QString &filePath,
                                   int volumePercent,
                                   int maximumSeconds,
                                   int repeatCount)
{
    const QString requestedPath = filePath.isEmpty()
        ? QStringLiteral("builtin://system") : filePath;
    if (player_->playbackState() == QMediaPlayer::PlayingState
        || fadeAnimation_.state() == QAbstractAnimation::Running) {
        pendingFilePath_ = requestedPath;
        pendingVolumePercent_ = volumePercent;
        pendingMaximumSeconds_ = maximumSeconds;
        pendingRepeatCount_ = repeatCount;
        hasPendingPlayback_ = true;
        remainingRounds_ = 0;
        limitTimer_.stop();
        repeatDelayTimer_.stop();
        continueAfterFade_ = false;
        if (fadeAnimation_.state() != QAbstractAnimation::Running) {
            beginFadeOut(false);
        }
        return;
    }
    configurePlayback(requestedPath, volumePercent,
                      maximumSeconds, repeatCount);
}

void NotificationSoundPlayer::configurePlayback(const QString &filePath,
                                                int volumePercent,
                                                int maximumSeconds,
                                                int repeatCount)
{
    stopImmediately();
    targetVolume_ = qBound(0, volumePercent, 100) / 100.0;
    audioOutput_->setVolume(targetVolume_);
    maximumMilliseconds_ = maximumSeconds <= 0
        ? 0 : qBound(1, maximumSeconds, 60) * 1000;
    remainingRounds_ = qBound(1, repeatCount, 5);
    filePath_ = filePath;

    if (filePath_.startsWith(QStringLiteral("builtin://"))) {
        builtInBuffer_->close();
        builtInBuffer_->setData(builtInWave(filePath_));
        builtInBuffer_->open(QIODevice::ReadOnly);
        player_->setSourceDevice(builtInBuffer_,
                                 QUrl(QStringLiteral("memory-sound.wav")));
    } else if (QFileInfo::exists(filePath_)) {
        player_->setSource(QUrl::fromLocalFile(filePath_));
    } else {
        filePath_ = QStringLiteral("builtin://system");
        builtInBuffer_->close();
        builtInBuffer_->setData(builtInWave(filePath_));
        builtInBuffer_->open(QIODevice::ReadOnly);
        player_->setSourceDevice(builtInBuffer_,
                                 QUrl(QStringLiteral("memory-sound.wav")));
    }
    startRound();
}

void NotificationSoundPlayer::stop()
{
    hasPendingPlayback_ = false;
    limitTimer_.stop();
    repeatDelayTimer_.stop();
    remainingRounds_ = 0;
    if (player_->playbackState() == QMediaPlayer::PlayingState) {
        beginFadeOut(false);
    } else {
        stopImmediately();
    }
}

void NotificationSoundPlayer::startRound()
{
    if (remainingRounds_ <= 0) {
        return;
    }
    repeatDelayTimer_.stop();
    fadeAnimation_.stop();
    finishingRound_ = false;
    audioOutput_->setVolume(targetVolume_);
    player_->setPosition(0);
    player_->play();
    if (maximumMilliseconds_ > 0) {
        const int fadeDuration = qBound(
            450, maximumMilliseconds_ / 3, 1800);
        fadeAnimation_.setDuration(fadeDuration);
        limitTimer_.start(qMax(1, maximumMilliseconds_ - fadeDuration));
    } else {
        limitTimer_.stop();
        fadeAnimation_.setDuration(1800);
    }
}

void NotificationSoundPlayer::finishRound()
{
    if (finishingRound_ || remainingRounds_ <= 0) {
        return;
    }
    beginFadeOut(true);
}

void NotificationSoundPlayer::handleMediaStatusChanged()
{
    if (player_->mediaStatus() == QMediaPlayer::EndOfMedia) {
        if (!finishingRound_ && remainingRounds_ > 0) {
            limitTimer_.stop();
            --remainingRounds_;
            if (remainingRounds_ > 0) {
                repeatDelayTimer_.start(300);
            }
        }
    }
}

void NotificationSoundPlayer::stopImmediately()
{
    limitTimer_.stop();
    repeatDelayTimer_.stop();
    fadeAnimation_.stop();
    finishingRound_ = true;
    continueAfterFade_ = false;
    player_->stop();
    finishingRound_ = false;
}

void NotificationSoundPlayer::beginFadeOut(bool continueRounds)
{
    if (finishingRound_) {
        return;
    }
    finishingRound_ = true;
    continueAfterFade_ = continueRounds;
    limitTimer_.stop();
    fadeAnimation_.stop();
    fadeAnimation_.setStartValue(audioOutput_->volume());
    fadeAnimation_.setEndValue(0.0);
    fadeAnimation_.start();
}

void NotificationSoundPlayer::completeFadeOut()
{
    player_->stop();
    if (hasPendingPlayback_) {
        const QString path = pendingFilePath_;
        const int volume = pendingVolumePercent_;
        const int maximumSeconds = pendingMaximumSeconds_;
        const int repeatCount = pendingRepeatCount_;
        hasPendingPlayback_ = false;
        continueAfterFade_ = false;
        finishingRound_ = false;
        configurePlayback(path, volume, maximumSeconds, repeatCount);
        return;
    }
    if (continueAfterFade_ && remainingRounds_ > 0) {
        --remainingRounds_;
    }
    const bool hasNextRound = continueAfterFade_ && remainingRounds_ > 0;
    continueAfterFade_ = false;
    finishingRound_ = false;
    audioOutput_->setVolume(targetVolume_);
    if (hasNextRound) {
        repeatDelayTimer_.start(300);
    }
}

void NotificationSoundPlayer::handlePlaybackError()
{
    stop();
    playFallbackBeep();
}

void NotificationSoundPlayer::playFallbackBeep()
{
    const int rounds = qMax(1, remainingRounds_);
    remainingRounds_ = 0;
    for (int index = 0; index < rounds; ++index) {
        QTimer::singleShot(index * 500, qApp, [] { QApplication::beep(); });
    }
}
