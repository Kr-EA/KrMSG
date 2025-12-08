
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include "tcpconnection.h"
#include <atomic>
#include <map>
#include <fstream>
#include <thread>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct MessageInfo{
    std::string author;
    std::string msg;
};

struct HistoryMsg{
    int type;
    MessageInfo msg;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setUsername(QString);

signals:
    void message(const QString &author, const QString &msg, const int);
    void fileDownloaded(QString path);

private slots:
    void messageReciever(const QString &author, const QString &msg, const int);
    void onSendButtonClicked();

    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_sendFile_clicked();

private:
    void receiveLoop();
    void addMessage(const QString &author, const QString &text, bool fromMe, bool isLoaded, int type);
    void loadMessages(std::string user);
    void getFile(std::string, QPushButton*);

    const int FRAGMENT_SIZE = 256;

    void sendFile(QString filename, QString prefix);

    Ui::MainWindow *ui;
    QVBoxLayout *chatLayout = nullptr;
    QPushButton *currentDownloadingFile = nullptr;
    QString username;
    QString btnOriginalName;

    int barPrevMaxPosition;

    MessageInfo getMessageInfo(std::string);

    TCPConnection  m_connection;
    std::string recieverName;
    std::string requestedFilePath;
    std::ofstream* currentFile = nullptr;
    std::map<std::string, std::vector<HistoryMsg>> histories;
    std::thread    m_receiverThread;
    std::thread    m_fileThread;
    std::atomic<bool> m_running{false};
};

#endif // MAINWINDOW_H
