#ifndef RECEIVER_H
#define RECEIVER_H

#include "istatedisplay.h"
#include <QAbstractSocket>
#include <QSocketNotifier>
#include <QObject>

class Receiver : public QObject
{
    Q_OBJECT
private:
    QSocketNotifier *notifier;

    QByteArray buffer;
    IStateDisplay *dsp;
    QAbstractSocket *sock;
    void HandleState(QByteArray frame);

public slots:
    void read_data(int socket);

public:
    Receiver(IStateDisplay *dsp, QAbstractSocket *sock);
    virtual ~Receiver() {}
};

#endif // RECEIVER_H
