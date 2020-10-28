#include "receiver.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

void Receiver::HandleState(QByteArray frame)
{
    QJsonDocument doc = QJsonDocument::fromJson(frame);

    QString type = doc["type"].toString();
    if (type == "machine_config")
    {
        QJsonObject tools = doc["tools"].toObject();
        dsp->ClearTools();
        for (QString key : tools.keys())
        {
            Tool toolDesc;
            QJsonObject tool = tools[key].toObject();

            QString type = tool["type"].toString();
            QString name = tool["name"].toString();
            QString driver = tool["driver"].toString();

            toolDesc.name = name;
            if (driver == "dummy")
                toolDesc.driver = driver_dummy;
            else if (driver == "n700e")
                toolDesc.driver = driver_n700e;
            else if (driver == "gpio")
                toolDesc.driver = driver_gpio;
            else if (driver == "modbus")
                toolDesc.driver = driver_modbus;

            if (type == "null")
                toolDesc.type = type_none;
            else if (type == "spindle")
                toolDesc.type = type_spindle;
            else if (type == "binary")
                toolDesc.type = type_binary;

            int id = key.toInt();
            switch (toolDesc.type)
            {
                case type_none:
                {
                    dsp->AddNoneTool(id, toolDesc.name, driver);
                    break;
                }
                case type_binary:
                {
                    dsp->AddBinaryTool(id, toolDesc.name, driver);
                    break;
                }
                case type_spindle:
                {
                    dsp->AddSpindleTool(id, toolDesc.name, driver);
                    break;
                }
            }

            this->tools.insert(id, toolDesc);
        }
    }
    else if (type == "machine_state")
    {
        /* HW State frame
            {
                'coordinates': {
                    'cs': 'G53',
                    'global': [0, 0, 0],
                    'hardware': [0, 0, 0],
                    'local': [0, 0, 0]
                },
                'endstops': {
                    'axes': [False, False, False],
                    'probe': False
                },
                'movement': {
                    'command': 'G0',
                    'feed': 6000,
                    'status': ''
                },
                'tools': {
                    '1' : {
                        'enabled' : true,
                        'direction': 'CW',
                        'speed': 440
                    },
                    '2' : {
                        'enabled' : false
                    }
                },
                'type': 'machine_state'
            }
        */

        // position
        double hw_x = doc["coordinates"]["hardware"][0].toDouble();
        double hw_y = doc["coordinates"]["hardware"][1].toDouble();
        double hw_z = doc["coordinates"]["hardware"][2].toDouble();

        double gl_x = doc["coordinates"]["global"][0].toDouble();
        double gl_y = doc["coordinates"]["global"][1].toDouble();
        double gl_z = doc["coordinates"]["global"][2].toDouble();

        double loc_x = doc["coordinates"]["local"][0].toDouble();
        double loc_y = doc["coordinates"]["local"][1].toDouble();
        double loc_z = doc["coordinates"]["local"][2].toDouble();
        QString loc_cs = doc["coordinates"]["cs"].toString();

        // endstops
        bool es_x = doc["endstops"]["axes"][0].toBool();
        bool es_y = doc["endstops"]["axes"][1].toBool();
        bool es_z = doc["endstops"]["axes"][2].toBool();
        bool es_probe = doc["endstops"]["probe"].toBool();

        // movement
        double feed = doc["movement"]["feed"].toDouble();
        QString cmd = doc["movement"]["command"].toString();
        bool is_moving = false;

        // tools
        QJsonValue toolsv = doc["tools"];
        if (toolsv.isObject())
        {
            QJsonObject tools = toolsv.toObject();
            for (auto ids : tools.keys())
            {
                int id = ids.toInt();
                QJsonObject desc = tools[ids].toObject();
                Tool tool = this->tools[id];
                switch (tool.type)
                {
                case type_none:
                    {
                        break;
                    }
                case type_binary:
                    {
                        bool enabled = desc["enabled"].toBool();
                        dsp->SetBinaryState(id, enabled);
                        break;
                    }
                case type_spindle:
                    {
                        int speed = desc["speed"].toInt();
                        bool enabled = desc["enabled"].toBool();
                        QString dir = desc["direction"].toString();
                        dsp->SetSpindleState(id, enabled, dir, speed);
                        break;
                    }
                }
            }
        }

        dsp->SetHwPosition(hw_x, hw_y, hw_z);
        dsp->SetGlobalPosition(gl_x, gl_y, gl_z);
        dsp->SetLocalPosition(loc_x, loc_y, loc_z, loc_cs);

        dsp->SetEndstops(es_x, es_y, es_z, es_probe);
        dsp->SetMovement(feed, cmd, is_moving);
    }
    else if (type == "state")
    {
        /*
            {
               'message': '',
               'state': 'init',
               'type': 'state'
            }
         */

        QString message = doc["message"].toString();
        QString state = doc["state"].toString();

        if (state == "init")
        {
            dsp->SetStateIdle();
        }
        else if (state == "running")
        {
            dsp->SetStateRunning();
        }
        else if (state == "paused")
        {
            dsp->SetStatePaused();
        }
        else
        {
            /* error */
        }

        if (message.length() > 0)
        {
            dsp->DisplayMessage("state", message);
        }
    }
    else if (type == "line")
    {
        /*
         {
            'type' : line,
            'line' : 0
         }
        */

        int line = doc["line"].toInt();
        dsp->SetActiveLine(line);
    }
    else if (type == "message")
    {
        QString msg_type = doc["message_type"].toString();
        QString msg = doc["message"].toString();
        dsp->DisplayMessage(msg_type, msg);
    }
}

void Receiver::read_data()
{
    int rcvd = sock->bytesAvailable();

    QByteArray data = sock->readAll();
    if (data.length() == 0)
        return;

    buffer.append(data);
    do
    {

        int i;
        int lenlen = 0;
        bool lenfound = false;
        for (i = 0; i < buffer.length(); i++)
        {
            if (buffer[i] == ';')
            {
                lenfound = true;
                lenlen = i;
                break;
            }
        }
        if (!lenfound)
            break;

        QByteArray lenarr = buffer.mid(0, lenlen);
        QString lenstr = QString(lenarr);
        int len = lenstr.toInt();

        if (buffer.length() < lenlen + 1 + len + 1)
            break;

        QByteArray data = buffer.mid(lenlen+1, len);
        buffer = buffer.mid(lenlen + 1 + len + 1);

        HandleState(data);
    } while(true);
    return;
}

Receiver::Receiver(IStateDisplay *dsp, QAbstractSocket *sock) : QObject()
{
    this->dsp = dsp;
    this->sock = sock;

    QObject::connect(sock, &QIODevice::readyRead, this, &Receiver::read_data);
}
