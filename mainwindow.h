// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <thread>
#include <atomic>
#include "tcpconnection.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setUsername(QString);

signals:
    void message(const QString &author, const QString &msg);

private slots:
    void messageReciever(const QString &author, const QString &msg);
    void onSendButtonClicked();

private:
    void receiveLoop();
    void addMessage(const QString &author, const QString &text, bool fromMe);

    Ui::MainWindow *ui;
    QVBoxLayout *chatLayout = nullptr;
    QString username;

    int barPrevMaxPosition;

    TCPConnection  m_connection;
    std::thread    m_receiverThread;
    std::atomic<bool> m_running{false};
};

#endif // MAINWINDOW_H
