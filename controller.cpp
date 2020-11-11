#include "controller.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

void Controller::SendJson(QJsonDocument doc)
{
    QByteArray json = doc.toJson(QJsonDocument::JsonFormat::Compact);
    int jsonlen = json.size();
    QByteArray lenarr = QString::number(jsonlen).toUtf8();
    QByteArray array = lenarr + QByteArray(";") + json + QByteArray(";");

    sock->write(array);
}

Controller::Controller(QAbstractSocket *sock)
{
    this->sock = sock;
}

void Controller::RunCommand(QString cmd)
{
    /*
        {
            "type": "command",
            "command": "execute",
            "program": "G0 X1"
        }
     */

    QJsonObject obj;
    obj.insert("type", "command");
    obj.insert("command", "execute");
    obj.insert("program", cmd);

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::LoadGCode(QString gcode)
{
    /*
        {
            "type": "command",
            "command": "load",
            "program": [...]
        }
     */

    QStringList lines = gcode.split("\n");
    QJsonObject obj;
    obj.insert("type", "command");
    obj.insert("command", "load");

    QJsonArray ls;
    for (auto line : lines)
    {
        ls.append(line.trimmed());
    }
    obj.insert("program", ls);

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::Start()
{
    /*
        {
           "type" : "command",
           "command" : "start"
        }
    */

    QJsonObject obj;
    obj.insert("type", "command");
    obj.insert("command", "start");

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::Stop()
{
    /*
        {
           "type" : "command",
           "command" : "stop"
        }
    */

    QJsonObject obj;
    obj.insert("type", "command");
    obj.insert("command", "stop");

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::Continue()
{
    /*
        {
           "type" : "command",
           "command" : "continue"
        }
    */

    QJsonObject obj;
    obj.insert("type", "command");
    obj.insert("command", "continue");

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::Reset()
{
    /*
        {
           "type" : "command",
           "command" : "reset"
        }
    */

    QJsonObject obj;
    obj.insert("type", "command");
    obj.insert("command", "reset");

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::Configure(QJsonObject cfg)
{
    /*
        {
            "type" : "configuration",
            "configuration" : {
                ...
            }
        }
    */

    QJsonObject obj;
    obj.insert("type", "configuration");
    obj.insert("configuration", cfg);

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::StartManualMovement()
{
    QJsonObject obj;

    obj.insert("type", "mode_selection");
    obj.insert("mode", "manual");

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::StopManualMovement()
{
    QJsonObject obj;
    obj.insert("type", "mode_selection");
    obj.insert("mode", "gcode");

    QJsonDocument doc(obj);
    SendJson(doc);
}

void Controller::ManualMovementFeed(double fx, double fy, double fz)
{
    QJsonObject obj;
    obj.insert("type", "command");
    obj.insert("command", "manual_feed");

    QJsonObject feeds;
    feeds.insert("x", fx);
    feeds.insert("y", fy);
    feeds.insert("z", fz);

    obj.insert("feed", feeds);

    QJsonDocument doc(obj);
    SendJson(doc);
}
