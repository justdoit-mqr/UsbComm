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
 */
#include "usbcomm.h"
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>

/*
 *@brief:   构造函数，负责对libusb进行初始化
 *@author:  缪庆瑞
 *@date:    2021.03.15
 */
UsbComm::UsbComm(QObject *parent)
    : QObject(parent)
{
    //成员变量初始化
    context = NULL;
    hotplugHandle = -1;
    hotplugEventHandler = NULL;
    deviceHandle = NULL;
    //libusb初始化
    int err = libusb_init(&context);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_init error:"<<err<<libusb_error_name(err);
    }
    //设置日志输出等级
    libusb_set_option(context,LIBUSB_OPTION_LOG_LEVEL,LIBUSB_LOG_LEVEL_WARNING);
}
/*
 *@brief:   析构函数，负责对libusb进行资源释放
 *@author:  缪庆瑞
 *@date:    2021.03.15
 */
UsbComm::~UsbComm()
{
    deregisterHotplugMonitorService();//注销热插拔服务
    releaseUsbInterface();//释放声明的接口
    closeUsbDevice();//关闭usb设备
    libusb_exit(context);//libusb退出
}
/*
 *@brief:   探测系统当前接入的usb设备，打印设备详细信息(调试用)
 *@author:  缪庆瑞
 *@date:    2021.03.15
 */
void UsbComm::findUsbDevices()
{
    libusb_device **devs;
    ssize_t count = libusb_get_device_list(context,&devs);//获取设备列表
    for(int i=0;i<count;i++)
    {
        printDevInfo(devs[i]);//打印设备详情
    }
    libusb_free_device_list(devs,1);//释放设备列表(解引用)
}
/*
 *@brief:   注册热插拔监测服务
 *@author:  缪庆瑞
 *@date:    2021.03.18
 *@param:   deviceClass:监测的设备类
 *@param:   vendorId:监测的设备厂商id
 *@param:   productId:监测的设备产品id
 *@return:   bool:true=注册成功  false=注册失败
 */
bool UsbComm::registerHotplugMonitorService(int deviceClass, int vendorId, int productId)
{
    //先判断当前平台的libusb库是否支持热插拔监测
    if(!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG))
    {
        qDebug()<<"hotplug capabilites are not supported on this platform";
        return false;
    }
    //注册热插拔的回调函数
    int err = libusb_hotplug_register_callback(
                context, (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED|LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                LIBUSB_HOTPLUG_NO_FLAGS, vendorId,productId, deviceClass,hotplugCallback,(void *)this, &hotplugHandle);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_hotplug_register_callback error:"<<err<<libusb_error_name(err);
        return false;
    }
    //回调函数需要经过轮询事件处理才可以被触发执行
    if(hotplugEventHandler == NULL)
    {
        hotplugEventHandler = new UsbEventHandler(context,this);
    }
    hotplugEventHandler->setStopped(false);
    hotplugEventHandler->start();

    return true;
}
/*
 *@brief:   注销热插拔监测服务
 *@author:  缪庆瑞
 *@date:    2021.03.18
 */
void UsbComm::deregisterHotplugMonitorService()
{
    if(hotplugHandle != -1)//注销回调
    {
        libusb_hotplug_deregister_callback(context,hotplugHandle);
        hotplugHandle = -1;
    }
    if(hotplugEventHandler != NULL)//停止热插拔事件处理线程
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
 *@author:  缪庆瑞
 *@date:    2021.03.18
 *@param:   ctx:表示libusb的一个会话
 *@param:   device:热插拔的设备
 *@param:   event:热插拔的事件
 *@param:   user_data:用户数据指针，在调用libusb_hotplug_register_callback()传递，
 *                  我们这里传的是this指针,实现在静态成员函数访问实例对象的方法与数据。
 *@return:   int:当返回值为1时，则会撤销注册(deregistered)
 */
int UsbComm::hotplugCallback(libusb_context *ctx, libusb_device *device,
                             libusb_hotplug_event event, void *user_data)
{
    Q_UNUSED(ctx)
    Q_UNUSED(device)
    //强制转换成注册热插拔监测的实例对象指针
    UsbComm *tmpUsbComm = (UsbComm*)user_data;
    //设备插入
    if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
    {
        emit tmpUsbComm->deviceHotplugSig(true);
    }
    //设备拔出
    else
    {
        emit tmpUsbComm->deviceHotplugSig(false);
    }
    return 0;
}
/*
 *@brief:   打开指定的usb设备
 *@author:  缪庆瑞
 *@date:    2021.03.16
 *@param:   vendorId:厂商id
 *@param:   productId:产品id
 *@param:   bConfigurationValue:要激活的配置值(1-*)，默认值为1
 *@return:   bool:true=成功  false=失败
 */
bool UsbComm::openUsbDevice(quint16 vendorId, quint16 productId, int bConfigurationValue)
{
    /*打开设备
     * libusb_open_device_with_vid_pid()是一个便捷函数，方便快速打开指定pid和vid的设备。函数内部
     * 实际通过libusb_get_device_list()获取所有设备，然后遍历各个设备的描述符，判断是否有匹配(pid/vid)
     * 的设备，如果有则调用libusb_open()打开设备返回句柄，并自动释放设备列表。
     *注:官方注释中提示该函数有使用限制，比如多个设备有相同的id，该函数仅返回第一个设备句柄，所以
     * 并不打算在真正的应用程序中使用该函数。不过对于常规的项目需求，一般不会遇到该限制。
     */
    deviceHandle = libusb_open_device_with_vid_pid(context,vendorId,productId);
    if(deviceHandle == NULL)
    {
        qDebug()<<"libusb_open_device_with_vid_pid error.";
        return false;
    }
    /*激活指定的配置(一个设备可能有多个配置，但同一时刻只能激活1个)
     *可以通过libusb_get_configuration()获取当前激活的配置值(默认为1)，如果选择的配置已经激活，那么此调用将
     * 会是一个轻量级的操作，用来重置相关usb设备的状态。
     */
    int err = libusb_set_configuration(deviceHandle,bConfigurationValue);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_set_configuration error:"<<err<<libusb_error_name(err);
        return false;
    }
    return true;
}
/*
 *@brief:   关闭当前打开的usb设备
 *@author:  缪庆瑞
 *@date:    2021.03.16
 */
