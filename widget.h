#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "usbcomm.h"

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
    void deviceHotplugSlot(bool isAttached);//设备插拔响应槽

    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();

private:
    Ui::Widget *ui;

    UsbMonitor *hotplugMonitor;
    UsbComm    *usbReceive;
    unsigned char *recvBuffer;

};

#endif // WIDGET_H
