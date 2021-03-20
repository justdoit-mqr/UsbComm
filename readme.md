# UsbComm
这是一个使用Qt框架开发的USB应用层通信组件，内部对libusb的常用api接口进行了二次封装，方便开发使用。主要实现了两大功能：==负责对usb设备热插拔监测==和==与usb设备端进行通信数据传输==。  
注:在项目的3rdparty目录下提供了libusb-1.0的头文件和库，其中库是基于我个人用的嵌入式linux平台环境交叉编译得到的，对于不同的平台和环境只需要替换该库即可。  

## 功能概述
UsbComm组件目前由两个类组成：`UsbComm`和`UsbEventHandler`，为了方便移植，两个类统一声明/定义在usbcomm.h和usbcomm.cpp中。  
### 1.UsbComm
USB通信主类，该类可以定义成局部对象，实现与usb设备端的通信数据传输。也可以定义成"全局"(有较长的生命周期)对象用于对指定的usb设备进行热插拔监测。因为目前代码设计细节还不完善，不建议使用同一对象完成以上两个功能。  
### 2.UsbEventHandler
USB事件处理类,该类继承自QThread，重写run()方法，在子线程中轮询处理挂起的事件(主要是USB设备的热插拔事件)，进而触发热插拔的回调函数。目前该类单纯是配合UsbComm的热插拔监测接口使用，相关处理已经封装在接口内，其他地方无需使用。  
## 代码接口
因为UsbEventHandler只在UsbComm内部使用，所以该组件的对外接口全在UsbComm类中。主要分为：热插拔监测相关接口、设备初始化接口、数据传输接口(目前仅实现了批量写传输的功能封装)。如下所示，详情可参考代码头文件。  
```
//探测系统当前接入的usb设备，打印设备详细信息(调试用)
void findUsbDevices();

/*热插拔监测*/
//注册热插拔监测服务
bool registerHotplugMonitorService(int deviceClass=LIBUSB_HOTPLUG_MATCH_ANY,int vendorId=LIBUSB_HOTPLUG_MATCH_ANY,int productId=LIBUSB_HOTPLUG_MATCH_ANY);
//注销热插拔监测服务
void deregisterHotplugMonitorService();
//热插拔回调函数
static int LIBUSB_CALL hotplugCallback(libusb_context *ctx,libusb_device *device,libusb_hotplug_event event,void *user_data);

/***************设备初始化***************/
//打开指定usb设备
bool openUsbDevice(quint16 vendorId,quint16 productId,int bConfigurationValue=1);
//关闭当前打开的usb设备
void closeUsbDevice();
//声明usb设备的接口
bool claimUsbInterface(int interfaceNumber);
//释放所有声明过的接口
void releaseUsbInterface();
/***************数据传输***************/
//发送数据(批量(块)传输)
int sendBulkData(quint8 endPoint,QByteArray &bulkData);
```
## 小结
该组件的设计初衷是为了实现在嵌入式Linux平台连接USB热敏打印机打印小票的需求。因为使用的打印机不提供Linux系统的驱动，而Linux系统通用usblp驱动跟设备不匹配，所以最终只能使用libusb这种'免驱'设计，在应用层直接与usb设备建立通信，使用ESC/POS指令控制打印机。为了日后能够应对其他USB设备的通信，故将usb通信部分单独提取出来封装成该组件，方便使用。  
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
