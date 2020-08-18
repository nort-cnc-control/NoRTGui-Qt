#include "commandlog.h"

CommandLog::CommandLog()
{
    index = -1;
}

void CommandLog::AddNewLine(QString cmd)
{
    log.push_back(cmd);
}

void CommandLog::LogUp()
{
    if (index > 0)
    {
        index--;
    }
    else if (index == -1)
    {
        index = log.size()-1;
    }
}

void CommandLog::LogDown()
{
    if (index >= 0)
    {
        index++;
    }
    if (index == (int)log.size())
    {
        index = -1;
    }
}

void CommandLog::CacheNew(QString cmd)
{
    cache = cmd;
}

void CommandLog::CacheClear()
{
    cache = "";
}

void CommandLog::IndexReset()
{
    index = -1;
}

QString CommandLog::GetCurrentCommand()
{
    if (index == -1)
    {
        return cache;
    }
    else
    {
        return log[index];
    }
}

QString CommandLog::GetLog()
{
    return log.join("\n");
}
