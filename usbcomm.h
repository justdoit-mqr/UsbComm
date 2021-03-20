/****************************************************************************
*
* Copyright (C) 2021 MiaoQingrui. All rights reserved.
* Author: 缪庆瑞 <justdoit_mqr@163.com>
*
****************************************************************************/
/*
 *@author:  缪庆瑞
 *@date:    2021.03.15
 *@brief:   USB应用层通信组件(对libusb的api接口进行二次封装，方便使用)
 *
 * 1.负责对usb设备热插拔监测
 * 2.与usb设备端进行通信数据传输
 *
 */
#ifndef USBCOMM_H
#define USBCOMM_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QTimer>
#include "libusb-1.0/include/libusb.h"

class UsbEventHandler;

/* USB通信主类
 * 该类可以定义成局部对象，实现与usb设备端的通信数据传输。也可以定义成"全局"(有较长的生命
 * 周期)对象用于对指定的usb设备进行热插拔监测。
 * 因为目前代码设计细节还不完善，不建议使用同一对象完成以上两个功能。
 */
class UsbComm : public QObject
{
    Q_OBJECT
public:
    explicit UsbComm(QObject *parent = 0);
    ~UsbComm();

    void findUsbDevices();//探测系统当前接入的usb设备，打印设备详细信息(调试用)

    /*热插拔监测*/
    bool registerHotplugMonitorService(int deviceClass=LIBUSB_HOTPLUG_MATCH_ANY,
                                int vendorId=LIBUSB_HOTPLUG_MATCH_ANY,
                                int productId=LIBUSB_HOTPLUG_MATCH_ANY);//注册热插拔监测服务
    void deregisterHotplugMonitorService();//注销热插拔监测服务
    static int LIBUSB_CALL hotplugCallback(libusb_context *ctx,libusb_device *device,
                                           libusb_hotplug_event event,void *user_data);//热插拔回调函数

    /*设备初始化*/
    bool openUsbDevice(quint16 vendorId,quint16 productId,int bConfigurationValue=1);//打开指定usb设备
    void closeUsbDevice();//关闭当前打开的usb设备
    bool claimUsbInterface(int interfaceNumber);//声明usb设备的接口
    void releaseUsbInterface();//释放所有声明过的接口
    /*数据传输*/
    int sendBulkData(quint8 endPoint,QByteArray &bulkData);//发送数据(批量(块)传输)

private:
    void printDevInfo(libusb_device *usbDevice);//打印USB设备详细信息

    libusb_context *context;//表示libusb的一个会话，由libusb_init创建
    libusb_hotplug_callback_handle hotplugHandle;//热插拔回调句柄
    UsbEventHandler *hotplugEventHandler;//热插拔事件处理对象
    libusb_device_handle *deviceHandle;//一个usb设备的句柄
    QList<int> claimedInterfaceList;//当前声明的接口列表

signals:
    void deviceHotplugSig(bool isAttached);//设备插拔信号

public slots:

};

/* USB事件处理类
 * 该类继承自QThread，重写run()方法，在子线程中轮询处理挂起的事件(主要是USB设备的
 * 热插拔事件)，进而触发热插拔的回调函数。目前该类单纯是配合UsbComm的热插拔监测
 * 接口使用，相关处理已经封装在接口内，其他地方无需使用。
 */
class UsbEventHandler : public QThread
{
    Q_OBJECT
public:
    UsbEventHandler(libusb_context *context, QObject *parent = 0);
    //设置控制线程结束的标记变量状态
    void setStopped(bool stopped){this->stopped = stopped;}

protected:
    virtual void run();

private:
    libusb_context *context;//表示libusb的一个会话，由构造函参传递
    volatile bool stopped;//标记变量，控制线程结束
};

#endif // USBCOMM_H
