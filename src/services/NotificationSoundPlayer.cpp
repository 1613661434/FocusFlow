#include "services/NotificationSoundPlayer.h"

#include <QApplication>
#include <QAudioOutput>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QUrl>

NotificationSoundPlayer::NotificationSoundPlayer(QObject *parent)
    : QObject(parent),
      player_(new QMediaPlayer(this)),
      audioOutput_(new QAudioOutput(this))
{
    player_->setAudioOutput(audioOutput_);
    limitTimer_.setSingleShot(true);
    repeatDelayTimer_.setSingleShot(true);
    fadeAnimation_.setDuration(350);
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
    stopImmediately();
    targetVolume_ = qBound(0, volumePercent, 100) / 100.0;
    audioOutput_->setVolume(targetVolume_);
    maximumMilliseconds_ = qBound(1, maximumSeconds, 60) * 1000;
    remainingRounds_ = qBound(1, repeatCount, 5);
    filePath_ = filePath;

    if (filePath_.isEmpty() || !QFileInfo::exists(filePath_)) {
        playFallbackBeep();
        return;
    }

    player_->setSource(QUrl::fromLocalFile(filePath_));
    startRound();
}

void NotificationSoundPlayer::stop()
{
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
    limitTimer_.start(qMax(1, maximumMilliseconds_ - fadeAnimation_.duration()));
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
