#include "receiver.h"

#include <QJsonDocument>
#include <QThread>

void Receiver::HandleState(QByteArray frame)
{
    QJsonDocument doc = QJsonDocument::fromJson(frame);

    QString type = doc["type"].toString();
    if (type == "machine_state")
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
                'spindel': {
                    'direction': '-',
                    'speed': 0,
                    'status': 'OFF'
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

        // spindle
        int speed = doc["spindel"]["speed"].toDouble();
        QString state = doc["spindel"]["direction"].toString();
        if (doc["spindel"]["status"].toString() == "OFF")
            state = "OFF";

        dsp->SetHwPosition(hw_x, hw_y, hw_z);
        dsp->SetGlobalPosition(gl_x, gl_y, gl_z);
        dsp->SetLocalPosition(loc_x, loc_y, loc_z, loc_cs);

        dsp->SetEndstops(es_x, es_y, es_z, es_probe);
        dsp->SetMovement(feed, cmd, is_moving);
        dsp->SetSpindleState(speed, state);
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
            dsp->DisplayMessage(message);
        }
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
