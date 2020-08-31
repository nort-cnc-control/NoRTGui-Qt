#ifndef ISTATEDISPLAY_H
#define ISTATEDISPLAY_H

#include <QString>


class IStateDisplay
{
public:
    virtual ~IStateDisplay() {}

    virtual void SetHwPosition(double x, double y, double z) = 0;
    virtual void SetGlobalPosition(double x, double y, double z) = 0;
    virtual void SetLocalPosition(double x, double y, double z, QString cs) = 0;
    virtual void SetEndstops(bool x, bool y, bool z, bool probe) = 0;
    virtual void SetMovement(double feed, QString command, bool is_moving) = 0;
    virtual void SetSpindleState(int speed, QString state) = 0;
    virtual void SetActiveLine(int line) = 0;

    virtual void SetStateIdle() = 0;
    virtual void SetStatePaused() = 0;
    virtual void SetStateRunning() = 0;

    virtual void DisplayMessage(QString header, QString message) = 0;
};

#endif // ISTATEDISPLAY_H
