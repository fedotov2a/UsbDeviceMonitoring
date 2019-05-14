# UsbDeviceMonitoring
Monitoring usb devices. libudev example.

Пример использования библиотеки libudev. Тестировал на OpenSuse 15, AstraLinux 1.6.

1. Основные функции
  udev_enumerate_new() - инициализирует поиск.
  udev_enumerate_add_match_subsystem()
  udev_enumerate_add_match_property() - функции, с помощью которых задается фильтр поиска.
  udev_enumerate_scan_devices() - запуск сканирования (один раз).
  udev_enumerate_get_list_entry() - получение результата сканирования.
  udev_list_entry_get_name() - получение названия устройства.
  udev_device_new_from_syspath() - получение самого устройства.
  udev_device_get_action() - получает действие, совершенное устройством (add, remove, exists).
  udev_monitor_new_from_netlink() - инициализирует мониторинг.
  udev_monitor_filter_add_match_subsystem_devtype() - устанавливает фильтр мониторинга.
  udev_monitor_enable_receiving() - запускает мониторинг.
  udev_monitor_get_fd() - получает файл дескриптор, из которого можно отслеживать события.
  
2. Получение свойств устройства
  udev_device_get_property_value() - получает значение заданного свойства
  
  Список свойств usb устройства:
    BUSNUM
    DEVNAME = /dev/bus/usb/001/013
    DEVNUM
    DEVPATH
    DEVTYPE = usb_device
    DRIVER = usb
    ID_BUS
    ID_MODEL = Mass_Storage
    ID_MODEL_ENC
    ID_MODEL_FROM_DATABASE = Flash Drive - берется из файла /var/lib/usbutils/usb.ids (поставляется в пакете usbutils)
    ID_MODEL_ID = 6387
    ID_REVISION
    ID_SERIAL - полный серийный номер usb устройства
    ID_SERIAL_SHORT - короткий серийный номер usb устройства
    ID_USB_INTERFACES
    ID_VENDOR
    ID_VENDOR_ENC
    ID_VENDOR_FROM_DATABASE = Alcor Micro Corp - берется из файла /var/lib/usbutils/usb.ids
    ID_VENDOR_ID = 058f
    MAJOR
    MINOR
    PRODUCT
    SUBSYSTEM = usb
    TYPE
    USEC_INITIALIZED
    net.ifnames
    
  3. Список свойств файла устройства usb (неполный)
    DEVLINKS
    DEVNAME = /dev/sdb
    DEVPATH
    DEVTYPE = disk
    ID_BUS = usb
    ID_FS_LABEL
    ID_FS_TYPE
    ID_FS_UUID
    ID_MODEL = Flash_Disk
    ID_MODEL_ID = 6387
    ID_PART_TABLE_TYPE = dos
    ID_SERIAL
    ID_SERIAL_SHORT
    ID_USB_DRIVER = usb-storage
    ID_VENDOR_ID = 058f
    SUBSYSTEM = block
