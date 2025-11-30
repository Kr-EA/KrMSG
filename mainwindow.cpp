// mainwindow.cpp

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "appprotocol.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QString>
#include <QVBoxLayout>
#include <vector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_connection("127.0.0.1", "8080", 1024)
{
    ui->setupUi(this);

    connect(this, &MainWindow::message, this, &MainWindow::messageReciever);

    connect(ui->sendMessage, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);

    connect(ui->messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendButtonClicked);

    QWidget *container = new QWidget();
    chatLayout = new QVBoxLayout(container);
    chatLayout->setContentsMargins(10, 10, 10, 10);
    chatLayout->setSpacing(4);
    chatLayout->addStretch(1);

    ui->chatView->setWidget(container);
    ui->chatView->setWidgetResizable(true);

    if (m_connection.isConnected()) {
        m_running = true;
        m_receiverThread = std::thread(&MainWindow::receiveLoop, this);
    } else {
        qDebug() << "Failed to connect";
    }

    auto bar = ui->chatView->verticalScrollBar();
    connect(bar, &QScrollBar::rangeChanged, this, [bar](int /*min*/, int /*max*/) {
        bar->setValue(bar->maximum());
    });
}

void MainWindow::setUsername(QString val)
{
    username = val;
    AppProtocol data(2, username.toStdString());
    std::vector<uint8_t> packet = data.getCode();
    m_connection.sendMessage(packet);
}

void MainWindow::addMessage(const QString &author, const QString &text, bool fromMe)
{
    if (!chatLayout)
        return;

    QWidget *msgWidget = new QWidget();
    QHBoxLayout *h = new QHBoxLayout(msgWidget);
    QVBoxLayout *v = new QVBoxLayout(nullptr);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);

    QLabel *authorView = new QLabel(author);

    QLabel *label = new QLabel(text);
    label->setWordWrap(true);
    label->setMaximumWidth(600);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    msgWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    label->setStyleSheet("padding:6px 10px;"
                         "border-radius:8px;"
                         "background:"
                         + QString(fromMe ? "#6a1b9a" : "#424242")
                         + ";"
                           "color:white;");

    if (fromMe) {
        h->addStretch();
        v->addWidget(authorView);
        v->addWidget(label);
        h->addLayout(v);
    } else {
        v->addWidget(authorView);
        v->addWidget(label);
        h->addLayout(v);
        h->addStretch();
    }

    int insertPos = chatLayout->count() - 1;
    chatLayout->insertWidget(insertPos, msgWidget);
}

void MainWindow::onSendButtonClicked()
{
    QString text = ui->messageInput->text().trimmed();

    if (text.isEmpty())
        return;

    const QString me("You");

    std::string input = text.toStdString();
    std::vector<std::string> fragments = m_connection.messageFragmentation(input, 256);

    for (std::string fragment : fragments) {
        addMessage(me, QString::fromStdString(fragment), true);

        AppProtocol data(1, fragment);
        std::vector<uint8_t> packet = data.getCode();
        m_connection.sendMessage(packet);
    }

    ui->messageInput->clear();
}

void MainWindow::receiveLoop()
{
    while (m_running && m_connection.isConnected()) {
        std::string reply = m_connection.receiveMessage();

        std::string author{};
        std::string answer{};

        int flag = 0;

        for (int i = 0; i < reply.size(); i++) {
            if (reply[i] == ':') {
                flag = 1;
                continue;
            }
            if (flag == 0) {
                author += reply[i];
            } else {
                answer += reply[i];
            }
        }

        if (!reply.empty()) {
            emit message(QString::fromStdString(author), QString::fromStdString(answer));
        }
    }
}

void MainWindow::messageReciever(const QString &author, const QString &msg)
{
    addMessage(author, msg, false);
}

MainWindow::~MainWindow()
{
    m_running = false;

    if (m_receiverThread.joinable())
        m_receiverThread.join();

    m_connection.closeConnection();

    delete ui;
}
