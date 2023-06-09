#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ch341interface.h"
#include <QMainWindow>
#include <QTranslator>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void onloaded();

protected:
    void resizeEvent(QResizeEvent *e);

private slots:
    void on_action_open_triggered();

    void on_action_save_triggered();

    void on_action_quit_triggered();

    void on_btn_flush_clicked();

    void on_btn_erase_clicked();

    void on_btn_checkEmpty_clicked();

    void on_btn_read_clicked();

    void on_btn_write_clicked();

    void on_btn_check_clicked();

    void onMemTypeChanged(int index);

    void on_btn_detect_clicked();

    void on_btn_reset_clicked();

    void onPortChanged(const QString &port);

    void on_action_select_zh_CN_triggered();

    void on_action_select_en_triggered();

private:
    int chipSize();
    void initModel();
    void displayBufferData();
    void blockActions(bool block);

    Ui::MainWindow *ui;
    QTranslator m_translator;
    Ch341Interface m_port;
    QByteArray m_buffer; //缓存数据
    QString m_file;
};
#endif // MAINWINDOW_H
