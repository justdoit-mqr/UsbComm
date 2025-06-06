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
 */
#include "usbmonitor.h"
#include <QDebug>

/*
 *@brief:   构造函数
 *@date:    2022.02.22
 *@parent:   parent:父对象
 */
UsbMonitor::UsbMonitor(QObject *parent)
    :QObject(parent)
{
    //成员变量初始化
    context = NULL;
    hotplugEventHandler = NULL;
    //libusb初始化
    int err = libusb_init(&context);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_init error:"<<libusb_error_name(err);
    }
    //设置日志输出等级
    libusb_set_debug(context,LIBUSB_LOG_LEVEL_WARNING);//旧版本
    //libusb_set_option(context,LIBUSB_OPTION_LOG_LEVEL,LIBUSB_LOG_LEVEL_WARNING);//新版本
}

UsbMonitor::~UsbMonitor()
{
    deregisterHotplugMonitorService();//注销热插拔服务
    libusb_exit(context);//libusb退出
}
/*
 *@brief:   注册热插拔监测服务
 *该接口支持调用多次，注册监测不同的设备类、vpid等
 *@date:    2022.02.22
 *@update:  2025.06.06
 *@param:   deviceClass:监测的设备类，默认LIBUSB_HOTPLUG_MATCH_ANY
 *@param:   vendorId:监测的设备厂商id，默认LIBUSB_HOTPLUG_MATCH_ANY
 *@param:   productId:监测的设备产品id，默认LIBUSB_HOTPLUG_MATCH_ANY
 *@param:   hotplugHandle:保存热插拔句柄，默认传递空指针，表示不需要在外部使用该句柄。
 *如果需要，可以在外部定义变量，并传递指针，当注册成功后会将句柄赋值给外部变量，一般用于在外部注销该热插拔句柄服务。
 *注：只有注册成功，该句柄值才有效
 *@return:  bool:true=注册成功  false=注册失败
 */
bool UsbMonitor::registerHotplugMonitorService(int deviceClass, int vendorId, int productId,
                                               libusb_hotplug_callback_handle *hotplugHandle)
{
    //先判断当前平台的libusb库是否支持热插拔监测
    if(!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG))
    {
        qDebug()<<"hotplug capabilites are not supported on this platform";
        return false;
    }
    //注册热插拔的回调函数
    libusb_hotplug_callback_handle tmpHotplugHandle = -1;
    int err = libusb_hotplug_register_callback(
                context, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED|LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                LIBUSB_HOTPLUG_NO_FLAGS, vendorId,productId, deviceClass,hotplugCallback,(void *)this, &tmpHotplugHandle);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_hotplug_register_callback error:"<<libusb_error_name(err);
        return false;
    }
    if(hotplugHandle)
    {
        *hotplugHandle = tmpHotplugHandle;
    }
    hotplugHandleList.append(tmpHotplugHandle);
    //回调函数需要经过轮询事件处理才可以被触发执行
    if(hotplugEventHandler == NULL)
    {
        hotplugEventHandler = new UsbEventHandler(context,this);
    }
    hotplugEventHandler->setStopped(false);
    if(!hotplugEventHandler->isRunning())
    {
        hotplugEventHandler->start();
    }

    return true;
}
/*
 *@brief:   注销热插拔监测服务
 *@date:    2022.02.22
 *@update:  2025.06.06
 *@param:   hotplugHandle:要注销的热插拔句柄指针，默认为空，表示注销当前所有注册的热插拔服务
 */
void UsbMonitor::deregisterHotplugMonitorService(libusb_hotplug_callback_handle *hotplugHandle)
{
    if(hotplugHandle)//注销指定的热插拔服务回调
    {
        if(hotplugHandleList.contains(*hotplugHandle))
        {
            libusb_hotplug_deregister_callback(context,*hotplugHandle);
            hotplugHandleList.removeAll(*hotplugHandle);
        }
    }
    else//注销所有的热插拔服务回调
    {
        for(int i=0;i<hotplugHandleList.size();i++)
        {
            libusb_hotplug_deregister_callback(context,hotplugHandleList.at(i));
        }
        hotplugHandleList.clear();
    }
    if(hotplugHandleList.isEmpty() && hotplugEventHandler != NULL)//停止热插拔事件处理线程
    {
        hotplugEventHandler->setStopped(true);
        hotplugEventHandler->wait();//等待线程结束
    }
}

/*
 *@brief:   热插拔回调函数(在UsbEventHandler子线程中执行)
 * 注1:必须是静态成员函数或全局函数，否则会因为隐含的this指针导致注册回调语句编译不通过，
 * 也因此回调函数无法直接使用实例对象，但可以通过函参user_data访问实例对象的方法与数据。
 * 注2:该函数内使用user_data发射实例对象的信号，因为信号依附于子线程发射，而槽一般在主线
 * 程，connect默认采用队列连接，确保了该函数只做最小处理，绝不拖泥带水。
 *@date:    2022.02.22
 *@param:   ctx:表示libusb的一个会话
 *@param:   device:热插拔的设备
 *@param:   event:热插拔的事件
 *@param:   user_data:用户数据指针，在调用libusb_hotplug_register_callback()传递，
 *                  我们这里传的是this指针,实现在静态成员函数访问实例对象的方法与数据。
 *@return:  int:当返回值为1时，则会撤销注册(deregistered)
 */
int UsbMonitor::hotplugCallback(libusb_context *ctx, libusb_device *device,
                             libusb_hotplug_event event, void *user_data)
{
    Q_UNUSED(ctx)

    int vendorId = -1,productId = -1;
    int port = libusb_get_port_number(device);//获取热插拔设备的端口号
    //获取热插拔设备的vid pid
    libusb_device_descriptor deviceDesc;
    int err = libusb_get_device_descriptor(device, &deviceDesc);
    if(err == LIBUSB_SUCCESS)
    {
        vendorId = deviceDesc.idVendor;
        productId = deviceDesc.idProduct;
    }
    //强制转换成注册热插拔监测的实例对象指针
    UsbMonitor *tmpUsbMonitor = static_cast<UsbMonitor*>(user_data);
    //设备插入
    if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
    {
        emit tmpUsbMonitor->deviceHotplugSig(true,vendorId,productId,port);
    }
    //设备拔出
    else
    {
        emit tmpUsbMonitor->deviceHotplugSig(false,vendorId,productId,port);
    }
    return 0;
}

/*
 *@brief:   构造函数
 *@date:    2021.03.18
 *@param:   context:表示libusb的一个会话
 *@parent:  parent:父对象
 */
UsbEventHandler::UsbEventHandler(libusb_context *context, QObject *parent)
    :QThread(parent)
{
    this->context = context;
    this->stopped = false;
}
/*
 *@brief:   子线程运行
 *@date:    2021.03.18
 */
void UsbEventHandler::run()
{
    //超时时间 100ms
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    while(!this->stopped && context != NULL)
    {
        //qDebug()<<"libusb_handle_events().......";
        /* 处理挂起的事件，非阻塞，超时即返回
         * 最开始使用的是libusb_handle_events()阻塞操作，但该阻塞会导致线程无法正常结束，
         * 调用terminate()强制结束后执行wait操作会卡死，怀疑是该阻塞操作会陷入内核态，导
         * 致在用户态下强制终止线程失败。
         * 注:如果有挂起的热插拔事件，注册的回调函数会在该线程内被调用。
         */
        libusb_handle_events_timeout_completed(context,&tv,NULL);
    }
}
