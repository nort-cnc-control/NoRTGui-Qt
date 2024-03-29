#include "configuration.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QScrollBar>
#include <QTcpSocket>
#include <QThread>
#include <QHostAddress>
#include <QFormLayout>
#include <QJsonObject>
#include <iostream>
#include <QJsonArray>
#include <QGamepad>


class DummyToolWidget : public QPushButton
{
    QLabel *idlabel;
    QLabel *namelabel;

public:
    DummyToolWidget(int id, const QString &name, const QString &driver, QWidget *parent=nullptr):QPushButton(parent)
    {
        Q_UNUSED(driver);
        idlabel = new QLabel(QString::number(id), this);
        namelabel = new QLabel(name, this);
        QHBoxLayout *layout = new QHBoxLayout();
        setLayout(layout);
        setMinimumHeight(90);
        setMinimumWidth(150);

        layout->addWidget(idlabel);
        layout->addWidget(namelabel);
    }
    ~DummyToolWidget()
    {}
};

class BinaryToolWidget : public QPushButton
{
    QLabel *enlabel;
    QLabel *idlabel;
    QLabel *namelabel;
public:
    BinaryToolWidget(int id, const QString &name, const QString &driver, QWidget *parent=nullptr):QPushButton(parent)
    {
        Q_UNUSED(driver);
        enlabel = new QLabel("⬤", this);
        idlabel = new QLabel(QString::number(id), this);
        namelabel = new QLabel(name, this);
        QHBoxLayout *layout = new QHBoxLayout();
        setLayout(layout);
        setMinimumHeight(90);
        setMinimumWidth(150);
        layout->addWidget(idlabel);
        layout->addWidget(enlabel);
        layout->addWidget(namelabel);
        enlabel->setStyleSheet("QLabel { color : red; }");
    }

    ~BinaryToolWidget()
    {
    }

    void setToolEnabled(bool enabled)
    {
        if (enabled)
            enlabel->setStyleSheet("QLabel { color : lightgreen; }");
        else
            enlabel->setStyleSheet("QLabel { color : red; }");
    }
};

class SpindleToolWidget : public QPushButton
{
    QLabel *idlabel;
    QLabel *namelabel;
    QLabel *spdlabel;
    QLabel *enlabel;
    QLabel *dirlabel;
public:
    SpindleToolWidget(int id, const QString &name, const QString &driver, QWidget *parent=nullptr):QPushButton(parent)
    {
        Q_UNUSED(driver);
        spdlabel = new QLabel(this);
        enlabel = new QLabel("⬤", this);
        dirlabel = new QLabel(this);
        idlabel = new QLabel(QString::number(id), this);
        namelabel = new QLabel(name, this);

        enlabel->setStyleSheet("QLabel { color : red; }");

        QHBoxLayout *layout = new QHBoxLayout();
        setLayout(layout);
        setMinimumHeight(90);
        setMinimumWidth(190);

        layout->addWidget(idlabel);
        layout->addWidget(enlabel);
        layout->addWidget(namelabel);
        layout->addWidget(dirlabel);
        layout->addWidget(spdlabel);


        enlabel->setStyleSheet("QLabel { color : red; }");
    }
    ~SpindleToolWidget()
    {
    }

    void setToolSpeed(int speed)
    {
        spdlabel->setNum(speed);
    }

    void setToolDirection(QString dir)
    {
        dirlabel->setText(dir);
    }

    void setToolEnabled(bool enabled)
    {
        if (enabled)
            enlabel->setStyleSheet("QLabel { color : lightgreen; }");
        else
            enlabel->setStyleSheet("QLabel { color : red; }");
    }
};


void MainWindow::createConfigurationDir(QString configdir)
{
    if (!QDir().mkdir(configdir))
    {
        //ERROR
        return;
    }
    QFile cfgfile(QDir(configdir).filePath("nort.json"));
    if (!cfgfile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // ERROR
        return;
    }
    QJsonObject cfg;
    QJsonArray profiles;
    cfg.insert("default", "Default");
    profiles.append("Default");
    cfg.insert("profiles", profiles);
    cfgfile.write(QJsonDocument(cfg).toJson());
    cfgfile.close();

    QFile prffile(QDir(configdir).filePath("Default.json"));
    if (!prffile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // ERROR
        return;
    }
    QJsonObject defconfig;
    prffile.write(QJsonDocument(defconfig).toJson());
    prffile.close();
}


