#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QRegExp>
#include <QRegExpValidator>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    labelStBar = new QLabel(QString("就绪"));
    ui->statusbar->addWidget(labelStBar);

    // 设置 IP 及端口号输入限制
    QRegExp ipMask ("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b");
    ui->editIP->setValidator(new QRegExpValidator(ipMask));
    QRegExp portMask ("^([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])$");
    ui->editPort->setValidator(new QRegExpValidator(portMask));
    ui->editIP->setText("127.0.0.1");
    ui->editPort->setText("502");

    // 设置读取参数初值
    ui->editInterval->setText("3000");
    ui->editStart->setText("0");
    ui->editLength->setText("8");

    // 使获取数据按钮默认不可用
    ui->btnFetch->setEnabled(false);

    // 创建连接按钮的信号槽
    connect(ui->btnConnect, SIGNAL(clicked()), this, SLOT(onBtnConnect()));

    // 初始化 QTimer
    timer = new QTimer(this);

}

void MainWindow::onBtnConnect()
{
    // 处理 IP / 端口未填写的情况，防止程序异常退出
    if(ui->editIP->text().isEmpty() | ui->editPort->text().isEmpty())
    {
        QMessageBox::information(this, "ModBus 连接", "请输入连接信息");
        return;
    }

    // 判断是否已经连接到 Slave 端
    if(!isConnected)
    {
        // 建立 modbus 客户端连接
        modbusClient = modbus_new_tcp(ui->editIP->text().toStdString().c_str(), ui->editPort->text().toInt());
        // 设置从机地址
        modbus_set_slave(modbusClient, 1);
        // 连接客户端
        int mConnect = modbus_connect(modbusClient);
        if(mConnect == -1)
        {
            // errno 为标准错误代码
            QMessageBox::warning(this, "连接失败", modbus_strerror(errno));
            ui->statusbar->removeWidget(labelStBar);
            labelStBar = new QLabel(QString("连接失败: ").append(modbus_strerror(errno)));
            ui->statusbar->addWidget(labelStBar);
            modbus_free(modbusClient);
            return;
        }
        // 连接成功 更改按钮及连接状态变量
        isConnected = true;
        isFetching = false;
        ui->btnFetch->setEnabled(true);
        ui->btnConnect->setText("Disconnect");
        ui->statusbar->removeWidget(labelStBar);
        labelStBar = new QLabel(QString("连接成功！IP: ").append(ui->editIP->text()).append(" 端口: ").append(ui->editPort->text()));
        ui->statusbar->addWidget(labelStBar);
        // 激活数据获取按钮功能
        connect(ui->btnFetch, SIGNAL(clicked()), this, SLOT(onBtnFetch()));
    }
    else
    {
        // 断开连接
        // 停止正在获取的数据
        stopFetching();
        isFetching = false;
        isConnected = false;
        modbus_free(modbusClient);
        ui->btnFetch->setEnabled(false);
        ui->btnConnect->setText("Connect");
        ui->statusbar->removeWidget(labelStBar);
        labelStBar = new QLabel(QString("连接已断开"));
        ui->statusbar->addWidget(labelStBar);
    }
}

void MainWindow::onBtnFetch()
{
    // 设置超时时间
    timeOut.tv_sec = 1;
    modbus_set_response_timeout(modbusClient, &timeOut);
    int interval = ui->editInterval->text().toInt();
    int start = ui->editStart->text().toInt();
    int length = ui->editLength->text().toInt();

    timer->setInterval(interval);

    // bug: 偶数次连接时 timer 已启动，但是内部动作并未执行
    connect(timer, &QTimer::timeout, [=](){
        fetchData(start, length);
    });

    if(isConnected && !isFetching)
    {
        // 暂时取消点击后立即获取数据的功能，原因：多次连接后会导致同时触发多次 fetchData 操作
//        fetchData(start, length);
        qDebug() << timer->isActive() << endl;
        startTimer(timer);
        qDebug() << timer->isActive() << endl;
        ui->btnFetch->setText("Stop");
        isFetching = true;
    }
    else if(isConnected && isFetching)
    {
        stopFetching();
        isFetching = false;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 清空按钮
void MainWindow::on_btnClear_clicked()
{
    ui->textRecv->setText("");
    // 清空寄存器
    memset(tab_reg, 0, sizeof(tab_reg));
}

// 启动计时器
void MainWindow::startTimer(QTimer *timer)
{
    timer->start();
}

// 停止计时器
void MainWindow::stopTimer(QTimer *timer){
    if(timer->isActive())
    {
        timer->stop();
    }
}

// 获取数据
void MainWindow::fetchData(int start, int length)
{
    // 读取保持寄存器
    int rdNum = modbus_read_registers(modbusClient, start, length, tab_reg);
    // 在尝试解决读取长度超出 rdNum 范围时遇到问题，暂时搁置
    if(rdNum > 0 && rdNum >= start)
    {
        QString str;
        ui->textRecv->append("读取时间: " + QTime::currentTime().toString());
        for(int i = start; i < start + length; i++)
        {
            str = QString("Address: %1, Data: %2").arg(i).arg(tab_reg[i]);
            ui->textRecv->append(str);
        }
    }
    else
    {
        ui->textRecv->append("无数据");
    }
}

// 停止获取数据
void MainWindow::stopFetching()
{
    stopTimer(timer);
    ui->btnFetch->setText("Fetch");
    timer->disconnect();
}
