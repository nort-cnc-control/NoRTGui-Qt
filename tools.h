#ifndef TOOLLIST_H
#define TOOLLIST_H

#include <QMap>
#include <QString>

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

struct DummyTool : public Tool
{};

struct ModbusTool : public Tool
{
    int device_id;
    int reg;
};

struct GPIOTool : public Tool
{
    int gpio_id;
};

struct N700ETool : public Tool
{
    int device_id;
    int maxspeed;
    int basespeed;
};

#endif // TOOLLIST_H
