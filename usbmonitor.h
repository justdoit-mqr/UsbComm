/****************************************************************************
*
* Copyright (C) 2021-2025 MiaoQingrui. All rights reserved.
* Author: 缪庆瑞 <justdoit_mqr@163.com>
*
****************************************************************************/
/*
 *@author:  缪庆瑞
 *@date:    2021.03.15
 *@update:  2025.06.06
 *@brief:   USB插拔状态监测组件
 *内部通过调用libusb的热插拔相关的api接口实现
 *备注：libusb库V1.0.23之前的版本，在热插拔回调监测时存在一个bug,会报错提示“libusb: error [udev_hotplug_event]
 *ignoring udev action bind”,可以通过升级版本解决该问题
 */
#ifndef USBMONITOR_H
#define USBMONITOR_H

#include <QObject>
#include <QThread>
#include <QList>
#include "libusb-1.0/include/libusb.h"


class UsbEventHandler;
/* USB热插拔监测类
 * 该类可以用来定义成"全局"(有较长的生命周期)对象，实现对指定的usb设备进行热插拔监测。*/
class UsbMonitor : public QObject
{
    Q_OBJECT
public:
    UsbMonitor(QObject *parent = 0);
    ~UsbMonitor();

    //注册热插拔监测服务
    bool registerHotplugMonitorService(int deviceClass=LIBUSB_HOTPLUG_MATCH_ANY,
                                int vendorId=LIBUSB_HOTPLUG_MATCH_ANY,
                                int productId=LIBUSB_HOTPLUG_MATCH_ANY,
                                libusb_hotplug_callback_handle *hotplugHandle = nullptr);
    //注销热插拔监测服务
    void deregisterHotplugMonitorService(libusb_hotplug_callback_handle *hotplugHandle = nullptr);

signals:
    void deviceHotplugSig(bool isAttached,int vendorId,int productId,int port);//设备插拔信号

private:
    //热插拔回调函数
    static int LIBUSB_CALL hotplugCallback(libusb_context *ctx,libusb_device *device,
                                           libusb_hotplug_event event,void *user_data);

    libusb_context *context;//表示libusb的一个会话，由libusb_init创建
    QList<libusb_hotplug_callback_handle> hotplugHandleList;//注册的热插拔回调句柄列表
    UsbEventHandler *hotplugEventHandler;//热插拔事件处理对象

};

/* USB事件处理类
 * 该类继承自QThread，重写run()方法，在子线程中轮询处理挂起的事件(主要是USB设备的
 * 热插拔事件)，进而触发热插拔的回调函数。目前该类单纯是配合UsbMonitor的热插拔监测
 * 接口使用，相关处理已经封装在接口内，其他地方无需使用。*/
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

#endif // USBMONITOR_H
