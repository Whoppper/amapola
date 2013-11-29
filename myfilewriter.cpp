#include "myfilewriter.h"
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QTextCodec>

// Class to write text datas in a specified file "fileName" with specified format "textCodec"
MyFileWriter::MyFileWriter(QString fileName, QString textCodec) {

    mFileName = fileName;
    mTextCodec = textCodec;
    mStringData.clear();
    mRawData.clear();
    mErrorMsg = "";
}

// Add datas
void MyFileWriter::writeText(QString buffer) {

    mStringData += buffer;
}

// Add datas
void MyFileWriter::writeRawData(QList<quint8> buffer) {

    mRawData.append(buffer);
}

// Write the datas in the file
bool MyFileWriter::toFile(bool rawData) {

    if ( !mFileName.isEmpty() ) {
        QFile file_write(mFileName);
        if ( !file_write.open(QIODevice::WriteOnly) ) {
            mErrorMsg = "Can't open file" +mFileName;
            return false;
        } else {

            if ( rawData == false ) {

                // Open a stream to the file
                QTextStream stream_out(&file_write);

                // If a codec is specified, force the stream to use it
                if ( !mTextCodec.isEmpty() ) {
                    QTextCodec* text_codec = QTextCodec::codecForName(mTextCodec.toLocal8Bit());
                    stream_out.setCodec(text_codec);
                }

                // Write all the datas and close the file
                stream_out << mStringData;
                stream_out.flush();
            }
            else {

                char data_array[mRawData.size()];
                for ( quint16 i = 0; i < mRawData.count(); i++ ) {
                    data_array[i] = mRawData[i];
                }
                file_write.write(data_array, mRawData.size());
            }

            file_write.close();

            return true;
        }
    }
    mErrorMsg = "no file name specified";
    return false;
}

QString MyFileWriter::errorMsg() {
    return mErrorMsg;
}
