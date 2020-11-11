#include "gamepadmovecontroller.h"

void GamepadMoveController::update_state()
{
    if (x == 0 && y == 0 && z == 0)
    {
        // Don't move
        if (moving)
        {
            moving = false;
            emit movement_finish();
        }
    }
    else
    {
        // Move
        if (!moving)
        {
            moving = true;
            emit movement_start();
        }
        double fx;
        double fy;
        double fz;
        if (slow)
        {
            fx = slowfeed * x;
            fy = slowfeed * y;
            fz = slowfeed * z;
        }
        if (fast)
        {
            fx = fastfeed * x;
            fy = fastfeed * y;
            fz = fastfeed * z;
        }
        else
        {
            fx = feed * x;
            fy = feed * y;
            fz = feed * z;
        }
        emit movement_change(fx, fy, fz);
    }
}

GamepadMoveController::GamepadMoveController(double slowfeed, double feed, double fastfeed, QObject *parent) : QObject(parent)
{
    this->slowfeed = slowfeed;
    this->feed = feed;
    this->fastfeed = fastfeed;
    x = y = z = 0;
    fast = false;
    moving = false;
}

void GamepadMoveController::set_position_x(double x)
{
    this->x = x;
    update_state();
}

void GamepadMoveController::set_position_y(double y)
{
    this->y = y;
    update_state();
}

void GamepadMoveController::set_position_z(double z)
{
    this->z = z;
    update_state();
}

void GamepadMoveController::select_speed(bool fast, bool slow)
{
    this->slow = slow;
    this->fast = fast;
}
