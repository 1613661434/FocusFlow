#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantAnimation>

class QAudioOutput;
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
    void stopImmediately();
    void beginFadeOut(bool continueRounds);
    void playFallbackBeep();

    QMediaPlayer *player_ = nullptr;
    QAudioOutput *audioOutput_ = nullptr;
    QTimer limitTimer_;
    QTimer repeatDelayTimer_;
    QVariantAnimation fadeAnimation_;
    QString filePath_;
    int remainingRounds_ = 0;
    int maximumMilliseconds_ = 5000;
    qreal targetVolume_ = 0.7;
    bool finishingRound_ = false;
    bool continueAfterFade_ = false;
};
