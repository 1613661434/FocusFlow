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
    connect(&limitTimer_, &QTimer::timeout,
            this, &NotificationSoundPlayer::finishRound);
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
    stop();
    audioOutput_->setVolume(qBound(0, volumePercent, 100) / 100.0);
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
    remainingRounds_ = 0;
    finishingRound_ = true;
    player_->stop();
    finishingRound_ = false;
}

void NotificationSoundPlayer::startRound()
{
    if (remainingRounds_ <= 0) {
        return;
    }
    finishingRound_ = false;
    player_->setPosition(0);
    player_->play();
    limitTimer_.start(maximumMilliseconds_);
}

void NotificationSoundPlayer::finishRound()
{
    if (finishingRound_ || remainingRounds_ <= 0) {
        return;
    }
    finishingRound_ = true;
    limitTimer_.stop();
    player_->stop();
    --remainingRounds_;
    finishingRound_ = false;
    if (remainingRounds_ > 0) {
        QTimer::singleShot(300, this, &NotificationSoundPlayer::startRound);
    }
}

void NotificationSoundPlayer::handleMediaStatusChanged()
{
    if (player_->mediaStatus() == QMediaPlayer::EndOfMedia) {
        finishRound();
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
