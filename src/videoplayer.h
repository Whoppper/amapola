#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QMainWindow>
#include <QMediaPlayer>

namespace Ui {
class VideoPlayer;
}

class QMediaPlayer;
class QGraphicsVideoItem;
class QGraphicsScene;

// This widget manage a media player with a position slider, a clock and a play/pause button
class VideoPlayer : public QWidget
{
    Q_OBJECT

    
public:
    explicit VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();
    QSizeF videoItemSize();
    QSizeF videoItemNativeSize();

signals:
    void positionChanged(qint64);
    void durationChanged(qint64);
    void playerStateInfos(QString);
    void nativeSizeChanged(QSizeF);

public slots:
    QString openFile(QString fileName);
    void unloadMedia();
    void setPosition(qint64 videoPlayerPositionMs);
    qint64 playerPosition();
    bool videoAvailable();
    bool changePlaybackRate(bool moreSpeed);
    void on_playButton_clicked();
    void loadWaveForm(QString waveFormFileName);
    void updateTime(qint64 positionMs);

private slots:
    void updatePlayerState(QMediaPlayer::State state);
    void updateSliderPosition(qint64 playerPositionMs);
    void updateDuration();
    void resizeEvent(QResizeEvent* event);
    void sliderMoved(qint64 positionMs);
    void checkVideoNativeSize(QSizeF videoNativeSize);

    
private:
    // Qt designer ui reference
    Ui::VideoPlayer *ui;

    // Media player object
    QMediaPlayer* mpPlayer;

    // Graphics scene to display video item
    QGraphicsScene* mpVideoScene;

    // Graphics item to draw video
    QGraphicsVideoItem* mpVideoItem;

    // Player position changed notify interval
    qint16 mPlayerPositionNotifyIntervalMs;

    // Flag to check if the slider was moved by user
    bool mTimeSliderPositionChangedByGui;

    // Video duration time in millisecond
    qint64 mVideoDuration;

    // Check if waveform data are ready
    bool mWaveFormReady;

    // Waveform data file name
    QString mWaveFormFileName;

    bool mMediaChanged;

};

#endif // VIDEOPLAYER_H