MainWindow::MainWindow(QString addr, int port, QString configdir, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QString val;
    this->configdir = configdir;
    if (!QDir(configdir).exists())
        createConfigurationDir(configdir);

    // Read nort.json
    QFile configfile(QDir(configdir).filePath("nort.json"));
    if (!configfile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // ERROR
        return;
    }
    QJsonDocument config = QJsonDocument::fromJson(configfile.readAll());
    configfile.close();

    currentProfileName = config["default"].toString();

    // Read profiles
    auto cfgprofiles = config["profiles"].toArray();
    for (int i = 0; i < cfgprofiles.count(); ++i)
    {
        auto name = cfgprofiles[i].toString();
        auto filename = QDir(configdir).filePath(name + ".json");
        QFile file(filename);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        auto profile = file.readAll();
        file.close();
        QPair<QString, QString> pair(name, profile);
        profiles.append(pair);
        if (name == currentProfileName)
            currentProfile = profile;
    }

    gcode_changed = true;
    ui->setupUi(this);
    qApp->installEventFilter(this);

    ui->profile_name->setText("Profile: " + currentProfileName);
    ui->remoteAddr->setText(addr);
    ui->remotePort->setText(QString::number(port));

    sock = new QTcpSocket(this);
    rcv = new Receiver(this, sock);
    ctl = new Controller(sock);

    rconnect(ui->remoteAddr->text(), ui->remotePort->text().toInt());

    ctl->Reset();

    SetActiveLine(-1);
    ui->statusbar->show();
    SetStateIdle();

    tools_layout = new QHBoxLayout();
    ui->tools_bar->setLayout(tools_layout);

    optionsDialog = nullptr;

    gpmc = new GamepadMoveController(10, 100, 800, this);

    Q_ASSERT(connect(gpmc, &GamepadMoveController::movement_start, this, &MainWindow::gp_movement_started));
    Q_ASSERT(connect(gpmc, &GamepadMoveController::movement_finish, this, &MainWindow::gp_movement_finished));
    Q_ASSERT(connect(gpmc, &GamepadMoveController::movement_change, this, &MainWindow::gp_movement_changed));

    gamepad = nullptr;
    gamepad_manager = QGamepadManager::instance();

    use_first_gamepad();

    Q_ASSERT(connect(gamepad_manager, &QGamepadManager::gamepadConnected, this, &MainWindow::gamepad_connected));
    Q_ASSERT(connect(gamepad_manager, &QGamepadManager::gamepadDisconnected, this, &MainWindow::gamepad_disconnected));
}

void MainWindow::gamepad_connected(int deviceId)
{
    use_gamepad(deviceId);
}

void MainWindow::gamepad_disconnected(int deviceId)
{
    Q_UNUSED(deviceId);
    use_first_gamepad();
}

void MainWindow::use_first_gamepad()
{
    auto gamepads = gamepad_manager->connectedGamepads();
    if (gamepads.count() == 0)
    {
        use_gamepad(-1);
    }
    else
    {
        use_gamepad(gamepads[0]);
    }

}

void MainWindow::use_gamepad(int id)
{
    if (gamepad != nullptr)
    {
        disconnect(gamepad, &QGamepad::axisLeftXChanged, this, &MainWindow::gamepadLeftXChanged);
        disconnect(gamepad, &QGamepad::axisLeftYChanged, this, &MainWindow::gamepadLeftYChanged);
        disconnect(gamepad, &QGamepad::buttonUpChanged, this, &MainWindow::gamepadButtonUpChanged);
        disconnect(gamepad, &QGamepad::buttonDownChanged, this, &MainWindow::gamepadButtonDownChanged);
        disconnect(gamepad, &QGamepad::buttonL1Changed, this, &MainWindow::gamepadButtonLeftUp);
        disconnect(gamepad, &QGamepad::buttonL3Changed, this, &MainWindow::gamepadButtonLeftDown);
        delete gamepad;
        gamepad = nullptr;
    }

    if (id < 0)
        return;

    QString name = gamepad_manager->gamepadName(id);
    if (!gamepad_manager->isGamepadConnected(id))
        return;

    qDebug() << "Use gamepad: " << name;
    QString cfg = QDir(configdir).filePath("gamepad.cfg");
    gamepad_manager->setSettingsFile(cfg);
    gamepad_manager->resetConfiguration(id);
    gamepad_id = id;
    gamepad = new QGamepad(id, this);

    Q_ASSERT(connect(gamepad, &QGamepad::axisLeftXChanged, this, &MainWindow::gamepadLeftXChanged));
    Q_ASSERT(connect(gamepad, &QGamepad::axisLeftYChanged, this, &MainWindow::gamepadLeftYChanged));
    Q_ASSERT(connect(gamepad, &QGamepad::buttonUpChanged, this, &MainWindow::gamepadButtonUpChanged));
    Q_ASSERT(connect(gamepad, &QGamepad::buttonDownChanged, this, &MainWindow::gamepadButtonDownChanged));
    Q_ASSERT(connect(gamepad, &QGamepad::buttonL1Changed, this, &MainWindow::gamepadButtonLeftUp));
    Q_ASSERT(connect(gamepad, &QGamepad::buttonL3Changed, this, &MainWindow::gamepadButtonLeftDown));
}

