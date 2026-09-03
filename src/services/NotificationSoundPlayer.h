#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantAnimation>

class QAudioOutput;
class QBuffer;
class QMediaPlayer;

class NotificationSoundPlayer final : public QObject
{
    Q_OBJECT

public:
    explicit NotificationSoundPlayer(QObject *parent = nullptr);

    void play(const QString &filePath,
              int volumePercent,
              int maximumSeconds,
              int repeatCount);
    void stop();

private slots:
    void handleMediaStatusChanged();
    void handlePlaybackError();
    void finishRound();
    void completeFadeOut();

private:
    void startRound();
    void configurePlayback(const QString &filePath,
                           int volumePercent,
                           int maximumSeconds,
                           int repeatCount);
    void stopImmediately();
    void beginFadeOut(bool continueRounds);
    void playFallbackBeep();

    QMediaPlayer *player_ = nullptr;
    QAudioOutput *audioOutput_ = nullptr;
    QBuffer *builtInBuffer_ = nullptr;
    QTimer limitTimer_;
    QTimer repeatDelayTimer_;
    QVariantAnimation fadeAnimation_;
    QString filePath_;
    int remainingRounds_ = 0;
    int maximumMilliseconds_ = 5000;
    qreal targetVolume_ = 0.7;
    bool finishingRound_ = false;
    bool continueAfterFade_ = false;
    bool hasPendingPlayback_ = false;
    QString pendingFilePath_;
    int pendingVolumePercent_ = 70;
    int pendingMaximumSeconds_ = 5;
    int pendingRepeatCount_ = 1;
};
