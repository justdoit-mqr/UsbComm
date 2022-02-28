# UsbComm
这是一个使用Qt框架开发的USB应用层通信组件，内部对libusb的常用api接口进行了二次封装，方便开发使用。主要实现了两大功能：++与usb设备端进行通信数据传输(UsbComm)++ 和 ++负责对usb设备热插拔监测(UsbMonitor)++。  
注:在项目的3rdparty目录下提供了libusb-1.0的头文件和库，这里是我用的Ubuntu16.04平台通过"apt install libusb-1.0-0-dev"命令安装，版本是1.0.20，对于不同的平台和环境只需要替换头文件和库即可。  

## 功能概述
UsbComm组件目前由三个类组成：`UsbComm`、`UsbMonitor`和`UsbEventHandler`，为了方便移植，三个类统一声明/定义在usbcomm.h和usbcomm.cpp中。 
### 1.UsbComm
USB通信主类，该类主要实现与usb设备端的通信数据传输。内部按需封装libusb的方法接口，并维护着当前打开的设备句柄列表和声明的接口列表，所以对于设备句柄和接口的相关操作尽量都使用该类的方法处理，不要在外边单独使用原生libusb接口，避免造成内部维护的列表失效而产生异常。  
```
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
```
### 2.UsbMonitor
USB热插拔监测类,该类可以用来定义成"全局"(有较长的生命周期)对象，实现对指定的usb设备进行热插拔监测。  
```
    //注册热插拔监测服务
    bool registerHotplugMonitorService(int deviceClass=LIBUSB_HOTPLUG_MATCH_ANY,
                                int vendorId=LIBUSB_HOTPLUG_MATCH_ANY,
                                int productId=LIBUSB_HOTPLUG_MATCH_ANY);
    //注销热插拔监测服务
    void deregisterHotplugMonitorService();

signals:
    void deviceHotplugSig(bool isAttached);//设备插拔信号
```
### 3.UsbEventHandler
USB事件处理类，该类继承自QThread，重写run()方法，在子线程中轮询处理挂起的事件(主要是USB设备的热插拔事件)，进而触发热插拔的回调函数。目前该类单纯是配合UsbMonitor的热插拔监测接口使用，相关处理已经封装在接口内，其他地方无需使用。  

## 小结
该组件的设计初衷是为了实现在嵌入式Linux平台连接USB热敏打印机打印小票的需求。因为使用的打印机不提供Linux系统的驱动，而Linux系统通用usblp驱动跟设备不匹配，所以最终只能使用libusb这种'免驱'设计，在应用层直接与usb设备建立通信，使用ESC/POS指令控制打印机。为了日后能够应对其他USB设备的通信，故将usb通信部分单独提取出来封装成该组件，方便使用。  
而之后又遇到一个与USB接口相机通信取图的需求，所以在原来组件的基础上进行了一些修改，将热插拔监测功能从UsbComm中分离出去，单独成类。而UsbComm只负责通信数据传输，内部维护设备句柄列表，实现对多个设备(包括相同vpid的设备)的访问。  
## 参考资料
1. [libusb官网](https://libusb.info/)  
2. [libusb源代码(Github)](https://github.com/libusb/libusb)  
3. [libusb源代码(Sourceforge)](https://sourceforge.net/projects/libusb/)  
4. [libusb-1.0-api接口文档](http://libusb.sourceforge.net/api-1.0/)  
5. [巨人的肩膀之QtUsb](https://github.com/fpoussin/QtUsb)  
6. [巨人的肩膀之QUSB](https://github.com/bimetek/QUSB)  

## 作者联系方式
**邮箱:justdoit_mqr@163.com**  
**新浪微博:@为-何-而来**  
