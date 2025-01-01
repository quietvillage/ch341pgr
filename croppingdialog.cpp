#include "croppingdialog.h"
#include "ui_croppingdialog.h"

CroppingDialog::CroppingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CroppingDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
}

CroppingDialog::~CroppingDialog()
{
    delete ui;
}

void CroppingDialog::setStartIndex(int n)
{
    ui->spinBox_start->setValue(n);
}

void CroppingDialog::setEndIndex(int n)
{
    ui->spinBox_end->setValue(n);
}

void CroppingDialog::setMaximum(int n)
{
    ui->spinBox_start->setMaximum(n);
    ui->spinBox_end->setMaximum(n);
}

void CroppingDialog::on_btn_chop_clicked()
{
    char ch = ui->spinBox_chop->value();
    emit chop(ch);
}


void CroppingDialog::on_btn_ok_clicked()
{
    emit cropping(ui->spinBox_start->value(), ui->spinBox_end->value());
    this->hide();
}

