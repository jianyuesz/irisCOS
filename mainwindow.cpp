#include "mainwindow.h"
#include "ui_mainwindow.h"

QString ffmpegPath = "ffmpeg";
QString cmd_start = "get",cmd_get_echo = "",cmd_set_echo = "";
bool isDone = false;

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::MainWindow),transcodingProcess(new QProcess)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::WindowMaximizeButtonHint;
    setWindowFlags(flags);
    setFixedSize(810,515);

    ui->btn_convert->setEnabled(true);
    ui->btn_start->setEnabled(false);
    ui->btn_cancel->setEnabled(false);

    connect(this->transcodingProcess,SIGNAL(started()),this,SLOT(processStarted()));
    connect(this->transcodingProcess,SIGNAL(readyReadStandardOutput()),this,SLOT(readyReadStandardOutput()));
    connect(this->transcodingProcess,SIGNAL(finished(int)),this,SLOT(processFinished()));
}

MainWindow::~MainWindow()
{
    this->transcodingProcess->close();
    delete ui;
}

void MainWindow::processStarted(){
    QApplication::setOverrideCursor(Qt::WaitCursor);
}

void MainWindow::processFinished(){
    QApplication::restoreOverrideCursor();
}

QString MainWindow::get_cutterStr(QString startStr,QString endStr){
    int poit_s = cmd_get_echo.indexOf(startStr);
    cmd_get_echo = cmd_get_echo.mid(poit_s,cmd_get_echo.length() - poit_s);
    int poit_e = cmd_get_echo.indexOf(endStr);
    return cmd_get_echo.mid(startStr.length(),poit_e - startStr.length());
}

QString MainWindow::set_cutterStr(QString startStr,QString endStr){
    int poit_s = cmd_set_echo.indexOf(startStr);
    cmd_set_echo = cmd_set_echo.mid(poit_s,cmd_set_echo.length() - poit_s);
    int poit_e = cmd_set_echo.indexOf(endStr);
    return cmd_set_echo.mid(startStr.length(),poit_e - startStr.length());
}

void MainWindow::log(QString info){
    ui->te_echo->moveCursor(QTextCursor::End);
    ui->te_echo->insertPlainText(info + "\r\n");
    ui->te_echo->moveCursor(QTextCursor::End);
}

