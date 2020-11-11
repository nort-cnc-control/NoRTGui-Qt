#ifndef GAMEPADMOVECONTROLLER_H
#define GAMEPADMOVECONTROLLER_H

#include <QObject>

class GamepadMoveController : public QObject
{
    Q_OBJECT;
    double x, y, z;
    bool fast, slow;
    void update_state();
    double slowfeed, feed, fastfeed;
    bool moving;
public:
    GamepadMoveController(double slowfeed, double feed, double fastfeed, QObject *parent=nullptr);
    void set_position_x(double x);
    void set_position_y(double y);
    void set_position_z(double z);
    void select_speed(bool fast, bool slow);
signals:
    void movement_start();
    void movement_change(double fx, double fy, double fz);
    void movement_finish();
};

#endif // GAMEPADMOVECONTROLLER_H
