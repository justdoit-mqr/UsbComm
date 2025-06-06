/*
 *@author:  缪庆瑞
 *@date:    2025.06.06
 *@brief:   作为一个demo负责提供UsbComm和UsbMonitor组件的使用方法和测试
 */
#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "usbcomm.h"
#include "usbmonitor.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void deviceHotplugSlot(bool isAttached,int vendorId,int productId,int port);//设备插拔响应槽

    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();

private:
    Ui::Widget *ui;

    UsbMonitor *hotplugMonitor;
    UsbComm    *usbReceive;
    unsigned char *recvBuffer;

};

#endif // WIDGET_H
