#ifndef AUTH_H
#define AUTH_H

#include <QDialog>
#include <QString>

namespace Ui {
class Auth;
}

class Auth : public QDialog
{
    Q_OBJECT

public:
    explicit Auth(QWidget *parent = nullptr);
    ~Auth();
    QString username;
    void setUsername(QString);
    QString getUsername();

private slots:
    void on_submit_clicked();

private:
    Ui::Auth *ui;
};

#endif // AUTH_H
