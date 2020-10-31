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


class DummyToolWidget : public QPushButton
{
    QLabel *idlabel;
    QLabel *namelabel;

public:
    DummyToolWidget(int id, const QString &name, const QString &driver, QWidget *parent=nullptr):QPushButton(parent)
    {
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


MainWindow::MainWindow(QString addr, int port, QString configfile, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QString val;
    QFile file;
    file.setFileName(configfile);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    val = file.readAll();
    file.close();
    QJsonDocument cfgdoc = QJsonDocument::fromJson(val.toUtf8());
    cfg = cfgdoc.object();

    gcode_changed = true;
    ui->setupUi(this);
    qApp->installEventFilter(this);

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
    ui->start->setEnabled(true);
    ui->continue_btn->setEnabled(false);
    ui->manual_command->setReadOnly(false);
}

void MainWindow::SetStatePaused()
{
    ui->start->setEnabled(false);
    ui->continue_btn->setEnabled(true);
    ui->manual_command->setReadOnly(false);
}

void MainWindow::SetStateRunning()
{
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

void MainWindow::new_file()
{
    ui->gcodeEditor->setPlainText("");
}

void MainWindow::on_new_file_clicked()
{
    new_file();
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
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        return;
    }

    QString gcode;
    QTextStream in(&file);
    in.setCodec("UTF-8");
    gcode = in.readAll();

    ui->gcodeEditor->setPlainText(gcode);
    load_gcode();
}

void MainWindow::on_open_file_clicked()
{
    open_file();
}

void MainWindow::save_file()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save GCode"), "", tr("Gcode (*.gcode);;All Files (*)"));
    if (fileName.isEmpty())
        return;

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
}

void MainWindow::on_save_file_clicked()
{
    save_file();
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

void MainWindow::load_gcode()
{
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
        ctl->Configure(cfg);
    }
    gcode_changed = true;
}
