#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "modbus.h"

#include <QMainWindow>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    bool isConnected = false;
    bool isFetching = false;
    void startTimer(QTimer *timer);
    void stopTimer(QTimer *timer);
    void stopFetching();
    QTimer *timer;
    modbus_t *modbusClient;
    QLabel *labelStBar;
    uint16_t tab_reg[65535] = {0};
    struct timeval timeOut;
    ~MainWindow();

public slots:
    void onBtnConnect();
    void onBtnFetch();
    void fetchData(int start, int length);

private slots:
    void on_btnClear_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
