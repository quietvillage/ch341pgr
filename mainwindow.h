#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ch341interface.h"
#include "croppingdialog.h"
#include <QMainWindow>
#include <QTranslator>
#include <unistd.h>

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
    QString homeDir() {return m_homeDir;}
    void setHomeDir(const QString &dir) {m_homeDir = dir;}
    void setUid(uid_t id) {m_uid = id;}
    void setGid(uid_t id) {m_gid = id;}

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

    void onPortChanged(int index);

    void on_action_select_zh_CN_triggered();

    void on_action_select_en_triggered();

    void on_action_cropping_triggered();
    void onChopRequest(char); //删除尾部特定值
    void onCroppingRequest(int, int);
    void onModelChanged(int index);


private:
    int chipSize();
    void initModel();
    void blockActions(bool block);

    Ui::MainWindow *ui;
    CroppingDialog *m_croppingDialog;
    QTranslator m_translator;
    Ch341Interface m_port;
    QString m_file;
    QString m_homeDir;
    QList<QPair<int, int>> m_portList;
    uid_t m_uid; //user id
    uid_t m_gid; //group id
};
#endif // MAINWINDOW_H