void MainWindow::readyReadStandardOutput(){
    if(cmd_start == "get"){
        cmd_get_echo += this->transcodingProcess->readAllStandardOutput();
        if(cmd_get_echo.contains("At least one output file must be specified",Qt::CaseSensitive)){
            cmd_get_echo = cmd_get_echo.remove(QRegExp("\\s"));
            duration = get_cutterStr("Duration:",",");
            bitrate = get_cutterStr("bitrate:","kb/s");
            QString videoInfo = get_cutterStr("Video:","(default)");
            QStringList videoInfos = videoInfo.split(",");
            if(videoInfos.count() == 8){
                video = videoInfos.at(0);
                yuv = videoInfos.at(1);
                px = videoInfos.at(2);
                fps = videoInfos.at(4);
                audio = get_cutterStr("Audio:","Atleast");
            }else if (videoInfos.count() == 9) {
                video = videoInfos.at(0);
                yuv = videoInfos.at(1) + "," + videoInfos.at(2);
                px = videoInfos.at(3);
                fps = videoInfos.at(5);
                audio = get_cutterStr("Audio:","Atleast");
            }

            if(videoInfos.count() == 8 || videoInfos.count() == 9){
                QStringList durationList = duration.split(":");
                if(durationList.count() == 3){
                    QString h = durationList.at(0);
                    QString m = durationList.at(1);
                    QString s = durationList.at(2);
                    timelen = h.toDouble() * 60 * 60 + m.toDouble() * 60 + s.toDouble();
                    frame = timelen * fps.mid(0,fps.indexOf("fps")).toDouble();
                    log("video:" + video);
                    log("audio:" + audio);

                    log("duration:" + duration + " s");
                    log("yuv:" + yuv);
                    log("bitrate:" + bitrate + "kb/s");
                    log("px:" + px);
                    log("fps:" + fps);
                    log("frame:" + QString::number(frame,10,2));
                    log("-------------------------");
                    this->arguments.clear();
                    this->arguments << "-i" << this->inputFileName;
                    if(ui->rb_pvr->isChecked()){
                        log("Convert to PVR format.");
                        if(video.indexOf("hevc") >= 0 || video.indexOf("h265") >= 0 || video.indexOf("x265") >= 0){
                            //this->arguments << "-vcodec" << "copy";
                        }else log("The conversion is encoded as HEVC.");

                        if(ui->cb_purevideo->isChecked()){
                            this->arguments << "-vcodec" << "hevc_nvenc";
                            log("Enable hardware acceleration");
                        }else this->arguments << "-vcodec" << "hevc";

                        if(bitrate.toInt() < 15000 || bitrate.toInt() > 16000){
                            this->arguments << "-b" << ui->le_bitrate->text() + "K";
                            log("The code flow will be modified to " + ui->le_bitrate->text() + "kb/s.");
                        }else this->arguments << "-b" << bitrate + "K";

                        if(fps.toDouble() < 55 || fps.toDouble() > 65){
                            this->arguments << "-r" << ui->le_fps->text();
                            frame = timelen * ui->le_fps->text().toInt();
                            log("The frame rate will be changed to " + ui->le_fps->text() + "fps.");
                            log("redefine frame = " + QString::number(frame,10,2));
                        }

                        if(px.indexOf("3840x1920") < 0){
                            this->arguments << "-s" << "3840x1920";
                            log("The resolution will be changed to 3840x1920.");
                        }

                        this->arguments << "-acodec" << "copy";

                        QString vfStr = "eq=";
                        if(ui->dsb_contrast->value() != 1){
                            vfStr += "contrast=" + QString::number(ui->dsb_contrast->value(),10,1) + ":";
                            log("Adjust the contrast to " + QString::number(ui->dsb_contrast->value(),10,1));
                        }
                        if(ui->dsb_brightness->value() != 0){
                            vfStr += "brightness=" + QString::number(ui->dsb_brightness->value(),10,1) + ":";
                            log("Adjust the brightness to " + QString::number(ui->dsb_contrast->value(),10,1));
                        }
                        if(ui->dsb_saturation->value() != 1){
                            vfStr += "saturation=" + QString::number(ui->dsb_saturation->value(),10,1) + ":";
                            log("Adjust the saturation to " + QString::number(ui->dsb_contrast->value(),10,1));
                        }

                        if(ui->dsb_contrast->value() == 1 && ui->dsb_brightness->value() == 0 && ui->dsb_saturation->value() == 1){

                        }else this->arguments << "-vf" << vfStr.mid(0,vfStr.length() - 1);

                        if(this->outputFileName == ""){
                            this->outputFileName = inputFileName.mid(0,inputFileName.length()-4) + "_3D180_LR.mp4";
                            ui->le_outputFileName->setText(outputFileName);
                        }
                    }

                    if(ui->rb_lr->isChecked()){
                        log("Convert to left and right formats.");
                        if(video.indexOf("hevc") >= 0 || video.indexOf("h265") >= 0 || video.indexOf("x265") >= 0){
                            //this->arguments << "-vcodec" << "copy";
                        }else log("The conversion is encoded as HEVC.");

                        if(ui->cb_purevideo->isChecked()){
                            this->arguments << "-vcodec" << "hevc_nvenc";
                            log("Enable hardware acceleration");
                        }else this->arguments << "-vcodec" << "hevc";

                        if(bitrate.toInt() < 15000 || bitrate.toInt() > 16000){
                            this->arguments << "-b" << ui->le_bitrate->text() + "K";
                            log("The code flow will be modified to " + ui->le_bitrate->text() + "kb/s.");
                        }

                        if(fps.toDouble() < 55 || fps.toDouble() > 65){
                            this->arguments << "-r" << ui->le_fps->text();
                            frame = timelen * ui->le_fps->text().toInt();
                            log("The frame rate will be changed to " + ui->le_fps->text() + "fps.");
                            log("redefine frame = " + QString::number(frame,10,2));
                        }

                        if(px.indexOf("3840x1920") < 0){
                            this->arguments << "-s" << "3840x1920";
                            log("The resolution will be changed to 3840x1920.");
                        }

                        this->arguments << "-acodec" << "copy";

                        QString vfStr = "eq=";
                        if(ui->dsb_contrast->value() != 1){
                            vfStr += "contrast=" + QString::number(ui->dsb_contrast->value(),10,1) + ":";
                            log("Adjust the contrast to " + QString::number(ui->dsb_contrast->value(),10,1));
                        }
                        if(ui->dsb_brightness->value() != 0){
                            vfStr += "brightness=" + QString::number(ui->dsb_brightness->value(),10,1) + ":";
                            log("Adjust the brightness to " + QString::number(ui->dsb_contrast->value(),10,1));
                        }
                        if(ui->dsb_saturation->value() != 1){
                            vfStr += "saturation=" + QString::number(ui->dsb_saturation->value(),10,1) + ":";
                            log("Adjust the saturation to " + QString::number(ui->dsb_contrast->value(),10,1));
                        }

                        if(ui->dsb_contrast->value() == 1 && ui->dsb_brightness->value() == 0 && ui->dsb_saturation->value() == 1){

                        }else this->arguments << "-vf" << vfStr.mid(0,vfStr.length() - 1);

                        int poit_s = 0;
                        int poit_e = px.indexOf("x");
                        QString px_x = px.mid(0,poit_e);

                        poit_s = px.indexOf("x");
                        px = px.mid(poit_s,px.length() - poit_s);
                        poit_e = px.indexOf("[");

                        QString vf = "crop=" + QString::number(px_x.toInt() / 2,10,0) + ":" + px.mid(1,poit_e - 1) + ",stereo3d=abl:sbsl";
                        this->arguments << "-vf" << vf;

                        if(this->outputFileName == ""){
                            this->outputFileName = inputFileName.mid(0,inputFileName.length()-4) + "_UD_3D180_LR.mp4";
                            ui->le_outputFileName->setText(outputFileName);
                        }
                    }

                    this->arguments << this->outputFileName;
                    log("Will save " + outputFileName);
                    cmd_start = "set";
                    isDone = false;

                    ui->btn_convert->setEnabled(false);
                    ui->btn_start->setEnabled(true);
                    ui->btn_cancel->setEnabled(true);
                    ui->rb_lr->setEnabled(false);
                    ui->rb_pvr->setEnabled(false);
                    ui->cb_purevideo->setEnabled(false);

                    log(">>>ffmpeg " + this->arguments.join(QString(" ")));
                }
            }
        }
    }else if (cmd_start == "set") {
        cmd_set_echo = this->transcodingProcess->readAllStandardOutput();
        cmd_set_echo = cmd_set_echo.remove(QRegExp("\\s"));
        if(cmd_set_echo.indexOf("frame=") == 0){
            QString current_frame = set_cutterStr("frame=","fps");
            QString current_fps = set_cutterStr("fps=","q=");
            QString current_time = set_cutterStr("time=","bitrate");
            QString current_bitrate = set_cutterStr("bitrate=","dup");
            QString current_speed = set_cutterStr("speed=","x");

            double progress = current_frame.toDouble() / frame * 100;
            ui->ql_progress->setText("progress:" + QString::number(progress,10,2) + "%");
            ui->progressBar->setValue((int)progress);
#ifdef Q_OS_WIN32
            QStringList currentTimeList = current_time.split(":");
            if(currentTimeList.count() == 3){
                QString eh = currentTimeList.at(0);
                QString em = currentTimeList.at(1);
                QString es = currentTimeList.at(2);
                double etimelen = eh.toDouble() * 60 * 60 + em.toDouble() * 60 + es.toDouble();

                int remainingTime = (int)((timelen - etimelen) / current_speed.toDouble());
                int rh = (remainingTime)/21600;
                int rm = (remainingTime)%3600/60;
                int rs = (remainingTime)%60;
                QString remainingTimeStr = QString::number(rh,10,0) + ":" + QString::number(rm,10,0) + ":" + QString::number(rs,10,0);
                ui->ql_remainingTime->setText("remainingTime:" + remainingTimeStr);
            }
#endif
            ui->ql_time->setText("time:" + current_time + " / " + duration);
#ifdef Q_OS_WIN32
            ui->ql_speed->setText("speed:" + current_speed + "x");
#endif
            ui->ql_bitrate->setText("bitrate:" + current_bitrate);
            ui->ql_frame->setText("frame:" + current_frame + " / " + QString::number(frame,10,2));
            ui->ql_fps->setText("fps:" + current_fps);
        }
        if(cmd_set_echo.indexOf("encoded") >= 0){
            ui->ql_progress->setText("progress:100%");
            ui->ql_remainingTime->setText("remainingTime:00:00:00");
            ui->ql_time->setText("time:" + duration);
            ui->ql_speed->setText("speed:--");
            ui->ql_bitrate->setText("bitrate:--");
            ui->ql_frame->setText("frame:--");
            ui->ql_fps->setText("fps:--");
            ui->progressBar->setValue(100);

            isDone = true;

            QString current_frame = set_cutterStr("encoded","frames");
            QString current_time = set_cutterStr("in","s");
            QString current_fps = set_cutterStr("(",")");
            QString current_bitrate = set_cutterStr(",","kb/s");
            log("After the transformation>>>");
            log("frame:" + current_frame + "    bitrate:" + current_bitrate + "kb/s    elapsed time:" + current_time + "s    average:" + current_fps + "/s");
            log("-------------DONE--------------");
        }
        if(cmd_set_echo.indexOf("muxingoverhead") >= 0){
            ui->ql_progress->setText("progress:100%");
            ui->ql_remainingTime->setText("remainingTime:00:00:00");
            ui->ql_time->setText("time:" + duration);
            ui->ql_speed->setText("speed:--");
            ui->ql_bitrate->setText("bitrate:--");
            ui->ql_frame->setText("frame:--");
            ui->ql_fps->setText("fps:--");
            ui->progressBar->setValue(100);

            isDone = true;

            QString current_muxing_overhead = set_cutterStr("overhead:","%");
            log("After the transformation>>>");
            log("muxing overhead:" + current_muxing_overhead + "%");
            log("-------------DONE--------------");
        }
        if(cmd_set_echo.indexOf("kb/s:") >= 0 && ui->rb_lr->isChecked()){
            ui->ql_progress->setText("progress:100%");
            ui->ql_remainingTime->setText("remainingTime:00:00:00");
            ui->ql_time->setText("time:" + duration);
            ui->ql_speed->setText("speed:--");
            ui->ql_bitrate->setText("bitrate:--");
            ui->ql_frame->setText("frame:--");
            ui->ql_fps->setText("fps:--");
            ui->progressBar->setValue(100);

            isDone = true;

            log("-------------DONE--------------");
        }
        if(cmd_set_echo.indexOf("Conversionfailed") >= 0){
            ui->ql_progress->setText("progress:100%");
            ui->ql_remainingTime->setText("remainingTime:00:00:00");
            ui->ql_time->setText("time:" + duration);
            ui->ql_speed->setText("speed:--");
            ui->ql_bitrate->setText("bitrate:--");
            ui->ql_frame->setText("frame:--");
            ui->ql_fps->setText("fps:--");
            ui->progressBar->setValue(100);

            isDone = false;

            log("-------------FAILED--------------");
        }

        if(isDone && ui->cb_encrypt->isChecked()){
            QFile file(this->outputFileName);
            if(file.open(QIODevice::ReadOnly)){
                log("Began to encrypt");
                qint64 file_size = file.size();
                qint64 file_poit = 0;
                qint64 write_size = 1024;
                char more[32] = {0x00,0x00,0x00,0x00,0x00,0x56,0x25,0x30,
                                 0x52,0x00,0x02,0x55,0x58,0x4a,0x68,0x72,
                                 0x55,0x11,0x02,0x08,0x01,0x00,0x31,0x32,
                                 0x58,0x68,0x66,0x58,0x12,0x12,0x35,0x36};
                QString newFilePath = outputFileName.mid(0,outputFileName.lastIndexOf(".")) + ".pvr";
                QFile file_out(newFilePath);
                if(file_out.open(QIODevice::ReadWrite)){
                    log("Encryption for " + newFilePath);
                    file_out.write(more,sizeof(more));
                    do{
                        if(file_poit + write_size > file_size)write_size = file_size - file_poit;
                        file.seek(file_poit);
                        file_out.write(file.read(write_size),write_size);
                        file_poit += write_size;
                        ui->progressBar->setValue((int)(file_poit / file_size * 100));
                    }while(file_poit < file_size);
                    ui->progressBar->setValue(100);
                    file_out.close();
                    log("To complete the encryption");
                }
                file.close();
            }else log("Unable to complete encryption");
        }
    }
}

