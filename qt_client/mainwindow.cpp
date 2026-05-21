#include "mainwindow.hpp"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(const QString& host, int port, QWidget *parent)
    : QMainWindow(parent), serverHost(host), serverPort(port), isConnected(false)
{
    setWindowTitle("SecureSQL Client");
    auto *central = new QWidget;
    setCentralWidget(central);
    auto *layout = new QVBoxLayout(central);
    
    statusLabel = new QLabel("Ready");
    layout->addWidget(statusLabel);
    
    inputEdit = new QTextEdit;
    inputEdit->setPlaceholderText("Enter SQL statement...");
    layout->addWidget(inputEdit);
    
    auto *sendBtn = new QPushButton("Send");
    layout->addWidget(sendBtn);
    
    outputEdit = new QTextEdit;
    outputEdit->setReadOnly(true);
    layout->addWidget(outputEdit);
    
    connect(sendBtn, &QPushButton::clicked, this, &MainWindow::sendSQL);
    
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readResponse);
}

void MainWindow::onDisconnected() {
    isConnected = false;
    statusLabel->setText("Ready");
}

void MainWindow::sendSQL() {
    QString sql = inputEdit->toPlainText().trimmed();
    if (sql.isEmpty()) return;
    
    if (!isConnected) {
        socket->connectToHost(serverHost, serverPort);
        if (!socket->waitForConnected(2000)) {
            QMessageBox::warning(this, "Error", 
                QString("Cannot connect to %1:%2. Is server running?").arg(serverHost).arg(serverPort));
            return;
        }
        isConnected = true;
        statusLabel->setText("Connected");
    }
    socket->write(sql.toUtf8() + "\n");
    outputEdit->append(">>> " + sql);
    inputEdit->clear();
}

void MainWindow::readResponse() {
    QByteArray data = socket->readAll();
    QString text = QString::fromUtf8(data);
    outputEdit->append("<<< " + text);
}