#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QDialog>
#include <QMap>

namespace Ui {
class Configuration;
}

class Configuration : public QDialog
{
    Q_OBJECT

public:
    explicit Configuration(QList<QPair<QString, QString>> profiles, QString profilename, QWidget *parent = nullptr);
    ~Configuration();

private slots:
    void on_remove_profile_clicked();

    void on_config_textChanged();

    void on_profiles_activated(int index);

    void on_profile_name_textEdited(const QString &arg1);

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

public:
    QPair<QString, QString> profile();
    QList<QPair<QString, QString>> profiles;
private:
    Ui::Configuration *ui;
    int currentProfile;
    QString profile_template;
};

#endif // CONFIGURATION_H