void MainWindow::gamepadLeftXChanged(double x)
{
    gpmc->set_position_x(x);
}

void MainWindow::gamepadLeftYChanged(double y)
{
    gpmc->set_position_y(y);
}

void MainWindow::gamepadButtonUpChanged(bool value)
{
    qDebug() << "gamepad Up changed" << value;
    if (value)
    {
        gpmc->set_position_z(1);
    }
    if (!value)
    {
        if (gamepad->buttonDown())
            gpmc->set_position_z(-1);
        else
            gpmc->set_position_z(0);
    }
}

void MainWindow::gamepadButtonDownChanged(bool value)
{
    if (value)
    {
        gpmc->set_position_z(-1);
    }
    if (!value)
    {
        if (gamepad->buttonUp())
            gpmc->set_position_z(1);
        else
            gpmc->set_position_z(0);
    }
}

void MainWindow::gamepadButtonLeftUp(bool value)
{
    Q_UNUSED(value);
    bool fast = gamepad->buttonL1();
    bool slow = gamepad->buttonL3();
    gpmc->select_speed(fast, slow);
}

void MainWindow::gamepadButtonLeftDown(bool value)
{
    Q_UNUSED(value);
    bool fast = gamepad->buttonL1();
    bool slow = gamepad->buttonL3();
    gpmc->select_speed(fast, slow);
}

void MainWindow::gp_movement_started()
{
    if (!gcode_running)
    {
        movement_running = true;
        ctl->StartManualMovement();
    }
}

void MainWindow::gp_movement_finished()
{
    movement_running = false;
    ctl->StopManualMovement();
}

