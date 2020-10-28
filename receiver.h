#ifndef RECEIVER_H
#define RECEIVER_H

#include "istatedisplay.h"
#include <QAbstractSocket>
#include <QObject>

enum ToolDriver
{
    driver_dummy = 0,
    driver_n700e,
    driver_modbus,
    driver_gpio,
};

enum ToolType
{
    type_none = 0,
    type_binary,
    type_spindle,
};

struct Tool
{
    QString name;
    ToolType type;
    ToolDriver driver;
};

class Receiver : public QObject
{
    Q_OBJECT
private:
    QByteArray buffer;
    IStateDisplay *dsp;
    QAbstractSocket *sock;
    void HandleState(QByteArray frame);

    QMap<int, Tool> tools;
public slots:
    void read_data();

public:
    Receiver(IStateDisplay *dsp, QAbstractSocket *sock);
    virtual ~Receiver() {}
};

#endif // RECEIVER_H
