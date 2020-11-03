#include "configuration.h"
#include "ui_configuration.h"

Configuration::Configuration(QList<QPair<QString, QString>> profiles, QString profilename, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Configuration)
{
    ui->setupUi(this);
    profile_template = "{}";
    this->profiles = profiles;
    if (profiles.count() > 0)
        currentProfile = 0;
    else
        currentProfile = -1;

    for (int i = 0; i < profiles.count(); ++i)
    {
        if (profiles[i].first == profilename)
        {
            currentProfile = i;
        }
        ui->profiles->insertItem(i, profiles[i].first);
    }
    ui->profiles->setCurrentIndex(currentProfile);
    ui->config->setPlainText(profiles[currentProfile].second);
    ui->profile_name->setText(profilename);
}

Configuration::~Configuration()
{
    delete ui;
}

void Configuration::on_remove_profile_clicked()
{

}

void Configuration::on_config_textChanged()
{
    if (currentProfile >= 0 && currentProfile < profiles.count())
        profiles[currentProfile].second = ui->config->toPlainText();
}

void Configuration::on_profiles_activated(int index)
{
    int nump = profiles.count();
    if (index > ui->profiles->count()-1)
    {
        return;
    }
    else if (index == ui->profiles->count()-1)
    {
        // Last item - Add new profile
        QPair<QString, QString> newp;
        newp.first = "New profile " + QString::number(nump);
        newp.second = profile_template;
        profiles.insert(nump, newp);
        ui->profiles->insertItem(nump, newp.first);
        ui->profiles->setCurrentIndex(index);
    }

    // Select profile
    currentProfile = index;
    ui->config->setPlainText(profiles[currentProfile].second);
    ui->profile_name->setText(profiles[currentProfile].first);
}

void Configuration::on_profile_name_textEdited(const QString &newname)
{
    if (currentProfile >= 0 && currentProfile < profiles.count())
    {
        ui->profiles->setItemText(currentProfile, newname);
        profiles[currentProfile].first = newname;
    }
}

QPair<QString, QString> Configuration::profile()
{
    if (currentProfile == -1)
    {
        QPair<QString, QString> nullpair(nullptr, nullptr);
        return nullpair;
    }
    return profiles[currentProfile];
}

void Configuration::on_buttonBox_accepted()
{
    this->done(0);
}

void Configuration::on_buttonBox_rejected()
{
    this->done(1);
}
