#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "myfilereader.h"
#include "myfilewriter.h"
#include "mysubtitlefileparser.h"
#include "myattributesconverter.h"
#include "SubtitlesParsers/SubRip/subripparser.h"
#include "SubtitlesParsers/DcSubtitle/dcsubparser.h"
#include <QFileDialog>
#include <QColorDialog>
#include <QString>
#include <QKeyEvent>
#include <QMessageBox>


#define SEC_TO_MSEC 1000


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Init UI
    ui->setupUi(this);

    // Hide columns to user
    for ( qint32 i = 5; i < ui->subTable->columnCount(); i++ ) {
        ui->subTable->setColumnHidden(i, true);
    }

    // Install an event filter on all the MainWindow
    this->installEventFilter(this);

    // Init flags
    mTextPosChangedBySoft = false;
    mTextPosChangedByUser = false;
    mTextFontChangedBySoft = false;
    mTextFontChangedByUser = false;
    mVideoPositionChanged = false;

    // Init the subtitles database
    ui->subTable->initStTable(0);

    //ui->stEditDisplay->setStyleSheet("background: transparent; color: yellow");


    // Init default properties
    qApp->setProperty("prop_FontSize_pt", ui->fontSizeSpinBox->cleanText());
    qApp->setProperty("prop_FontName", ui->fontIdComboBox->currentText());

    if ( ui->fontItalicButton->isChecked() ) {
        qApp->setProperty("prop_FontItalic", "yes");
    }
    else {
        qApp->setProperty("prop_FontItalic", "no");
    }

    if ( ui->fontUnderlinedButton->isChecked() ) {
        qApp->setProperty("prop_FontUnderlined", "yes");
    }
    else {
        qApp->setProperty("prop_FontUnderlined", "no");
    }

    if ( ui->fontColorRButton->isChecked() ) {
        qApp->setProperty("prop_FontColor_rgba", "FFFF0000");
    }
    else if ( ui->fontColorGButton->isChecked() ) {
        qApp->setProperty("prop_FontColor_rgba", "FF00FF00");
    }
    else if ( ui->fontColorBButton->isChecked() ) {
        qApp->setProperty("prop_FontColor_rgba", "FF0000FF");
    }
    else if ( ui->fontColorYButton->isChecked() ) {
        qApp->setProperty("prop_FontColor_rgba", "FFFFFF00");
    }
    else if ( ui->fontColorBlButton->isChecked() ) {
        qApp->setProperty("prop_FontColor_rgba", "FF000000");
    }
    else if ( ui->fontColorWButton->isChecked() ) {
        qApp->setProperty("prop_FontColor_rgba", "FFFFFFFF");
    }
    else if ( ui->fontColorOtherButton->isChecked() ) {
        qApp->setProperty("prop_FontColor_rgba",  ui->fontColorOtherText->text());
    }

    qApp->setProperty("prop_Valign", ui->vAlignBox->currentText());
    qApp->setProperty("prop_Halign", ui->hAlignBox->currentText());
    qApp->setProperty("prop_Vposition_percent", QString::number(ui->vPosSpinBox->value(), 'f', 1));
    qApp->setProperty("prop_Hposition_percent", QString::number(ui->hPosSpinBox->value(), 'f', 1));

    ui->stEditDisplay->defaultSub();

    ui->waveForm->openFile("", "");

    // Init text edit parameter from tool boxes
    this->updateTextPosition();
    this->updateTextFont(false);

    // Init the SIGNAL / SLOT connections
    connect(ui->videoPlayer, SIGNAL(durationChanged(qint64)),this, SLOT(updateStEditSize()));
    connect(ui->videoPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(videoPositionChanged(qint64)));

    connect(ui->waveForm, SIGNAL(markerPositionChanged(qint64)), this, SLOT(waveformMarerkPosChanged(qint64)));
    connect(ui->waveForm, SIGNAL(ctrlLeftClickEvent(qint64)), this, SLOT(changeCurrentSubStartTime(qint64)));
    connect(ui->waveForm, SIGNAL(ctrlRightClickEvent(qint64)), this, SLOT(changeCurrentSubEndTime(qint64)));
    connect(ui->waveForm, SIGNAL(shiftLeftClickEvent(qint64)), this, SLOT(shiftCurrentSubtitle(qint64)));

    connect(ui->stEditDisplay, SIGNAL(cursorPositionChanged()), this, SLOT(updateSubTable()));
    connect(ui->stEditDisplay, SIGNAL(subDatasChanged(MySubtitles)), ui->subTable, SLOT(updateDatas(MySubtitles)));
    connect(ui->stEditDisplay, SIGNAL(textLineFocusChanged(TextFont, TextLine)), this, SLOT(updateToolBox(TextFont, TextLine)));

    connect(ui->subTable, SIGNAL(itemSelectionChanged(qint64)), this, SLOT(currentItemChanged(qint64)));
    connect(ui->subTable, SIGNAL(newTextToDisplay(MySubtitles)), this, SLOT(updateTextEdit(MySubtitles)));
}

