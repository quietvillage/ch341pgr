#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ch341interface.h"
#include "croppingdialog.h"
#include <QMainWindow>
#include <QTranslator>

 #if defined (Q_OS_LINUX)
#include <unistd.h>
#endif

#define LANGUAGE_zh_CN  0
#define LANGUAGE_en_US  1

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
    void loadSettings();

 #if defined (Q_OS_LINUX)
    QString homeDir() {return m_homeDir;}
    void setHomeDir(const QString &dir) {m_homeDir = dir;}
    void setUid(uid_t id) {m_uid = id;}
    void setGid(uid_t id) {m_gid = id;}
#endif

protected:
    void resizeEvent(QResizeEvent *e);
    bool eventFilter(QObject *sender, QEvent *e);

private slots:
    void saveSettings();
    void on_action_open_triggered();

    void on_action_save_triggered();

    void on_action_quit_triggered();

    void on_btn_flush_clicked();
    
    void on_btn_cancel_clicked();

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
    int64_t chipSize();
    void initModel();
    void blockActions(bool block);

    Ui::MainWindow *ui;
    CroppingDialog *m_croppingDialog;
    QTranslator m_translator;
    Ch341Interface m_port;
    QString m_file;
    QString m_homeDir;
    QList<QPair<int, int>> m_portList; //<bus, port>
    int m_language;

#if defined (Q_OS_LINUX)
    uid_t m_uid; //user id
    uid_t m_gid; //group id
#endif
    
    bool m_cancel;
};
#endif // MAINWINDOW_H
