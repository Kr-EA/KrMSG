#include "auth.h"
#include <QDebug>
#include <QString>
#include "ui_auth.h"

Auth::Auth(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Auth)
{
    ui->setupUi(this);
}

Auth::~Auth()
{
    delete ui;
}

void Auth::setUsername(QString val)
{
    username = val;
}

QString Auth::getUsername()
{
    return username;
}

void Auth::on_submit_clicked()
{
    QString input = ui->username->text();
    if (input.size() > 3 && input.indexOf(":") == -1) {
        setUsername(input);
        this->close();
    }
}