void UsbComm::closeUsbDevice()
{
    if(deviceHandle != NULL)
    {
        libusb_close(deviceHandle);//对应libusb_open()
        deviceHandle = NULL;
    }
}
/*
 *@brief:   声明usb设备接口
 * 在操作I/O或其他端点的时候必须先声明接口，接口声明用于告知底层操作系统你的程序想要取得此接口的所有权。
 *@author:  缪庆瑞
 *@date:    2021.03.16
 *@param:   interfaceNumber:接口号
 *@return:   bool:true=成功  false=失败
 */
bool UsbComm::claimUsbInterface(int interfaceNumber)
{
    //确保指定接口的内核驱动程序未激活，否则将无法声明该接口
    if(libusb_kernel_driver_active(deviceHandle, interfaceNumber) == 1)
    {
        qDebug()<<"Kernel driver active for interface"<<interfaceNumber;
        //卸载指定接口的内核驱动
        int err = libusb_detach_kernel_driver(deviceHandle,interfaceNumber);
        if(err != LIBUSB_SUCCESS)
        {
            qDebug()<<"libusb_detach_kernel_driver error:"<<err<<libusb_error_name(err);
            return false;
        }
    }
    //声明接口(该接口是一个单纯的逻辑操作,不会通过总线发送任何请求)
    int err = libusb_claim_interface(deviceHandle, interfaceNumber);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_claim_interface error:"<<err<<libusb_error_name(err);
        return false;
    }
    //将成功声明的接口号添加到列表，方便在退出时释放所有声明的接口
    if(!claimedInterfaceList.contains(interfaceNumber))
    {
        claimedInterfaceList.append(interfaceNumber);
    }
    return true;
}
/*
 *@brief:   释放所有声明过的接口
 *@author:  缪庆瑞
 *@date:    2021.03.16
 */
void UsbComm::releaseUsbInterface()
{
    //在关闭设备句柄之前需要释放所有声明过的接口
    for(int i=0;i<claimedInterfaceList.size();i++)
    {
        libusb_release_interface(deviceHandle,claimedInterfaceList.at(i));
    }
    claimedInterfaceList.clear();
}
/*
 *@brief:   发送数据(批量(块)传输)
 *@author:  缪庆瑞
 *@date:    2021.03.17
 *@param:   endPoint:使用的端点，需要是输出端点(主机-->设备)，且传输类型为批量(块)传输
 *@param:   bulkData:发送的批量数据字节数组
 *@return:   int:实际发送的字节数。函数返回后最好检查该返回值是否与发送字节数一致，以判
 * 断传输是否成功。
 */
int UsbComm::sendBulkData(quint8 endPoint, QByteArray &bulkData)
{
    int actualLength=0;//实际发送的字节数
    int dataLength = bulkData.size();//只发送字节数组的有效数据，补的'\0'不发送
    //该函数是阻塞的，只有数据发完或者超时才会返回
    int err = libusb_bulk_transfer(deviceHandle,endPoint|LIBUSB_ENDPOINT_OUT,
                                   (unsigned char *)bulkData.data(),dataLength,&actualLength,0);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_bulk_transfer error:"<<err<<libusb_error_name(err);
    }
    return actualLength;
}
/*
 *@brief:   打印USB设备详细信息
 *@author:  缪庆瑞
 *@date:    2021.03.15
 *@param:   usbDevice:对应一个usb设备
 */
