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
 *@brief:   USB应用层通信组件
 *
 *该类主要实现与usb设备端进行通信数据传输
 *内部按需封装libusb的方法接口，并维护着当前打开的设备句柄列表和声明的接口列表，所以对于设备句柄和接口的相关操作尽量都使用该类的方法处理，
 *不要在外边单独使用原生libusb接口，避免造成内部维护的列表失效而产生异常。
 */
#ifndef USBCOMM_H
#define USBCOMM_H

#include <QObject>
#include <QList>
#include <QMultiMap>
#include "libusb-1.0/include/libusb.h"

class UsbComm : public QObject
{
    Q_OBJECT
public:
    explicit UsbComm(QObject *parent = 0);
    ~UsbComm();

    void findUsbDevices();//探测系统当前接入的usb设备，打印设备详细信息(调试用)

    /*设备初始化*/
    bool openUsbDevice(QMultiMap<quint16,quint16> &vpidMap);//打开指定设备(可能有多个)
    void closeUsbDevice(libusb_device_handle *deviceHandle);//关闭指定设备
    void closeAllUsbDevice();//关闭所有设备
    bool setUsbConfig(libusb_device_handle *deviceHandle,int bConfigurationValue=1);//激活usb设备当前配置
    bool claimUsbInterface(libusb_device_handle *deviceHandle,int interfaceNumber);//声明usb设备的接口
    void releaseUsbInterface(libusb_device_handle *deviceHandle,int interfaceNumber);//释放usb设备声明的接口
    bool setUsbInterfaceAltSetting(libusb_device_handle *deviceHandle,int interfaceNumber,int bAlternateSetting);//激活usb设备接口备用设置
    bool resetUsbDevice(libusb_device_handle *deviceHandle);//重置usb设备
    /*数据传输*/
    int bulkTransfer(libusb_device_handle *deviceHandle,quint8 endpoint, quint8 *data,
                     int length, quint32 timeout);//(批量(块)传输)

    /*设备查询*/
    int getOpenedDeviceCount(){return deviceHandleList.size();}//获取当前打开的设备数量
    /*该类中所有方法的函参(libusb_device_handle *deviceHandle)必须通过以下getDeviceHandleFrom*方法获取*/
    libusb_device_handle *getDeviceHandleFromIndex(int index);//通过索引获取打开的设备句柄
    libusb_device_handle *getDeviceHandleFromVpidAndPort(quint16 vid,quint16 pid,qint16 port);//通过vpid和端口号获取打开的设备句柄

private:
    void printDevInfo(libusb_device *usbDevice);//打印USB设备详细信息

    libusb_context *context;//表示libusb的一个会话，由libusb_init创建
    QList<libusb_device_handle *> deviceHandleList;//打开的usb设备句柄列表
    QMap<libusb_device_handle *,QList<int> > handleClaimedInterfacesMap;//句柄对应声明的接口列表的map

};

#endif // USBCOMM_H