void MainWindow::gp_movement_changed(double fx, double fy, double fz)
{
    if (!gcode_running)
    {
        ctl->ManualMovementFeed(fx, fy, fz);
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SetHwPosition(double x, double y, double z)
{
    ui->hw_x->setNum(x);
    ui->hw_y->setNum(y);
    ui->hw_z->setNum(z);
}

void MainWindow::SetGlobalPosition(double x, double y, double z)
{
    ui->gl_x->setNum(x);
    ui->gl_y->setNum(y);
    ui->gl_z->setNum(z);
}

void MainWindow::SetLocalPosition(double x, double y, double z, QString cs)
{
    ui->loc_x->setNum(x);
    ui->loc_y->setNum(y);
    ui->loc_z->setNum(z);
    ui->loc_cs->setText(cs);
}

void MainWindow::SetLocalStatePosition(double x, double y, double z)
{
    ui->loc_state_x->setNum(x);
    ui->loc_state_y->setNum(y);
    ui->loc_state_z->setNum(z);
}

void MainWindow::SetEndstops(bool x, bool y, bool z, bool probe)
{
    ui->es_x->setText(x ? "+" : "-");
    ui->es_y->setText(y ? "+" : "-");
    ui->es_z->setText(z ? "+" : "-");
    ui->es_probe->setText(probe ? "+" : "-");
}

void MainWindow::SetMovement(double feed, QString command, bool is_moving)
{
    ui->feed->setNum(feed);
    ui->command->setText(command);
    ui->is_moving->setText(QString(is_moving));
}

void MainWindow::ClearTools()
{
    for (auto id : tool_widgets.keys())
    {
        tools_layout->removeWidget(tool_widgets[id]);
        delete tool_widgets[id];
    }
    tool_widgets.clear();
}

void MainWindow::AddNoneTool(int id, QString name, QString driver)
{
    QPushButton *tool = new DummyToolWidget(id, name, driver, this);
    //tool->setMaximumSize(60, 60);
    tool->setEnabled(false);
    tool->setMinimumHeight(50);
    tools_layout->addWidget(tool);
    tools_layout->setAlignment(tool, Qt::AlignLeft);
    tool_widgets.insert(id, tool);
}

void MainWindow::AddBinaryTool(int id, QString name, QString driver)
{
    BinaryToolWidget *tool = new BinaryToolWidget(id, name, driver, this);
    //tool->setMaximumSize(60, 60);
    //tool->setEnabled(false);
    tool->setMinimumHeight(50);
    tools_layout->addWidget(tool);
    tools_layout->setAlignment(tool, Qt::AlignLeft);
    tool_widgets.insert(id, tool);
}

void MainWindow::AddSpindleTool(int id, QString name, QString driver)
{
    SpindleToolWidget *tool = new SpindleToolWidget(id, name, driver, this);
    //tool->setMaximumSize(60, 60);
    //tool->setEnabled(false);
    tool->setMinimumHeight(50);
    tools_layout->addWidget(tool);
    tools_layout->setAlignment(tool, Qt::AlignLeft);
    tool_widgets.insert(id, tool);
}

void MainWindow::SetBinaryState(int id, bool enabled)
{
    auto widget = (BinaryToolWidget *)(tool_widgets[id]);
    widget->setToolEnabled(enabled);
}

void MainWindow::SetSpindleState(int id, bool enabled, QString dir, int speed)
{
    auto widget = (SpindleToolWidget *)(tool_widgets[id]);
    widget->setToolEnabled(enabled);
    widget->setToolSpeed(speed);
    widget->setToolDirection(dir);
}

void MainWindow::SetActiveLine(int line)
{
    if (line >= 0)
        ui->statusbar->showMessage("Current line: " + QString::number(line+1)); // nort uses lines numbering from 0, but text editors from 1
    else
        ui->statusbar->showMessage("Current line: N/A");
}

void MainWindow::SetStateIdle()
{
    gcode_running = false;
    ui->start->setEnabled(true);
    ui->continue_btn->setEnabled(false);
    ui->manual_command->setReadOnly(false);
}

void MainWindow::SetStatePaused()
{
    gcode_running = false;
    ui->start->setEnabled(false);
    ui->continue_btn->setEnabled(true);
    ui->manual_command->setReadOnly(false);
}

void MainWindow::SetStateRunning()
{
    gcode_running = true;
    ui->start->setEnabled(false);
    ui->continue_btn->setEnabled(false);
    ui->manual_command->setReadOnly(true);
}

void MainWindow::DisplayMessage(QString header, QString message)
{
    QMessageBox msgBox;
    msgBox.setText(message);
    msgBox.setWindowTitle(header);
    msgBox.exec();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->manual_command && event->type() == QEvent::KeyRelease)
    {
        int key = static_cast<QKeyEvent *>(event)->key();
        switch (key)
        {
        case Qt::Key_Up:
            log.LogUp();
            ui->manual_command->setText(log.GetCurrentCommand());
            break;
        case Qt::Key_Down:
            log.LogDown();
            ui->manual_command->setText(log.GetCurrentCommand());
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            QString cmd = ui->manual_command->text();
            if (cmd.length() > 0)
            {
                run_command(cmd);
            }
            break;
        }
        default:
            break;
        }
    }
    if (obj == ui->gcodeEditor && event->type() == QEvent::KeyPress)
    {
        gcode_changed = true;
    }
    return QObject::eventFilter(obj, event);
}

void MainWindow::on_actionNewFile_triggered()
{

    new_file();
}

void MainWindow::on_actionOpenFile_triggered()
{
    open_file();
}

void MainWindow::on_actionSaveFile_triggered()
{
    save_file(false);
}

void MainWindow::on_actionSaveAsFile_triggered()
{
    save_file(true);
}

void MainWindow::on_actionConfigure_triggered()
{
    optionsDialog = new Configuration(profiles, currentProfileName, this);
    connect(optionsDialog, SIGNAL(finished(int)), this, SLOT(configure_finished(int)));
    optionsDialog->setModal(true);
    optionsDialog->show();
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About", "NoRT CNC Control.\nhttps://github.com/nort-cnc-control");
}

void MainWindow::new_file()
{
    ui->gcodeEditor->setPlainText("");
    filename = "";
    this->setWindowTitle("NoRT CNC");
}