MainWindow::~MainWindow() {
    delete ui;
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event) {

    // Catch shortcut key events
    if ( event->type() == QEvent::KeyPress ) {

        MySubtitles new_subtitle;
        QList<MySubtitles> subtitle_list;
        qint32 current_subtitle_index;
        qint64 current_position_ms;

        QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

        if ( key_event->key() == Qt::Key_F1 ) {

            current_position_ms = ui->waveForm->currentPositonMs();

            if ( key_event->modifiers() == Qt::ShiftModifier ) {
                // Shift + F1 : shift subtitle to the current position
                this->shiftCurrentSubtitle(current_position_ms);
            }
            else {
                // F1 set current subtitle start time
                this->changeCurrentSubStartTime(current_position_ms);
            }
        }
        else if ( key_event->key() == Qt::Key_F2 ) {

            // F2 set current subtitle end time
            current_position_ms = ui->waveForm->currentPositonMs();
            this->changeCurrentSubEndTime(current_position_ms);
        }
        else if ( key_event->key() == Qt::Key_F3 ) {

            qint64 end_time_ms = ui->waveForm->currentPositonMs();
            current_subtitle_index = ui->subTable->currentIndex();

            // F3 set current subtitle end time + ( add new subtitle entry or move the next subtitle start time)

            // Change the current subtitle "end time", continue if ok
            if ( this->changeCurrentSubEndTime(end_time_ms) == true ) {

                qint64 start_time_ms = end_time_ms + ( ( (qreal)SEC_TO_MSEC / qApp->property("prop_FrameRate_fps").toReal() ) * qApp->property("prop_SubMinInterval_frame").toReal() );

                // If next subtitle exist
                if ( ( current_subtitle_index + 1 ) < ui->subTable->subtitlesCount() ) {

                    // Move the next subtitle "start time" at the current subtitle "end time" + X frames,
                    // where X is the minimum interval between two subtitles
                    if ( ui->subTable->setStartTime(start_time_ms, ( current_subtitle_index + 1 ) ) == false ) {
                        QString error_msg = ui->subTable->errorMsg();
                        QMessageBox::warning(this, "Set start time", error_msg);
                    }
                    else {
                        // Redraw the subtitle zone in the waveform
                        ui->waveForm->changeZoneStartTime( ( current_subtitle_index + 1 ), start_time_ms);
                    }
                } // If next subtitle doesn't exist (current is the last)
                else {

                    new_subtitle = ui->stEditDisplay->getDefaultSub();

                    // Inserte new subtitle start_time at X frames of the current subtitle
                    // where X is the minimum interval in frame between two subtitle (defined in "settings" by the user)
                    if ( ui->subTable->insertNewSub( new_subtitle , start_time_ms  ) == false ) {
                        QString error_msg = ui->subTable->errorMsg();
                        QMessageBox::warning(this, "Insert subtitle", error_msg);
                    }
                    else {
                        // Draw subtitle zone in the waveform
                        subtitle_list.append(new_subtitle);
                        current_subtitle_index = ui->subTable->currentIndex();
                        ui->waveForm->drawSubtitlesZone(subtitle_list, current_subtitle_index);
                        ui->waveForm->changeZoneColor(current_subtitle_index);
                    }
                }
            }
        }
        else if ( key_event->key() == Qt::Key_F4 ) {

            // F4 add new subtitle entry after current subtitle
            new_subtitle = ui->stEditDisplay->getDefaultSub();

            if ( ui->subTable->insertNewSubAfterCurrent( new_subtitle ) == false ) {
                QString error_msg = ui->subTable->errorMsg();
                QMessageBox::warning(this, "Insert subtitle", error_msg);
            }
            else {
                subtitle_list.append(new_subtitle);
                current_subtitle_index = ui->subTable->currentIndex();
                ui->waveForm->drawSubtitlesZone(subtitle_list, current_subtitle_index);
                ui->waveForm->changeZoneColor(current_subtitle_index);
            }
        }
        else if ( key_event->key() == Qt::Key_F5 ) {

            // Ctrl + BackSpace remove current subtitle
//            Qt::KeyboardModifiers event_keybord_modifier = key_event->modifiers();

//            if ( event_keybord_modifier == Qt::ControlModifier ) {

                // Remove the current subtitle item from the waveform
                current_subtitle_index = ui->subTable->currentIndex();
                ui->waveForm->removeSubtitleZone(current_subtitle_index);

                // Remove the current subtitle from the database
                ui->subTable->deleteCurrentSub();

                // Current subtitle changed, change the item color in the waveform
                current_subtitle_index = ui->subTable->currentIndex();
                ui->waveForm->changeZoneColor(current_subtitle_index);
//            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

// Current item selection changed in the database.
void MainWindow::currentItemChanged(qint64 positionMs) {

    // Update the waveform marker position
    ui->waveForm->updatePostionMarker(positionMs);
}

// Update the subtitle text in the database
void MainWindow::updateSubTable() {

    // There are no subtitle for the current time. Try to add new subtitle entry
    if ( ui->subTable->isNewEntry() ) {
        MySubtitles new_subtitle = ui->stEditDisplay->subtitleData();
        if ( ui->subTable->insertNewSub(new_subtitle) == false ) {

            QString error_msg = ui->subTable->errorMsg();
            QMessageBox::warning(this, "Insert subtitle", error_msg);
            MySubtitles empty_subtitle;
            updateTextEdit(empty_subtitle);
        }
        else {
            // Draw subtitle zone in the waveform
            QList<MySubtitles> subtitle_list;
            subtitle_list.append(new_subtitle);
            qint32 current_subtitle_index = ui->subTable->currentIndex();
            ui->waveForm->drawSubtitlesZone(subtitle_list, current_subtitle_index);
            ui->waveForm->changeZoneColor(current_subtitle_index);
        }
    } // There are a subtitle for the current time. Update the text in the database
    else {
        ui->subTable->updateText( ui->stEditDisplay->text());
    }
}

// Video positon changed. Update the waveform marker position
void MainWindow::videoPositionChanged(qint64 positionMs) {

    // Update only if it's not the marker that commanded video position changment
    if ( mMarkerPosChanged == false ) {
        mVideoPositionChanged = true;
        ui->waveForm->updatePostionMarker(positionMs);
    }
    else {
        mMarkerPosChanged = false;
    }
}

// Interface to update the position of the player
void MainWindow::updateVideoPosition(qint64 positionMs) {

    if ( ui->videoPlayer->videoAvailable() ) {
        ui->videoPlayer->setPosition(positionMs);
    }
    else {
        mMarkerPosChanged = false;
    }
}

// Manage the waveform marker position changment
void MainWindow::waveformMarerkPosChanged(qint64 positionMs) {

    // Change the video positon if its not the mediaplayer that has changed the waveform marker position
    if ( mVideoPositionChanged == false ) {
        mMarkerPosChanged = true;
        this->updateVideoPosition(positionMs);
    }
    else {
        mVideoPositionChanged = false;
    }

    // Change the current item selection
    ui->subTable->updateStDisplay(positionMs);
}

// Interface to update the text of the text edit area
void MainWindow::updateTextEdit(MySubtitles subtitle) {

    ui->stEditDisplay->setText(subtitle);
    ui->waveForm->changeZoneColor(ui->subTable->currentIndex());
}

// Change the current subtitle "start time"
bool MainWindow::changeCurrentSubStartTime(qint64 positionMs) {

    qint32 current_subtitle_index = ui->subTable->currentIndex();

    // Check if there are a subtitle selected
    if ( current_subtitle_index >= 0 ) {
        // Check if there are a subtitle before
        if ( ( current_subtitle_index - 1 ) >= 0) {

            MySubtitles previous_subtitle = ui->subTable->getSubInfos(current_subtitle_index - 1);
            qint64 previous_sub_start_time_ms = MyAttributesConverter::timeStrHMStoMs( previous_subtitle.startTime() );

            // Check time validity
            if ( previous_sub_start_time_ms >= 0 ) {

                // If the current sub new "start time" is before the previous sub "start time" (previous sub completely recover)
                // Abord
                if ( positionMs <= previous_sub_start_time_ms ) {
                    QMessageBox::warning(this, "Set start time", "start time < previous start time");
                    return false;
                }

                qint64 previous_sub_end_time_ms = MyAttributesConverter::timeStrHMStoMs( previous_subtitle.endTime() );

                // Check time validity
                if ( previous_sub_end_time_ms >=0 ) {

                    // If the current sub new "start time" is between the previous sub "start/end time"
                    // Move the previous subtitle "end time" at the current subtitle "start time" - X frames,
                    // where X is the minimum interval between two subtitles
                    if ( positionMs <= previous_sub_end_time_ms) {

                        qint64 new_end_time_ms = positionMs - ( ( (qreal)SEC_TO_MSEC / qApp->property("prop_FrameRate_fps").toReal() ) * qApp->property("prop_SubMinInterval_frame").toReal() );

                        if ( ui->subTable->setEndTime( new_end_time_ms , ( current_subtitle_index - 1 ) ) == false ) {
                            QMessageBox::warning(this, "Set start time", "start time < previous start time");
                            return false;
                        }
                        else {
                            // If ok, redraw the subtitle zone in the waveform
                            ui->waveForm->changeZoneEndTime( ( current_subtitle_index - 1 ), new_end_time_ms);
                        }
                    }
                } // Time not valid, abord
                else return false;
            }   // Time not valid, abord
            else return false;
        }

        // Change the current sub "start time"
        if ( ui->subTable->setStartTime(positionMs, current_subtitle_index) == false ) {
            QString error_msg = ui->subTable->errorMsg();
            QMessageBox::warning(this, "Set start time", error_msg);
            return false;
        }
        else {
            // If ok, redraw the subtitle zone in the waveform
            ui->waveForm->changeZoneStartTime(current_subtitle_index, positionMs);
        }
    }
    return true;
}

// Change the current subtitle "start time"
bool MainWindow::changeCurrentSubEndTime(qint64 positionMs) {

    qint32 current_subtitle_index = ui->subTable->currentIndex();

    // Check if there are a subtitle selected
    if ( current_subtitle_index < ui->subTable->subtitlesCount() ) {
        // Check if there are a subtitle after
        if ( ( current_subtitle_index + 1 ) < ui->subTable->subtitlesCount() ) {

            MySubtitles next_subtitle = ui->subTable->getSubInfos(current_subtitle_index + 1);
            qint64 next_sub_end_time_ms = MyAttributesConverter::timeStrHMStoMs( next_subtitle.endTime() );

            // Check time validity
            if ( next_sub_end_time_ms >= 0 ) {

                // If the current sub new "end time" is after the next sub "end time" (next sub completely recover)
                // Abord
                if ( positionMs >= next_sub_end_time_ms ) {
                    QMessageBox::warning(this, "Set end time", "end time > next end time");
                    return false;
                }

                qint64 next_sub_start_time_ms = MyAttributesConverter::timeStrHMStoMs( next_subtitle.startTime() );

                // Check time validity
                if ( next_sub_start_time_ms >=0 ) {

                    // If the current sub new "end time" is between the next sub "start/end time"
                    // Move the next subtitle "start time" at the current subtitle "end time" + X frames,
                    // where X is the minimum interval between two subtitles
                    if ( positionMs >= next_sub_start_time_ms) {

                        qint64 new_start_time_ms = positionMs + ( ( (qreal)SEC_TO_MSEC / qApp->property("prop_FrameRate_fps").toReal() ) * qApp->property("prop_SubMinInterval_frame").toReal() );

                        if ( ui->subTable->setStartTime( new_start_time_ms , ( current_subtitle_index + 1 ) ) == false ) {
                            QMessageBox::warning(this, "Set end time", "end_time > next end time");
                            return false;
                        }
                        else {
                            // If ok, redraw the subtitle zone in the waveform
                            ui->waveForm->changeZoneStartTime( ( current_subtitle_index + 1 ), new_start_time_ms);
                        }
                    }
                } // Time not valid, abord
                else return false;
            } // Time not valid, abord
            else return false;
        }

        // Change the current sub "end time"
        if ( ui->subTable->setEndTime(positionMs, current_subtitle_index) == false ) {
            QString error_msg = ui->subTable->errorMsg();
            QMessageBox::warning(this, "Set end time", error_msg);
            return false;
        }
        else {
            // If ok, redraw the subtitle zone in the waveform
            ui->waveForm->changeZoneEndTime(current_subtitle_index, positionMs);
        }
    }
    return true;
}

// Move the current subtitle at given position
void MainWindow::shiftCurrentSubtitle(qint64 positionMs) {

    qint32 current_subtitle_index = ui->subTable->currentIndex();

    MySubtitles current_subtitle = ui->subTable->getSubInfos(current_subtitle_index);
    qint64 current_sub_start_time_ms = MyAttributesConverter::timeStrHMStoMs( current_subtitle.startTime() );
    qint64 current_sub_end_time_ms = MyAttributesConverter::timeStrHMStoMs( current_subtitle.endTime() );

    // Check time validity
    if ( ( current_sub_start_time_ms >= 0 ) &&  ( current_sub_end_time_ms >= 0 ) ) {

        qint32 current_sub_durationMs = current_sub_end_time_ms - current_sub_start_time_ms;

        // Check if the new "start time" is after the actual "start time"
        if ( positionMs > current_sub_start_time_ms ) {

            // Move the "end time" first
            if ( this->changeCurrentSubEndTime( positionMs + current_sub_durationMs ) == true ) {
                // If "end time" well moved, move the "start time"
                this->changeCurrentSubStartTime(positionMs);
            }
        }// Check if the new "start time" is before the actual "start time"
        else if ( positionMs < current_sub_start_time_ms ) {
            // Move the "start time" first
            if ( this->changeCurrentSubStartTime( positionMs ) == true ) {
                // If "start time" well moved, move the "end time"
                this->changeCurrentSubEndTime( positionMs + current_sub_durationMs );
            }
        }
    }
}

// Manage the openning of video file when "Open" button is triggered
void MainWindow::on_actionOpen_triggered()
{
    // Open a video file with the palyer
    QString video_file_name = ui->videoPlayer->openFile();

    // If the media player has openned the video
    if ( !video_file_name.isEmpty() ) {

        // Open the corresponding waveform
        QString wf_file_name = video_file_name;
        wf_file_name.truncate(wf_file_name.lastIndexOf("."));
        wf_file_name.append(".wf");

        ui->waveForm->openFile(wf_file_name, video_file_name);
    }
}

// Resize the subtitles edit zone in function of the current video size
void MainWindow::updateStEditSize() {

    QSizeF video_item_size;
    QSizeF video_native_size;
    QSizeF video_current_size;
    qreal video_item_ratio;
    qint32 video_x_pos;
    qint32 video_y_pos;

    // Video item size is the size of the zone where is displayed the video
    video_item_size = ui->videoPlayer->videoItemSize();
    video_item_ratio = video_item_size.width() / video_item_size.height();

    // Video native size is the real full size of the media
    video_native_size = ui->videoPlayer->videoItemNativeSize();

    // Video current size is the displayed size of the video,
    // scaled on the video item with its ratio kept

    // When no video loaded, the subtitles edit zone should be mapped on the video item
    // 9 is the margins value of the mediaplayer;
    video_current_size = video_item_size;
    video_x_pos = 9;
    video_y_pos = 9;

    // If a video is loaded
    if ( video_native_size.isValid() ) {

        // Retrieve the video current size (native size scaled in item size)
        video_current_size = video_native_size.scaled(video_item_size, Qt::KeepAspectRatio);

        qreal video_native_ratio = video_native_size.width() / video_native_size.height();

        // Compare the ratio of native video and video item
        // to knwon if the video is full scale on width or on height of the item
        if ( video_native_ratio > video_item_ratio ) {
            video_x_pos = 9;
            video_y_pos = (qint32)( qRound( video_item_size.height() - video_current_size.height() ) / 2.0 ) + 9;
        }
        else if ( video_native_ratio < video_item_ratio ) {
            video_x_pos = (qint32)( qRound( video_item_size.width() - video_current_size.width() ) / 2.0 ) + 9;
            video_y_pos = 9;
        }
    }

    // Set the edit region size and position mapped on video.
    ui->stEditDisplay->setFixedSize(video_current_size.toSize());
    ui->stEditDisplay->move(video_x_pos, video_y_pos);

}

void MainWindow::resizeEvent(QResizeEvent* event) {

    updateStEditSize();
    QMainWindow::resizeEvent(event);
}


void MainWindow::on_actionQuit_triggered()
{
    close();
}

// Manage the subtitles import when "Import" button triggered
void MainWindow::on_actionImport_Subtitles_triggered()
{
    QString selected_filter;

    // Open a subtitle file
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open Subtitles"),QDir::homePath() + "/Videos",
                                                     tr("SubRip (*.srt);;DCSubtitle (*.xml);;All Files(*)"),
                                                     &selected_filter);


    MyFileReader file_reader(file_name, "UTF-8");

    if ( file_reader.readFile(file_name, "UTF-8") == false ) {
        QString error_msg = file_reader.errorMsg();
        QMessageBox::warning(this, "Import subtitles", error_msg);
        return;
    }

    // Choose a parser to parse the file
    MySubtitleFileParser* parser;

    if ( selected_filter ==  ("SubRip (*.srt)") ) {
        parser = new SubRipParser();
    }
    else if ( selected_filter ==  ("DCSubtitle (*.xml)") ) {
        parser = new DcSubParser();
    }
    else {
        //TO DO
    }

    // Parse the file
    QList<MySubtitles> subtitles_list = parser->open(file_reader);

    // If parsing is successfull, load the subtitles list in the database
    if ( !subtitles_list.isEmpty() ) {

        ui->subTable->loadSubtitles(subtitles_list);

        // Remove all and draw subtitles zones in the waveform
        ui->waveForm->removeAllSubtitlesZones();
        qint32 current_subtitle_index = ui->subTable->currentIndex();
        ui->waveForm->drawSubtitlesZone(subtitles_list, current_subtitle_index);
        ui->waveForm->changeZoneColor(current_subtitle_index);
    }
}

// Manage the subtitles export when "Export" button triggered
void MainWindow::on_actionExport_Subtitles_triggered() {

    QString selected_filter;

    // Create or open file to write datas
    QString file_name = QFileDialog::getSaveFileName(this, tr("Open Subtitles"),QDir::homePath() + "/Videos",
                                                     tr("SubRip (*.srt);;DCSubtitle (*.xml);;All Files(*)"),
                                                     &selected_filter);


    MyFileWriter file_writer(file_name, "UTF-8");

    // Choose the good parser
    MySubtitleFileParser* parser;

    if ( selected_filter ==  ("SubRip (*.srt)") ) {
        parser = new SubRipParser();
    }
    else if ( selected_filter ==  ("DCSubtitle (*.xml)") ) {
        parser = new DcSubParser();
    }
    else {
        //TO DO
    }

    // Retreive the subtitles datas from databases
    QList<MySubtitles> subtitles_list = ui->subTable->saveSubtitles();

    // If there are datas, write it to asked format
   if ( !subtitles_list.isEmpty() ) {

       parser->save(file_writer, subtitles_list);

       if ( file_writer.toFile() == false ) {
           QString error_msg = file_writer.errorMsg();
           QMessageBox::warning(this, "Export subtitles", error_msg);
       }
   }
}

void MainWindow::on_settingsButton_clicked()
{
    switch ( ui->stackedWidget->currentIndex() ) {
    case 0 :
        ui->stackedWidget->setCurrentIndex(1);
        break;
    case 1 :
        ui->stackedWidget->setCurrentIndex(0);
        break;
    default:
        ui->stackedWidget->setCurrentIndex(0);
        break;
    }
}

void MainWindow::displayErrorMsg(QString errorMsg) {

    QMessageBox::warning(this, "Error", errorMsg);
}



// ******************************** Tool Box ***************************************************************//

void MainWindow::updateToolBox(TextFont textFont, TextLine textLine) {

    this->updatePosToolBox(textLine);

    this->updateFontToolBox(textFont);
}

// Update the "position" tool boxes
void MainWindow::updatePosToolBox(TextLine textLine) {

    // Update only if the user is not changing the parameters with tool boxes
    if ( mTextPosChangedByUser != true ) {

        // Set the flag to indicate that toolboxes states are changing
        mTextPosChangedBySoft = true;

        // Set the parameters to the boxes
        ui->hAlignBox->setCurrentText( textLine.textHAlign() );
        ui->hPosSpinBox->setValue( textLine.textHPosition().toDouble() );
        ui->vAlignBox->setCurrentText( textLine.textVAlign() );
        ui->vPosSpinBox->setValue( textLine.textVPosition().toDouble() );

    }
    mTextPosChangedBySoft = false;
}

// Change the "position" of the current "edit line" from the toolboxes parameters
void MainWindow::updateTextPosition() {

    mTextPosChangedByUser = true;

    TextLine text_line;

    text_line.setTextHAlign( ui->hAlignBox->currentText() );

    QString hPos_str = QString::number(ui->hPosSpinBox->value(), 'f', 1);
    text_line.setTextHPosition( hPos_str );

    text_line.setTextVAlign( ui->vAlignBox->currentText() );

    QString vPos_str = QString::number(ui->vPosSpinBox->value(), 'f', 1);
    text_line.setTextVPosition( vPos_str );

    ui->stEditDisplay->updateTextPosition(text_line);

    mTextPosChangedByUser = false;
}

// Vertical alignment toolbox value changed
void MainWindow::on_vAlignBox_activated(const QString &value) {

    qApp->setProperty("prop_Valign", value);

    // Tool box changed by user
    if ( mTextPosChangedBySoft == false ) {
        // Update the text edit position
        updateTextPosition();
    }
}

// Vertical position toolbox value changed
void MainWindow::on_vPosSpinBox_valueChanged(const QString &value) {

    qApp->setProperty("prop_Vposition_percent", value);

    if ( mTextPosChangedBySoft == false ) {
        updateTextPosition();
    }
}

// Horizontal alignment toolbox value changed
void MainWindow::on_hAlignBox_activated(const QString &value) {

    qApp->setProperty("prop_Halign", value);

    if ( mTextPosChangedBySoft == false ) {
        updateTextPosition();
    }
}

// Horizontal position toolbox value changed
void MainWindow::on_hPosSpinBox_valueChanged(const QString &value) {

    qApp->setProperty("prop_Hposition_percent", value);

    if ( mTextPosChangedBySoft == false ) {
        updateTextPosition();
    }
}

// Update the "font" tool boxes
void MainWindow::updateFontToolBox(TextFont textFont) {

    // Update only if the user is not changing the parameters with tool boxes
    if ( mTextFontChangedByUser != true ) {

        // Set the flag to indicate that toolboxes states are changing
        mTextFontChangedBySoft = true;

        // Set the parameters to the boxes
        QFont font(textFont.fontId());
        ui->fontIdComboBox->setCurrentFont(font);
        ui->fontSizeSpinBox->setValue(textFont.fontSize().toInt());

        QString font_color = textFont.fontColor();

        qApp->setProperty("prop_FontColor_rgba", font_color);

        ui->fontColorOtherText->setEnabled(false);

        if ( font_color == "FFFF0000" ) {
            ui->fontColorRButton->setChecked(true);
        }
        else if ( font_color == "FF00FF00" ) {
            ui->fontColorGButton->setChecked(true);
        }
        else if ( font_color == "FF0000FF" ) {
            ui->fontColorBButton->setChecked(true);
        }
        else if ( font_color == "FFFFFF00" ) {
            ui->fontColorYButton->setChecked(true);
        }
        else if ( font_color == "FF000000" ) {
            ui->fontColorBlButton->setChecked(true);
        }
        else if ( font_color == "FFFFFFFF") {
            ui->fontColorWButton->setChecked(true);
        }
        else {
            ui->fontColorOtherButton->setChecked(true);
            ui->fontColorOtherText->setEnabled(true);
            ui->fontColorOtherText->setText(textFont.fontColor());
        }

        if ( textFont.fontItalic().contains("yes", Qt::CaseInsensitive) == true ) {
            ui->fontItalicButton->setChecked(true);
        }
        else {
            ui->fontItalicButton->setChecked(false);
        }

        if ( textFont.fontUnderlined().contains("yes", Qt::CaseInsensitive) == true ) {
            ui->fontUnderlinedButton->setChecked(true);
        }
        else {
            ui->fontUnderlinedButton->setChecked(false);
        }

    }
    mTextFontChangedBySoft = false;
}

// Change the "font" of the current "edit line" from the font toolboxes parameters
void MainWindow::updateTextFont(bool customColorClicked) {

    mTextFontChangedByUser = true;

    TextFont text_font;

    if ( ui->fontLabel->isEnabled() == true )  {

        QFont font = ui->fontIdComboBox->currentFont();
        text_font.setFontId( font.family() );

        text_font.setFontSize(ui->fontSizeSpinBox->cleanText());

        ui->fontColorOtherText->setEnabled(false);

        if ( ui->fontColorRButton->isChecked() ) {
            text_font.setFontColor("FFFF0000");
            qApp->setProperty("prop_FontColor_rgba", "FFFF0000");
        }
        else if ( ui->fontColorGButton->isChecked() ) {
            text_font.setFontColor("FF00FF00");
            qApp->setProperty("prop_FontColor_rgba", "FF00FF00");
        }
        else if ( ui->fontColorBButton->isChecked() ) {
            text_font.setFontColor("FF0000FF");
            qApp->setProperty("prop_FontColor_rgba", "FF0000FF");
        }
        else if ( ui->fontColorYButton->isChecked() ) {
            text_font.setFontColor("FFFFFF00");
            qApp->setProperty("prop_FontColor_rgba", "FFFFFF00");
        }
        else if ( ui->fontColorBlButton->isChecked() ) {
            text_font.setFontColor("FF000000");
            qApp->setProperty("prop_FontColor_rgba", "FF000000");
        }
        else if ( ui->fontColorWButton->isChecked() ) {
            text_font.setFontColor("FFFFFFFF");
            qApp->setProperty("prop_FontColor_rgba", "FFFFFFFF");
        }
        else if ( ui->fontColorOtherButton->isChecked() ) {

            QString font_color_str;

            if ( customColorClicked == true ) {
                bool ok;
                QColor init_color = QColor::fromRgba( ui->fontColorOtherText->text().toUInt(&ok, 16) );
                QColor font_color = QColorDialog::getColor(init_color, 0, "Select Color", QColorDialog::ShowAlphaChannel);
                font_color_str = QString::number( font_color.rgba(), 16 ).toUpper();
                ui->fontColorOtherText->setText(font_color_str);
            }
            else {
                font_color_str = ui->fontColorOtherText->text();
            }
            ui->fontColorOtherText->setEnabled(true);
            text_font.setFontColor(font_color_str);
            qApp->setProperty("prop_FontColor_rgba", font_color_str);
        }

        if ( ui->fontItalicButton->isChecked() ) {
            text_font.setFontItalic("yes");
        }
        else {
            text_font.setFontItalic("no");
        }

        if ( ui->fontUnderlinedButton->isChecked() ) {
            text_font.setFontUnderlined("yes");
        }
        else {
            text_font.setFontUnderlined("no");
        }
    }

    ui->stEditDisplay->updateTextFont(text_font);

    mTextFontChangedByUser = false;
}

// Font size toolbox changed
void MainWindow::on_fontSizeSpinBox_valueChanged(const QString &value) {

    qApp->setProperty("prop_FontSize_pt", value);

    // Tool box changed by user
    if ( mTextFontChangedBySoft == false ) {
        // Update the text edit font
        updateTextFont(false);
    }
}

// Font color selected (the 7 colors boxes are exclusive, if one is checked, the others are unchecked)
void MainWindow::on_fontColorRButton_toggled(bool checked) {

    if ( ( mTextFontChangedBySoft == false ) && ( checked == true ) ) {
        updateTextFont(false);
    }
}

void MainWindow::on_fontColorGButton_toggled(bool checked) {

    if ( ( mTextFontChangedBySoft == false ) && ( checked == true ) ) {
        updateTextFont(false);
    }
}

void MainWindow::on_fontColorBButton_toggled(bool checked) {

    if ( ( mTextFontChangedBySoft == false ) && ( checked == true ) ) {
        updateTextFont(false);
    }
}

void MainWindow::on_fontColorYButton_toggled(bool checked) {

    if ( ( mTextFontChangedBySoft == false ) && ( checked == true ) ) {
        updateTextFont(false);
    }
}

void MainWindow::on_fontColorBlButton_toggled(bool checked) {

    if ( ( mTextFontChangedBySoft == false ) && ( checked == true ) ) {
        updateTextFont(false);
    }
}

void MainWindow::on_fontColorWButton_toggled(bool checked) {

    if ( ( mTextFontChangedBySoft == false ) && ( checked == true ) ) {
        updateTextFont(false);
    }
}

void MainWindow::on_fontColorOtherButton_clicked() {

    if ( mTextFontChangedBySoft == false ) {
        updateTextFont(true);
    }
}

// Italic value changed
void MainWindow::on_fontItalicButton_toggled(bool checked) {

    if ( checked ) {
        qApp->setProperty("prop_FontItalic", "yes");
    }
    else {
        qApp->setProperty("prop_FontItalic", "no");
    }

    if ( mTextFontChangedBySoft == false ) {
        updateTextFont(false);
    }
}

// Underlined value changed
void MainWindow::on_fontUnderlinedButton_toggled(bool checked) {

    if ( checked ) {
        qApp->setProperty("prop_FontUnderlined", "yes");
    }
    else {
        qApp->setProperty("prop_FontUnderlined", "no");
    }

    if ( mTextFontChangedBySoft == false ) {
        updateTextFont(false);
    }
}

// Font name changed
void MainWindow::on_fontIdComboBox_currentFontChanged(const QFont &f) {

    qApp->setProperty("prop_FontName", f.family());

    if ( mTextFontChangedBySoft == false ) {
        updateTextFont(false);
    }
}
