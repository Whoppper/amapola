#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <myvideoplayer.h>
#include "mysubtitles.h"

namespace Ui {
class MainWindow;
}

class QTableWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();
    void on_actionQuit_triggered();
    void updateSubTableText();
    void updateSubTableDatas(MySubtitles subtitleDatas);
    void videoPositionChanged(qint64 positionMs);
    void currentItemChanged(qint64 positionMs);
    void waveformMarerkPosChanged(qint64 positionMs);
    void updateVideoPosition(qint64 positionMs);
    void updateTextEdit(MySubtitles subtitle);
    bool eventFilter(QObject* watched, QEvent* event);
    void resizeEvent(QResizeEvent* event);
    void updateStEditSize();
    void displayErrorMsg(QString);
    void displayInfo(QString info);
    void eraseInfo();
    bool changeSubStartTime(qint64 positionMs, qint32 refIndex = -1, bool multiChange = true);
    bool changeSubEndTime(qint64 positionMs, qint32 refIndex = -1, bool multiChange = true);
    void shiftSubtitles(qint64 positionMs, qint32 index = -1);
    void updateFrameRate();

    void on_actionImport_Subtitles_triggered();

    void on_settingsButton_clicked();

    void on_actionExport_Subtitles_triggered();


    //***************************Tool Box ***************************//

    void updateToolBox();

    void updatePosToolBox(TextLine textLine);
    TextLine getPosToolBox();
    void updateTextPosition();

    void on_vPosSpinBox_valueChanged(const QString &value);

    void on_hPosSpinBox_valueChanged(const QString &value);

    void on_vAlignBox_activated(const QString &value);

    void on_hAlignBox_activated(const QString &value);


    void updateFontToolBox(TextFont textFont);
    TextFont getFontToolBox(bool customColorClicked);
    void updateTextFont(bool customColorClicked);

    void on_fontSizeSpinBox_valueChanged(const QString &value);

    void on_fontColorRButton_toggled(bool checked);

    void on_fontColorGButton_toggled(bool checked);

    void on_fontColorBButton_toggled(bool checked);

    void on_fontColorYButton_toggled(bool checked);

    void on_fontColorBlButton_toggled(bool checked);

    void on_fontColorWButton_toggled(bool checked);

    void on_fontItalicButton_toggled(bool checked);

    void on_fontUnderlinedButton_toggled(bool checked);

    void on_fontIdComboBox_currentFontChanged(const QFont &f);

    void on_fontColorOtherButton_clicked();

    void on_firstNumSpinBox_customContextMenuRequested(const QPoint &pos);

    void on_lastNumSpinBox_customContextMenuRequested(const QPoint &pos);

    void on_syncApplyPushButton_clicked();

    void on_firstNewStartTimeEdit_customContextMenuRequested(const QPoint &pos);

    void on_lastNewStartTimeEdit_customContextMenuRequested(const QPoint &pos);

private:
    Ui::MainWindow* ui;

    // Flag if the toolbox was changed by user or by software
    // To only treat the user changment
    bool mTextPosChangedBySoft;
    bool mTextPosChangedByUser;
    bool mTextFontChangedBySoft;
    bool mTextFontChangedByUser;

    // Flag to avoid infinite inter call between video-player and waveform
    bool mVideoPositionChanged;
    bool mMarkerPosChanged;

    QTimer* mpDisplayInfoTimer;
};

#endif // MAINWINDOW_H
