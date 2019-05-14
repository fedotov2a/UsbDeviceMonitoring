#include <QDateTime>
#include <QDebug>

#include <libudev.h>
#include <unistd.h>

struct udev_device *get_child_device(struct udev *udev,
                                     struct udev_device *parent,
                                     const char *subsystem)
{
    struct udev_device *child = NULL;
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_parent(enumerate, parent);
    udev_enumerate_add_match_subsystem(enumerate, subsystem);

    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices)
    {
        const char *path = udev_list_entry_get_name(entry);
        child = udev_device_new_from_syspath(udev, path);

        break;
    }

    udev_enumerate_unref(enumerate);

    return child;
}

QString get_usb_device_info(struct udev_device *device, struct udev_device *block)
{
    QString action = udev_device_get_property_value(device, "ACTION");

    if (action.isEmpty())
    {
        action = "exists";
    }

    QString devname_block = udev_device_get_property_value(block, "DEVNAME");

    if (devname_block.isEmpty())
    {
        devname_block = "none";
    }

    return QString("TIME_EVENT = [%1] "
                   "ACTION = [%2] "
                   "DEVNAME = [%3] "
                   "VENDOR = [%4 %5] "
                   "MODEL = [%6 %7] ")
           .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"))
           .arg(action)
           .arg(devname_block)
           .arg(udev_device_get_property_value(device, "ID_VENDOR"))
           .arg(udev_device_get_property_value(device, "ID_VENDOR_FROM_DATABASE"))
           .arg(udev_device_get_property_value(device, "ID_MODEL"))
           .arg(udev_device_get_property_value(device, "ID_MODEL_FROM_DATABASE"))
          + QString("VID:MID = [%1:%2] "
                    "SERIAL = [%3] "
                    "SERIAL_SHORT = [%4]")
           .arg(udev_device_get_property_value(device, "ID_VENDOR_ID"))
           .arg(udev_device_get_property_value(device, "ID_MODEL_ID"))
           .arg(udev_device_get_property_value(device, "ID_SERIAL"))
           .arg(udev_device_get_property_value(device, "ID_SERIAL_SHORT"));
}

void check_action_device(struct udev *udev, struct udev_device *device)
{
    QString action = QString(udev_device_get_action(device));

    if (action.isEmpty())
    {
        // Устройство уже присутствует в системе.

        // Нужно получить файл устройства (типа /dev/sdb).
        // Получаем подсистему block.
        struct udev_device *block = get_child_device(udev, device, "block");

        // USB устройство и файл устройства связаны серийными номерами.
        // Проверяю одинаковые ли серийные номера.
        QString device_serial = QString(udev_device_get_property_value(device, "ID_SERIAL_SHORT"));
        QString block_serial = QString(udev_device_get_property_value(block, "ID_SERIAL_SHORT"));

        if (! device_serial.isEmpty() &&
            ! block_serial.isEmpty() &&
            device_serial == block_serial)
        {
            qDebug() << get_usb_device_info(device, block);
            qDebug() << "-------------" << endl;

            udev_device_unref(block);
        }
    }
    else if (action == "add")
    {
        // Устройство было добавлено (воткнута флешка).

        // Пытаемся за несколько попыток получить файл устройства.
        // Поскольку при подключении флешки, файл устройства определяется не сразу.
        int count = 0;
        const int count_attempt_checking = 30;

        while (count < count_attempt_checking)
        {
            struct udev_device *block = get_child_device(udev, device, "block");

            QString device_serial = QString(udev_device_get_property_value(device, "ID_SERIAL_SHORT"));
            QString block_serial = QString(udev_device_get_property_value(block, "ID_SERIAL_SHORT"));

            if (! device_serial.isEmpty() &&
                ! block_serial.isEmpty() &&
                device_serial == block_serial)
            {
                qDebug() << get_usb_device_info(device, block);
                qDebug() << "-------------" << endl;
                break;
            }

            udev_device_unref(block);

            ++count;

            usleep(500 * 1000);     // 0.5s
        }

        if (count == count_attempt_checking)
        {
            qDebug() << "Can't detect device file";
        }

    }
    else if (action == "remove")
    {
        // Устройство было удалено (вынули флешку).

        struct udev_device *block = get_child_device(udev, device, "block");

        qDebug() << get_usb_device_info(device, block);
        qDebug() << "-------------" << endl;
    }
}

void detect_usb_devices(struct udev *udev)
{
    // Создаем поиск.
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);

    // Задаем фильтр поиска устройств.
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "usb_device");

    // Запускаем сканирование устройств.
    udev_enumerate_scan_devices(enumerate);

    // Получаем результат сканирования.
    struct udev_list_entry *deviceList = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, deviceList)
    {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *device = udev_device_new_from_syspath(udev, path);

        check_action_device(udev, device);
        udev_device_unref(device);
    }

    udev_enumerate_unref(enumerate);
}

void monitoring_usb_devices(struct udev *udev)
{
    struct udev_monitor *monitor = udev_monitor_new_from_netlink(udev, "udev");

    udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", "usb_device");
    udev_monitor_enable_receiving(monitor);

    int fd = udev_monitor_get_fd(monitor);

    while (true)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        if (select(fd + 1, &fds, NULL, NULL, NULL) <= 0)
        {
            break;
        }

        if (FD_ISSET(fd, &fds))
        {
            struct udev_device *dev = udev_monitor_receive_device(monitor);
            check_action_device(udev, dev);
        }
    }
}

void print_all_property(struct udev_device *device)
{
    udev_list_entry *property_list = udev_device_get_properties_list_entry(device);
    udev_list_entry *list_entry;

    udev_list_entry_foreach(list_entry, property_list)
    {
        qDebug() << QString("PROPERTY [%1] = [%2]")
                 .arg(udev_list_entry_get_name(list_entry))
                 .arg(udev_list_entry_get_value(list_entry));
    }

    qDebug() << "---------------------";
}

void detectAllDevices(struct udev *udev)
{
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);

    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *deviceList = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    // Посмотреть какие есть подсистемы и типы устройств.
    QMap<QString, int> subsystem_map;
    QMap<QString, int> devtype_map;

    udev_list_entry_foreach(entry, deviceList)
    {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *device = udev_device_new_from_syspath(udev, path);

        QString subsystem = udev_device_get_subsystem(device);

        if (subsystem_map.contains(subsystem))
        {
            subsystem_map[subsystem] += 1;
        }
        else
        {
            subsystem_map.insert(subsystem, 0);
        }

        QString devtype = udev_device_get_devtype(device);

        if (devtype_map.contains(devtype))
        {
            devtype_map[devtype] += 1;
        }
        else
        {
            devtype_map.insert(devtype, 0);
        }

        print_all_property(device);
        udev_device_unref(device);
    }

    qDebug() << "SUBSYSTEMS:" << subsystem_map.keys();
    qDebug() << "DEVTYPE:" << devtype_map.keys();

    udev_enumerate_unref(enumerate);
}

int main()
{
    struct udev *udev = udev_new();

//    detectAllDevices(udev);
    detect_usb_devices(udev);
    monitoring_usb_devices(udev);

    udev_unref(udev);

    return 0;
}
