#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QMessageBox>
#include <QDebug>
#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void processStarted();

    void processFinished();

    void readyReadStandardOutput();

    QString get_cutterStr(QString startStr,QString endStr);

    QString set_cutterStr(QString startStr,QString endStr);

    void log(QString info);

    void on_btn_inputFileName_clicked();

    void on_btn_outputFileName_clicked();

    void on_btn_convert_clicked();

    void on_btn_cancel_clicked();

    void on_btn_start_clicked();

    void on_btn_encrypt_clicked();

    void on_but_decode_clicked();

    void on_vs_contrast_valueChanged(int value);

    void on_vs_brightness_valueChanged(int value);

    void on_vs_saturation_valueChanged(int value);

    void on_dsb_contrast_valueChanged(double arg1);

    void on_dsb_brightness_valueChanged(double arg1);

    void on_dsb_saturation_valueChanged(double arg1);

    void on_but_eq_default_clicked();

private:
    Ui::MainWindow *ui;
    QString inputFileName,outputFileName;
    QString ffmpegCmd;
    QStringList arguments;
    QProcess *transcodingProcess;

    QString duration,bitrate,video,yuv,px,fps,audio;
    double frame,timelen;
};

#endif // MAINWINDOW_H
