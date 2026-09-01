#pragma once

#include <QObject>
#include <QTimer>

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

private:
    void startRound();
    void playFallbackBeep();

    QMediaPlayer *player_ = nullptr;
    QAudioOutput *audioOutput_ = nullptr;
    QTimer limitTimer_;
    QString filePath_;
    int remainingRounds_ = 0;
    int maximumMilliseconds_ = 5000;
    bool finishingRound_ = false;
};
