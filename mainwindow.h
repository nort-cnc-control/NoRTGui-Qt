#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "commandlog.h"
#include "controller.h"
#include "istatedisplay.h"
#include "receiver.h"

#include <QMainWindow>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow, public IStateDisplay
{
    Q_OBJECT

public:
    MainWindow(QString addr, int port, QWidget *parent = nullptr);
    ~MainWindow();

    void SetHwPosition(double x, double y, double z);
    void SetGlobalPosition(double x, double y, double z);
    void SetLocalPosition(double x, double y, double z, QString cs);
    void SetEndstops(bool x, bool y, bool z, bool probe);
    void SetMovement(double feed, QString command, bool is_moving);
    void SetActiveLine(int line);

    void SetStateIdle();
    void SetStatePaused();
    void SetStateRunning();

    void ClearTools();
    void AddNoneTool(int id, QString name, QString driver);
    void AddBinaryTool(int id, QString name, QString driver);
    void AddSpindleTool(int id, QString name, QString driver);
    void SetBinaryState(int id, bool enabled);
    void SetSpindleState(int id, bool enabled, QString dir, int speed);

    void DisplayMessage(QString header, QString message);
protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void on_new_file_clicked();
    void on_open_file_clicked();
    void on_save_file_clicked();
    void on_start_clicked();
    void on_stop_clicked();
    void on_continue_btn_clicked();
    void on_remoteConnect_clicked();
    void on_homing_btn_clicked();
    void on_zprobe_btn_clicked();
private:
    Ui::MainWindow *ui;
    CommandLog log;
    Controller *ctl;
    QTcpSocket *sock;
    Receiver *rcv;
    bool connected;
    bool gcode_changed;

    void load_gcode();
    void run_command(QString cmd);

    void new_file();
    void open_file();
    void save_file();
    void rconnect(QString addr, int port);

    QMap<int, QWidget *> tool_widgets;
    QLayout *tools_layout;
};
#endif // MAINWINDOW_H