void MainWindow::on_btn_inputFileName_clicked()
{
    this->inputFileName = QFileDialog::getOpenFileName(this,"load the source","","mpeg4 file(*.mp4)");
    ui->le_inputFileName->setText(this->inputFileName);
    if(inputFileName != "")log("load FileName:" + inputFileName);
}

void MainWindow::on_btn_outputFileName_clicked()
{
    this->outputFileName = QFileDialog::getSaveFileName(this,"save as ...","","mpeg4 file(*.mp4)");
    ui->le_outputFileName->setText(this->outputFileName);
    if(outputFileName != "")log("Save as " + outputFileName);
}

void MainWindow::on_btn_convert_clicked()
{
    log("initialize>>>");
    this->arguments.clear();
    this->arguments << "-i" << inputFileName;
    cmd_start = "get";
    cmd_get_echo = "";
    this->transcodingProcess->setProcessChannelMode(QProcess::MergedChannels);
    this->transcodingProcess->start(ffmpegPath,this->arguments);
}

void MainWindow::on_btn_cancel_clicked()
{
    cmd_start = "get";
    ui->btn_convert->setEnabled(true);
    ui->btn_start->setEnabled(false);
    ui->btn_cancel->setEnabled(false);
    ui->rb_lr->setEnabled(true);
    ui->rb_pvr->setEnabled(true);
    ui->cb_purevideo->setEnabled(true);
    ui->progressBar->setValue(0);

    this->outputFileName = "";
    this->transcodingProcess->close();

    ui->ql_progress->setText("progress:0%");
    ui->ql_remainingTime->setText("remainingTime:00:00:00");
    ui->ql_time->setText("time:00:00:00");
    ui->ql_speed->setText("speed:--");
    ui->ql_bitrate->setText("bitrate:--");
    ui->ql_frame->setText("frame:--");
    ui->ql_fps->setText("fps:--");
    log("-------------END--------------");
}

