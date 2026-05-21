#include <QApplication>
#include <QCommandLineParser>
#include "mainwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("SecureSQL Client");
    parser.addHelpOption();
    parser.addOption({{"H", "host"}, "Server host", "host", "127.0.0.1"});
    parser.addOption({{"p", "port"}, "Server port", "port", "54321"});
    parser.process(app);
    QString host = parser.value("host");
    int port = parser.value("port").toInt();
    if (port <= 0) port = 54321;
    
    MainWindow w(host, port);
    w.resize(500, 400);
    w.show();
    return app.exec();
}