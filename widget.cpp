/*
 *@author:  缪庆瑞
 *@date:    2025.06.06
 *@brief:   作为一个demo负责提供UsbComm和UsbMonitor组件的使用方法和测试
 */
#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QTextCodec>
#include <QByteArray>
#include <QTime>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),hotplugMonitor(NULL),usbReceive(NULL),recvBuffer(NULL)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
    if(recvBuffer)
    {
        free(recvBuffer);
    }
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
    QMultiMap<quint16, quint16> vpidMap;
    vpidMap.insert(0x0483,0x5748);
    if(usbComm.openUsbDevice(vpidMap))
    {
        if(usbComm.claimUsbInterface(usbComm.getDeviceHandleFromIndex(0),0))
        {
            QByteArray array0("hello printers\n");
            qDebug()<<usbComm.bulkTransfer(usbComm.getDeviceHandleFromIndex(0),0x07,
                                           (quint8 *)array0.data(),array0.size(),0);

            QTextCodec *codec = QTextCodec::codecForName("GBK");
            QByteArray array1 = codec->fromUnicode(QString::fromUtf8("我看看能不能打印中文！\n"));
            qDebug()<<usbComm.bulkTransfer(usbComm.getDeviceHandleFromIndex(0),0x07,
                                           (quint8 *)array1.data(),array1.size(),0);

            QByteArray array2;//ESC控制命令(打印并走纸1行)
            array2.append(0x1b);
            array2.append(0x64);
            array2.append(0x01);
            qDebug()<<usbComm.bulkTransfer(usbComm.getDeviceHandleFromIndex(0),0x07,
                                           (quint8 *)array2.data(),array2.size(),0);
        }
    }
}
//注册热插拔监测服务
void Widget::on_pushButton_3_clicked()
{
    if(hotplugMonitor == NULL)
    {
        hotplugMonitor = new UsbMonitor(this);
        connect(hotplugMonitor,&UsbMonitor::deviceHotplugSig,this,&Widget::deviceHotplugSlot);
    }
    hotplugMonitor->deregisterHotplugMonitorService();//注销当前所有的热插拔服务回调
    hotplugMonitor->registerHotplugMonitorService();
}
//设备插拔信号响应槽
void Widget::deviceHotplugSlot(bool isAttached, int vendorId, int productId, int port)
{
    if(isAttached)//插入
    {
        qDebug()<<QString("usb device attached::VID=0x%1 PID=0x%2 Port=%3")
                  .arg(vendorId,0,16).arg(productId,0,16).arg(port);
    }
    else//拔出
    {
        qDebug()<<QString("usb device detached::VID=0x%1 PID=0x%2 Port=%3")
                  .arg(vendorId,0,16).arg(productId,0,16).arg(port);
    }
}
//打开指定的usb设备，并接收通信数据
void Widget::on_pushButton_4_clicked()
{
    static int flag = 0;
    const int package_len = 8192;
    if(usbReceive == NULL)
    {
        usbReceive = new UsbComm(this);
        //这里对应的是我们用的USB接口的4k相机
        QMultiMap<quint16, quint16> vpidMap;
        vpidMap.insert(0x04b4,0x00f1);
        if(usbReceive->openUsbDevice(vpidMap))
        {
            if(usbReceive->claimUsbInterface(usbReceive->getDeviceHandleFromIndex(0),0))
            {
                flag = 1;
                recvBuffer = (unsigned char *)malloc(package_len);
            }
        }
        if(flag != 1)
        {
            usbReceive->deleteLater();
            usbReceive = NULL;
        }
    }
    if(flag == 1)
    {
        memset(recvBuffer,0,package_len);
        int len = usbReceive->bulkTransfer(usbReceive->getDeviceHandleFromIndex(0),0x81,recvBuffer,package_len,1);
        if(len < 0)
        {
            qDebug()<<"bulkTransfer error"<<len;
        }
        else
        {
            QByteArray array;
            array.append((char *)recvBuffer,len);
            qDebug()<<QString("array[%1]:").arg(len)<<array.toHex();
        }
    }
}
//重置接收设备(重置会清空当前缓冲区的数据)
void Widget::on_pushButton_5_clicked()
{
    if(usbReceive != NULL)
    {
        bool resetFlag = usbReceive->resetUsbDevice(usbReceive->getDeviceHandleFromIndex(0));
        qDebug()<<"resetFlag============"<<resetFlag;
    }
}