void MainWindow::on_btn_start_clicked()
{
   if(cmd_start == "set"){
       log("start to work>>>");
       ui->btn_convert->setEnabled(false);
       ui->btn_start->setEnabled(false);
       ui->btn_cancel->setEnabled(true);

       QFile file(outputFileName);
       if(file.exists())file.remove();

       this->transcodingProcess->setProcessChannelMode(QProcess::MergedChannels);
       this->transcodingProcess->start(ffmpegPath,this->arguments);
   }
}

void MainWindow::on_btn_encrypt_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,"Files that need to be encrypted.","","All files(*.*)");
    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly)){
        log("Began to encrypt");
        qint64 file_size = file.size();
        qint64 file_poit = 0;
        qint64 write_size = 1024;
        char more[32] = {0x00,0x00,0x00,0x00,0x00,0x56,0x25,0x30,
                         0x52,0x00,0x02,0x55,0x58,0x4a,0x68,0x72,
                         0x55,0x11,0x02,0x08,0x01,0x00,0x31,0x32,
                         0x58,0x68,0x66,0x58,0x12,0x12,0x35,0x36};
        QString newFilePath = filePath.mid(0,filePath.lastIndexOf(".")) + ".pvr";
        QFile file_out(newFilePath);
        if(file_out.open(QIODevice::ReadWrite)){
            log("Encryption for " + newFilePath);
            file_out.write(more,sizeof(more));
            do{
                if(file_poit + write_size > file_size)write_size = file_size - file_poit;
                file.seek(file_poit);
                file_out.write(file.read(write_size),write_size);
                file_poit += write_size;
                ui->progressBar->setValue((int)(file_poit / file_size * 100));
            }while(file_poit < file_size);
            ui->progressBar->setValue(100);
            file_out.close();
            log("To complete the encryption");
        }
        file.close();
    }else log("Unable to complete encryption");
}