void UsbComm::printDevInfo(libusb_device *usbDevice)
{
    /*设备描述符层级:
     *设备(device)->配置(configuration)->接口(interface)->备用设置(altsetting)->端点(endpoint)*/

    /*设备(device)*/
    libusb_device_descriptor deviceDesc;
    int err = libusb_get_device_descriptor(usbDevice, &deviceDesc);
    if(err != LIBUSB_SUCCESS)
    {
        qDebug()<<"libusb_get_device_descriptor error:"<<err<<libusb_error_name(err);
        return;
    }
    qDebug()<<"***************************************";
    qDebug()<<"Bus: "<<(int)libusb_get_bus_number(usbDevice);//设备所在总线
    qDebug()<<"Device Address: "<<(int)libusb_get_device_address(usbDevice);//设备在总线上的地址
    qDebug()<<"Device Port: "<<(int)libusb_get_port_number(usbDevice);//设备端口号
    qDebug()<<"Device Speed: "<<(int)libusb_get_device_speed(usbDevice);//设备连接速度，详见enum libusb_speed{}
    qDebug()<<"Device Class: "<<QString("0x%1").arg((int)deviceDesc.bDeviceClass,2,16,QChar('0'));//设备类
    qDebug()<<"VendorID: "<<QString("0x%1").arg((int)deviceDesc.idVendor,4,16,QChar('0'));//设备厂商id
    qDebug()<<"ProductID: "<<QString("0x%1").arg((int)deviceDesc.idProduct,4,16,QChar('0'));//设备产品id
    qDebug()<<"Number of configurations: "<<(int)deviceDesc.bNumConfigurations;//设备的配置数
    /*配置(configuration)*/
    for(int i=0;i<(int)deviceDesc.bNumConfigurations;i++)
    {
        qDebug()<<"Configuration index:"<<i;
        libusb_config_descriptor *configDesc;
        libusb_get_config_descriptor(usbDevice,i,&configDesc);
        qDebug()<<"Configuration Value: "<<(int)configDesc->bConfigurationValue;
        qDebug()<<"Number of interfaces: "<<(int)configDesc->bNumInterfaces;
        /*接口(interface)*/
        for(int j=0; j<(int)configDesc->bNumInterfaces;j++)
        {
            qDebug()<<"\tInterface index:"<<j;
            const libusb_interface *usbInterface;
            usbInterface = &configDesc->interface[j];
            qDebug()<<"\tNumber of alternate settings: "<<usbInterface->num_altsetting;
            /*备用设置(altsetting)*/
            for(int k=0; k<usbInterface->num_altsetting; k++)
            {
                qDebug()<<"\t\tAltsetting index:"<<k;
                const libusb_interface_descriptor *interfaceDesc;
                interfaceDesc = &usbInterface->altsetting[k];
                qDebug()<<"\t\tInterface Class: "<<
                          QString("0x%1").arg((int)interfaceDesc->bInterfaceClass,2,16,QChar('0'));//接口类
                qDebug()<<"\t\tInterface Number: "<<(int)interfaceDesc->bInterfaceNumber;
                qDebug()<<"\t\tNumber of endpoints: "<<(int)interfaceDesc->bNumEndpoints;
                /*端点(endpoint)*/
                for(int m=0; m<(int)interfaceDesc->bNumEndpoints; m++)
                {
                    qDebug()<<"\t\t\tEndpoint index:"<<m;
                    const libusb_endpoint_descriptor *endpointDesc;
                    endpointDesc = &interfaceDesc->endpoint[m];
                    qDebug()<<"\t\t\tEP address: "<<
                              QString("0x%1").arg((int)endpointDesc->bEndpointAddress,2,16,QChar('0'));
                    qDebug()<<"\t\t\tEP transfer type:"<<
                              (endpointDesc->bmAttributes&0x03);//端点传输类型，详见enum libusb_transfer_type{}
                }
            }
        }
        libusb_free_config_descriptor(configDesc);//释放配置描述符空间
    }
    qDebug()<<"***************************************"<<endl;
}
/*
 *@brief:   构造函数
 *@author:  缪庆瑞
 *@date:    2021.03.18
 *@param:   context:表示libusb的一个会话
 *@parent:   parent:父对象
 */
UsbEventHandler::UsbEventHandler(libusb_context *context, QObject *parent)
    :QThread(parent)
{
    this->context = context;
    this->stopped = false;
}
/*
 *@brief:   子线程运行
 *@author:  缪庆瑞
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
