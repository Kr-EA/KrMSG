#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include "tcpconnection.h"
#include <atomic>
#include <map>
#include <thread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct MessageInfo{
    std::string author;
    std::string msg;
};

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

    void on_listWidget_itemClicked(QListWidgetItem *item);

private:
    void receiveLoop();
    void addMessage(const QString &author, const QString &text, bool fromMe, bool isLoaded);
    void loadMessages(std::string user);

    Ui::MainWindow *ui;
    QVBoxLayout *chatLayout = nullptr;
    QString username;

    int barPrevMaxPosition;

    MessageInfo getMessageInfo(std::string);

    TCPConnection  m_connection;
    std::string recieverName;
    std::map<std::string, std::vector<std::string>> histories;
    std::thread    m_receiverThread;
    std::atomic<bool> m_running{false};
};

#endif // MAINWINDOW_H