void MainWindow::on_but_decode_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,"Files to be decrypted.","","PVR files(*.pvr)");
    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly)){
        log("Began to decrypt");
        qint64 file_size = file.size();
        qint64 file_poit = 0;
        qint64 write_size = 1024;
        char more[32] = {0x00,0x00,0x00,0x00,0x00,0x56,0x25,0x30,
                         0x52,0x00,0x02,0x55,0x58,0x4a,0x68,0x72,
                         0x55,0x11,0x02,0x08,0x01,0x00,0x31,0x32,
                         0x58,0x68,0x66,0x58,0x12,0x12,0x35,0x36};
        file_poit = sizeof(more);
        QString newFilePath = filePath.mid(0,filePath.lastIndexOf(".")) + ".mp4";
        QFile file_out(newFilePath);
        if(file_out.open(QIODevice::ReadWrite)){
            log("Decryption for " + newFilePath);
            do{
                if(file_poit + write_size > file_size)write_size = file_size - file_poit;
                file.seek(file_poit);
                file_out.write(file.read(write_size),write_size);
                file_poit += write_size;
                ui->progressBar->setValue((int)(file_poit / file_size * 100));
            }while(file_poit < file_size);
            ui->progressBar->setValue(100);
            file_out.close();
            log("Complete the decryption");
        }
        file.close();
    }else log("Unable to complete decryption");
}

void MainWindow::on_vs_contrast_valueChanged(int value)
{
    ui->dsb_contrast->setValue(value / 10.00);
}

void MainWindow::on_vs_brightness_valueChanged(int value)
{
    ui->dsb_brightness->setValue(value / 10.00);
}

void MainWindow::on_vs_saturation_valueChanged(int value)
{
    ui->dsb_saturation->setValue(value / 10.00);
}

void MainWindow::on_dsb_contrast_valueChanged(double arg1)
{
    ui->vs_contrast->setValue(arg1 * 10);
}

void MainWindow::on_dsb_brightness_valueChanged(double arg1)
{
    ui->vs_brightness->setValue(arg1 * 10);
}

void MainWindow::on_dsb_saturation_valueChanged(double arg1)
{
    ui->vs_saturation->setValue(arg1 * 10);
}

void MainWindow::on_but_eq_default_clicked()
{
    ui->vs_contrast->setValue(10);
    ui->vs_brightness->setValue(0);
    ui->vs_saturation->setValue(10);
    ui->dsb_contrast->setValue(1);
    ui->dsb_brightness->setValue(0);
    ui->dsb_saturation->setValue(1);
}
