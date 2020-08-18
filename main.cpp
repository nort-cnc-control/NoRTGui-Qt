#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("NoRTGui-Qt");
    QCoreApplication::setApplicationVersion("1.0");

    QString addr = "127.0.0.1";
    int port = 8888;

    QCommandLineParser parser;
    parser.setApplicationDescription("NoRT Gui");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption remoteAddr("r", QCoreApplication::translate("main", "NoRT Server address"));
    remoteAddr.setValueName("address");
    remoteAddr.setDefaultValue(addr);
    parser.addOption(remoteAddr);

    QCommandLineOption remotePort("p", QCoreApplication::translate("main", "NoRT Server port"));
    remotePort.setValueName("port");
    remotePort.setDefaultValue(QString::number(port));
    parser.addOption(remotePort);

    parser.process(a);

    addr = parser.value(remoteAddr);
    port = parser.value(remotePort).toInt();

    MainWindow w(addr, port);
    w.show();
    return a.exec();
}
