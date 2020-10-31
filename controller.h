#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QJsonDocument>
#include <QString>
#include <QAbstractSocket>


class Controller
{
private:
    QAbstractSocket *sock;
    void SendJson(QJsonDocument doc);
public:
    Controller(QAbstractSocket *sock);
    void RunCommand(QString cmd);
    void LoadGCode(QString gcode);
    void Start();
    void Stop();
    void Continue();
    void Reset();
    void Configure(QJsonObject doc);
};

#endif // CONTROLLER_H
