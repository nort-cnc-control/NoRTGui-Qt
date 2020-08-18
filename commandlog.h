#ifndef COMMANDLOG_H
#define COMMANDLOG_H
#include <QString>
#include <QStringList>

class CommandLog
{
private:
    QString cache;
    QStringList log;
    int index;
public:
    CommandLog();
    void AddNewLine(QString cmd);
    void LogUp();
    void LogDown();
    void CacheNew(QString cmd);
    void CacheClear();
    void IndexReset();
    QString GetCurrentCommand();
    QString GetLog();
};

#endif // COMMANDLOG_H
