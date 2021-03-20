#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QTextCodec>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),hotplugMonitor(NULL)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}
//列出当前接入到系统的所有usb设备
void Widget::on_pushButton_clicked()
{
    UsbComm usbComm;
    usbComm.findUsbDevices();
}
//打开指定的usb设备，并发送通信数据
void Widget::on_pushButton_2_clicked()
{
    UsbComm usbComm;
    //这里对应的是我们用的USB接口的热敏打印机设备
    if(usbComm.openUsbDevice(0x0483,0x5748))
    {
        if(usbComm.claimUsbInterface(0))
        {
            QByteArray array0("hello printers\n");
            qDebug()<<usbComm.sendBulkData(0x07,array0);

            QTextCodec *codec = QTextCodec::codecForName("GBK");
            QByteArray array1 = codec->fromUnicode(QString::fromUtf8("我看看能不能打印中文！\n"));
            qDebug()<<usbComm.sendBulkData(0x07,array1);

            QByteArray array2;//ESC控制命令(打印并走纸1行)
            array2.append(0x1b);
            array2.append(0x64);
            array2.append(0x01);
            qDebug()<<usbComm.sendBulkData(0x07,array2);
        }
    }
}
//注册热插拔监测服务
void Widget::on_pushButton_3_clicked()
{
    if(hotplugMonitor == NULL)
    {
        hotplugMonitor = new UsbComm(this);
        connect(hotplugMonitor,SIGNAL(deviceHotplugSig(bool)),this,SLOT(deviceHotplugSlot(bool)));
    }
    hotplugMonitor->deregisterHotplugMonitorService();
    hotplugMonitor->registerHotplugMonitorService();
}
//设备插拔信号响应槽
void Widget::deviceHotplugSlot(bool isAttached)
{
    if(isAttached)//插入
    {
        qDebug()<<"usb device attached";
    }
    else//拔出
    {
        qDebug()<<"usb device detached";
    }
}


