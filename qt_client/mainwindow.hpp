#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QTcpSocket>

class QTextEdit;
class QPushButton;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(const QString& host, int port, QWidget *parent = nullptr);
private slots:
    void sendSQL();
    void readResponse();
    void onDisconnected();
private:
    QLabel *statusLabel;
    QTextEdit *inputEdit;
    QTextEdit *outputEdit;
    QTcpSocket *socket;
    QString serverHost;
    int serverPort;
    bool isConnected;
};

#endif