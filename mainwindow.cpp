// mainwindow.cpp

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "appprotocol.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QFileDialog>
#include <QString>
#include <QVBoxLayout>
#include <QMessageBox>
#include <fstream>
#include <vector>
#include <string>
#include <random>

std::string random_digits_10() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(0, 9);

    std::string s;
    s.reserve(10);

    for (int i = 0; i < 10; ++i) {
        s.push_back('0' + dist(gen));
    }
    return s;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_connection("72.56.70.125", "8080", 1024)//"72.56.70.125"
{
    ui->setupUi(this);

    connect(this, &MainWindow::message, this, &MainWindow::messageReciever);

    connect(ui->sendMessage, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);

    connect(ui->messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendButtonClicked);

    QWidget *container = new QWidget();
    chatLayout = new QVBoxLayout(container);
    chatLayout->setContentsMargins(10, 10, 10, 10);
    chatLayout->setSpacing(4);
    QSpacerItem *bottomSpacer = new QSpacerItem(
        0, 0,
        QSizePolicy::Minimum,
        QSizePolicy::Expanding
        );

    chatLayout->addItem(bottomSpacer);

    ui->chatView->setWidget(container);

    if (m_connection.isConnected()) {
        m_running = true;
        m_receiverThread = std::thread(&MainWindow::receiveLoop, this);
    } else {
        qDebug() << "Failed to connect";
    }

    auto bar = ui->chatView->verticalScrollBar();
    barPrevMaxPosition = bar->maximum();
    connect(bar, &QScrollBar::rangeChanged, this,
        [this, bar](int /*min*/, int /*max*/) {
        if(bar->value() == barPrevMaxPosition){
            bar->setValue(bar->maximum());
            barPrevMaxPosition = bar->maximum();
        }
        barPrevMaxPosition = bar->maximum();
    });

    connect(ui->listWidget, &QListWidget::itemSelectionChanged, this, [this] {
        if (ui->listWidget->selectedItems().size() == 0) {
            ui->messageInput->setEnabled(false);
            ui->sendMessage->setEnabled(false);
            ui->sendFile->setEnabled(false);
            ui->messageInput->setText("");
        }
    });

    connect(this, &MainWindow::fileDownloaded, this,
            [](QString path){
                QMessageBox::information(nullptr,
                                         "Файл загружен",
                                         "Файл успешно сохранён:\n" + path);
            });

    ui->listWidget->addItem(QString::fromStdString("Broadcast"));
}

MessageInfo MainWindow::getMessageInfo(std::string reply){
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

    MessageInfo result;
    result.author = author;
    result.msg = answer;

    return result;
}

void MainWindow::setUsername(QString val)
{
    username = val;
    AppProtocol data(2, 0, username.toStdString());
    std::vector<uint8_t> packet = data.getCode();
    m_connection.sendMessage(packet);
}

void MainWindow::addMessage(const QString &author, const QString &text, bool fromMe, bool isLoaded, int type)
{
    if (!chatLayout)
        return;

    QWidget *msgWidget = new QWidget();
    QHBoxLayout *h = new QHBoxLayout(msgWidget);
    QVBoxLayout *v = new QVBoxLayout(nullptr);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);

    std::string title;
    if (author.toStdString()[0] == '$'){
        title = author.toStdString().substr(1, -1);
    }
    else{
        title = author.toStdString();
    }

    QLabel *authorView = new QLabel(QString::fromStdString(title));

    if (type != 5){
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
            if (author.toStdString() == recieverName || recieverName == "Broadcast"){
                v->addWidget(authorView);
                v->addWidget(label);
                h->addLayout(v);
                h->addStretch();
            }
        }
    }
    else{
        QPushButton* btn = new QPushButton(text);
        connect(btn, &QPushButton::clicked, this, [this, btn, text]{
            getFile(text.toStdString(), btn);
        });
        btn->setMaximumWidth(600);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        msgWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        btn->setStyleSheet("padding:6px 10px;"
                             "border-radius:8px;"
                             "text-decoration: underline;"
                             "font-weight: bold;"
                             "background:"
                             + QString(fromMe ? "#6a1b9a" : "#424242")
                             + ";"
                               "color:white;");

        if (fromMe) {
            h->addStretch();
            v->addWidget(authorView);
            v->addWidget(btn);
            h->addLayout(v);
        } else {
            if (author.toStdString() == recieverName || recieverName == "Broadcast"){
                v->addWidget(authorView);
                v->addWidget(btn);
                h->addLayout(v);
                h->addStretch();
            }
        }
    }

    if (fromMe || author.toStdString() == recieverName){
        chatLayout->addWidget(msgWidget);
    }

    else{
        if (fromMe || (recieverName == "Broadcast" && author.toStdString()[0] == '$')){
            chatLayout->addWidget(msgWidget);
        }
    }

    if (!isLoaded) {
        if (author.toStdString()[0] != '$'){
            HistoryMsg msg;
            msg.msg.author = author.toStdString();
            msg.msg.msg = text.toStdString();
            msg.type = type;
            histories[author.toStdString() == "You" ? recieverName : author.toStdString()].push_back(
                msg
            );
        }
        else{
            HistoryMsg msg;
            msg.msg.author = author.toStdString();
            msg.msg.msg = text.toStdString();
            msg.type = type;
            histories["Broadcast"].push_back(
                msg
            );
        }

    }
}

