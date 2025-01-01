#ifndef CROPPINGDIALOG_H
#define CROPPINGDIALOG_H

#include <QDialog>

namespace Ui {
class CroppingDialog;
}

class CroppingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CroppingDialog(QWidget *parent = nullptr);
    ~CroppingDialog();

    void setStartIndex(int);
    void setEndIndex(int);
    void setMaximum(int);

signals:
    void chop(char);
    void cropping(int, int);

private slots:
    void on_btn_chop_clicked();

    void on_btn_ok_clicked();

private:
    Ui::CroppingDialog *ui;
};

#endif // CROPPINGDIALOG_H
