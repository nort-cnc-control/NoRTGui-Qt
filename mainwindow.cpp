#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QScrollBar>
#include <QTcpSocket>
#include <QThread>
#include <QHostAddress>

#include <iostream>

MainWindow::MainWindow(QString addr, int port, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    gcode_changed = true;
    ui->setupUi(this);
    qApp->installEventFilter(this);

    ui->remoteAddr->setText(addr);
    ui->remotePort->setText(QString::number(port));

    sock = new QTcpSocket(this);
    rconnect(ui->remoteAddr->text(), ui->remotePort->text().toInt());

    rcv = new Receiver(this, sock);
    ctl = new Controller(sock);
    ctl->Reset();

    SetActiveLine(-1);
    ui->statusbar->show();
    SetStateIdle();
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

void MainWindow::SetSpindleState(int speed, QString state)
{
    ui->speed->setNum(speed);
    ui->running_state->setText(state);
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

void MainWindow::load_gcode()
{
    ctl->LoadGCode(ui->gcodeEditor->toPlainText());
    gcode_changed = false;
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
    }
}