void MainWindow::onSendButtonClicked()
{
    QString text = ui->messageInput->text().trimmed();

    if (text.isEmpty())
        return;

    const QString me("You");

    std::string input = text.toStdString();
    std::vector<std::string> fragments = m_connection.messageFragmentation(input, FRAGMENT_SIZE);

    for (std::string fragment : fragments) {
        addMessage(me, QString::fromStdString(fragment), true, false, 1);

        if (recieverName == "Broadcast"){
            AppProtocol data(1, 0, fragment);
            std::vector<uint8_t> packet = data.getCode();
            m_connection.sendMessage(packet);
        }
        else{
            AppProtocol data(1, m_connection.clientsEnumeration[recieverName], fragment);
            std::vector<uint8_t> packet = data.getCode();
            m_connection.sendMessage(packet);
        }
    }

    ui->messageInput->clear();
}

void MainWindow::receiveLoop()
{
    while (m_running && m_connection.isConnected()) {
        Msg reply = m_connection.receiveMessage();

        MessageInfo info = getMessageInfo(reply.msg);

        if (!reply.msg.empty()) {
            if (reply.msg == "clientSYNC"){
                ui->listWidget->clear();
                for (auto const& it:m_connection.clientsEnumeration){
                    ui->listWidget->addItem(QString::fromStdString(it.first));
                }
                ui->listWidget->addItem(QString::fromStdString("Broadcast"));
            }
            else{
                if (reply.type == 10){
                    currentFile = new std::ofstream(requestedFilePath, std::ios::binary);
                }
                if (reply.type == 11){
                    currentFile->write(reply.msg.data(), static_cast<std::streamsize>(reply.msg.size()));;
                }
                if (reply.type == 12) {
                    if (currentFile) {
                        currentFile->close();
                        delete currentFile;
                        currentFile = nullptr;
                        currentDownloadingFile->setText(btnOriginalName);
                    }

                    emit fileDownloaded(QString::fromStdString(requestedFilePath));
                }
                else
                emit message(QString::fromStdString(info.author), QString::fromStdString(info.msg), reply.type);
            }
        }
    }
}

void MainWindow::messageReciever(const QString &author, const QString &msg, const int type)
{
    if (type != 3 && type != 4 && type != 10 && type != 11 && type != 12)
    addMessage(author, msg, (author.toStdString()=="You" || author.toStdString()=="$You"), false, type);
}

void MainWindow::getFile(std::string filename, QPushButton* btn)
{
    if (currentFile != nullptr) return;

    currentDownloadingFile = btn;
    btnOriginalName = currentDownloadingFile->text();

    currentDownloadingFile->setText(btnOriginalName + " скачивается...");

    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Куда сохранить файл?",
        QString::fromStdString(filename)
        );

    if (savePath.isEmpty())
        return;

    requestedFilePath = savePath.toStdString();

    AppProtocol request(6, 0, filename);
    std::vector<uint8_t> packet = request.getCode();
    m_connection.sendMessage(packet);
}

MainWindow::~MainWindow()
{
    m_running = false;

    m_connection.closeConnection();

    if (m_receiverThread.joinable())
        m_receiverThread.join();

    if (m_fileThread.joinable())
        m_fileThread.join();

    delete ui;
}

void MainWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
    if (recieverName != item->text().toStdString()){
        recieverName = item->text().toStdString();
        ui->messageInput->setEnabled(true);
        ui->sendMessage->setEnabled(true);
        ui->sendFile->setEnabled(true);
        QWidget* container = ui->chatView->widget();
        QVBoxLayout* messages = container->findChild<QVBoxLayout *>();
        QLayoutItem* message;
        while ((message = messages->takeAt(0)) != nullptr) {
            if (QWidget* widget = message->widget()) {
                delete widget;
            }
            delete message;
        }

        messages->addItem(new QSpacerItem(
            0, 0,
            QSizePolicy::Minimum,
            QSizePolicy::Expanding
        ));

        for (HistoryMsg msg: histories[recieverName]){
            addMessage(
                QString::fromStdString(msg.msg.author),
                QString::fromStdString(msg.msg.msg),
                msg.msg.author == "You",
                true,
                msg.type
            );
        }
    }
}

void MainWindow::sendFile(QString filename, QString prefix){
    ui->sendFile->setEnabled(false);
    ui->sendMessage->setEnabled(false);
    m_connection.sendFile(filename, prefix, recieverName == "Broadcast" ? 0 : m_connection.clientsEnumeration[recieverName], 4096);
    ui->sendFile->setEnabled(true);
    ui->sendMessage->setEnabled(true);
}


void MainWindow::on_sendFile_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Выберите файл",
        "",
        "Все файлы (*.*)"
        );

    if (filename.isEmpty()) {
        return;
    }

    QString prefix = QString::fromStdString(random_digits_10());

    if (m_fileThread.joinable()) {
        m_fileThread.join();
    }

    m_fileThread = std::thread(&MainWindow::sendFile, this, filename, prefix);

    QFileInfo fileInfo(filename);
    addMessage("You", prefix + '_' + fileInfo.fileName(), true, false, 5);
}