void MainWindow::open_file()
{
    QString fileName = QFileDialog::getOpenFileName(this,
           tr("Open GCode"), "",
           tr("GCode (*.gcode);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly| QFile::Text))
    {
        this->setWindowTitle("NoRT CNC");
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        return;
    }

    QString gcode;
    QTextStream in(&file);
    in.setCodec("UTF-8");
    gcode = in.readAll();

    ui->gcodeEditor->setPlainText(gcode);
    load_gcode();

    filename = fileName;
    this->setWindowTitle("NoRT CNC: " + filename);
}

void MainWindow::save_file(bool select_name)
{
    QString fileName;
    if (select_name || filename == "")
    {
        fileName = QFileDialog::getSaveFileName(this, tr("Save GCode"), "", tr("Gcode (*.gcode);;All Files (*)"));
        if (fileName.isEmpty())
            return;
    }
    else
    {
        fileName = filename;
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly| QFile::Text))
    {
        QMessageBox::information(this, tr("Unable to save file"), file.errorString());
        return;
    }

    QString gcode = ui->gcodeEditor->toPlainText();

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << gcode;

    filename = fileName;
    this->setWindowTitle("NoRT CNC: " + filename);
}

void MainWindow::on_start_clicked()
{
    load_gcode();
    ctl->Start();
}

void MainWindow::on_stop_clicked()
{
    ctl->Stop();
}

void MainWindow::on_continue_btn_clicked()
{
    ctl->Continue();
}

void MainWindow::on_en_steppers_clicked()
{
    ctl->EnableSteppers();
}

void MainWindow::on_dis_steppers_clicked()
{
    ctl->DisableSteppers();
}

void MainWindow::on_remoteConnect_clicked()
{
    if (connected)
    {
        connected = false;
        sock->close();
        ui->remoteConnect->setText("Connect");
        SetStateIdle();
    }
    else
    {
        rconnect(ui->remoteAddr->text(), ui->remotePort->text().toInt());
    }
}

void MainWindow::on_homing_btn_clicked()
{
    run_command("G28");
}

void MainWindow::on_zprobe_btn_clicked()
{
    run_command("G30");
}

void MainWindow::configure_finished(int result)
{
    if (optionsDialog == nullptr)
        return;
    if (result == 0)
    {
        auto prf = optionsDialog->profile();
        if (prf.first != nullptr)
        {
            currentProfile = prf.second;
            currentProfileName = prf.first;
        }
        profiles = optionsDialog->profiles;

        // Save nort.json
        QJsonObject currentConfiguration;
        currentConfiguration.insert("default", currentProfileName);
        QJsonArray prfs;
        for (int i = 0; i < profiles.count(); ++i)
            prfs.append(profiles[i].first);
        currentConfiguration.insert("profiles", prfs);

        QFile cfgfile(QDir(configdir).filePath("nort.json"));
        if (!cfgfile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            // ERROR
        }
        cfgfile.write(QJsonDocument(currentConfiguration).toJson());
        cfgfile.close();

        // Save profiles
        for (int i = 0; i < profiles.count(); ++i)
        {
            QFile prffile(QDir(configdir).filePath(profiles[i].first) + ".json");
            if (!prffile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                // ERROR
                return;
            }
            prffile.write(profiles[i].second.toUtf8());
            prffile.close();
        }
    }
    disconnect(optionsDialog, SIGNAL(finished(int)), this, SLOT(configure_finished(int)));
    delete optionsDialog;
    optionsDialog = nullptr;
    ui->profile_name->setText("Profile: " + currentProfileName);
}

void MainWindow::select_format()
{
    QString format = "gcode";
    QString fmt = ui->sourceFormat->currentText();
    if (fmt == "GCode")
        format = "gcode";
    else if (fmt == "Excellon")
        format = "excellon";

    ctl->SetSourceFormat(format);
}

void MainWindow::on_sourceFormat_currentTextChanged(const QString &text)
{
    select_format();
}

void MainWindow::load_gcode()
{
    select_format();
    ctl->LoadGCode(ui->gcodeEditor->toPlainText());
    gcode_changed = false;
}

void MainWindow::run_command(QString cmd)
{
    if (gcode_changed)
        load_gcode();
    log.AddNewLine(cmd);
    log.CacheClear();
    log.IndexReset();
    ui->manual_command->setText("");
    ui->commandLog->setPlainText(log.GetLog());
    ui->commandLog->verticalScrollBar()->setValue(ui->commandLog->verticalScrollBar()->maximum());
    ctl->RunCommand(cmd);
}

void MainWindow::rconnect(QString addr, int port)
{
    QHostAddress ha;
    ha.setAddress(addr);

    sock->connectToHost(ha, port);

    if(!sock->waitForConnected(3000))
    {
        /*Err*/
        //auto err = sock->error();
        auto errs = sock->errorString();
        QMessageBox msg;
        msg.setText("Can not connect to " + addr + ":" + QString::number(port));
        msg.exec();
        connected = false;
        sock->close();
        ui->remoteConnect->setText("Connect");
    }
    else
    {
        ui->remoteConnect->setText("Disconnect");
        connected = true;
        // Load current profile
        QJsonObject machineConfiguration = QJsonDocument::fromJson(currentProfile.toUtf8()).object();
        ctl->Configure(machineConfiguration);
    }
    gcode_changed = true;
}
