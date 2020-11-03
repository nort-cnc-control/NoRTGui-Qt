#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("NoRTGui-Qt");
    QCoreApplication::setApplicationVersion("1.0");

    QString addr = "127.0.0.1";
    int port = 8888;
    QString configdir;


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

    QCommandLineOption config("c", QCoreApplication::translate("main", "NoRT config dir"));
    config.setValueName("config");
    QString cfgdir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir cfg(cfgdir);
    config.setDefaultValue(cfg.filePath("NoRT"));
    parser.addOption(config);

    parser.process(a);

    addr = parser.value(remoteAddr);
    port = parser.value(remotePort).toInt();
    configdir = parser.value(config);

    MainWindow w(addr, port, configdir);
    w.show();
    return a.exec();
}
