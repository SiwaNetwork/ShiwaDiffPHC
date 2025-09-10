# 🔧 Руководство по устранению неполадок ShiwaDiffPHC

## 🚨 Частые проблемы и решения

### 1. Отказано в доступе (Permission Denied)

**Симптомы:**
```
Error: Root privileges required to access PTP devices
```

**Решение:**
```bash
# Запустите с привилегиями sudo
sudo ./shiwadiffphc-cli -d 0 -d 1
sudo ./shiwadiffphc-gui
```

**Причина:** PTP устройства требуют привилегий root для доступа.

---

### 2. PTP устройства не найдены

**Симптомы:**
```
Error: No PTP devices found in the system
```

**Диагностика:**
```bash
# Проверьте доступные PTP устройства
ls /dev/ptp*

# Проверьте загруженные модули ядра
lsmod | grep ptp

# Проверьте поддержку PTP в ядре
grep PTP /boot/config-$(uname -r)
```

**Решения:**

#### 2.1. Включить поддержку PTP в ядре
```bash
# Для Ubuntu/Debian
sudo apt-get install linux-headers-$(uname -r)
sudo modprobe ptp

# Для RHEL/CentOS
sudo yum install kernel-devel
sudo modprobe ptp
```

#### 2.2. Проверить аппаратуру
```bash
# Проверьте сетевые интерфейсы с поддержкой PTP
ethtool -T eth0  # замените eth0 на ваш интерфейс

# Ищите строки типа:
# PTP Hardware Clock: 0
# Hardware Transmit Timestamp Modes: off
```

#### 2.3. Загрузить драйверы
```bash
# Для Intel сетевых карт
sudo modprobe igb
sudo modprobe ixgbe

# Для других производителей
sudo modprobe e1000e
sudo modprobe r8169
```

---

### 3. Ошибки компиляции

**Симптомы:**
```
fatal error: linux/ptp_clock.h: No such file or directory
```

**Решение:**
```bash
# Установите заголовочные файлы ядра
sudo apt-get install linux-headers-$(uname -r)  # Ubuntu/Debian
sudo yum install kernel-devel                   # RHEL/CentOS
```

---

### 4. Qt5 не найдена (для GUI)

**Симптомы:**
```
Warning: Qt5 development libraries not found. GUI will not be built.
```

**Решение:**
```bash
# Ubuntu/Debian
sudo apt-get install qtbase5-dev qt5-qmake

# RHEL/CentOS
sudo yum install qt5-qtbase-devel

# Fedora
sudo dnf install qt5-qtbase-devel
```

---

### 5. GUI не запускается

**Симптомы:**
- GUI запускается и сразу закрывается
- Ошибки X11/Wayland

**Диагностика:**
```bash
# Проверьте переменную DISPLAY
echo $DISPLAY

# Проверьте доступность X11
xrandr --query

# Запустите с отладкой
QT_DEBUG_PLUGINS=1 sudo ./shiwadiffphc-gui
```

**Решения:**

#### 5.1. Проблемы с X11
```bash
# Установите X11 сервер (если отсутствует)
sudo apt-get install xorg

# Проверьте права доступа
xhost +local:root
```

#### 5.2. Проблемы с Wayland
```bash
# Переключитесь на X11
export GDK_BACKEND=x11
sudo ./shiwadiffphc-gui
```

---

### 6. Огромные числа в результатах

**Симптомы:**
```
ptp1    -1757509835416646768    0
```

**Объяснение:** Это **нормальное поведение** для PTP устройств, которые не синхронизированы с системным временем.

**Что важно:**
- **Размах (range)** - показывает стабильность (должен быть в разумных пределах)
- **Стандартное отклонение** - показывает качество синхронизации
- **Медиана и среднее** - помогают оценить точность

**Пример нормальных результатов:**
```
Размах: 901950378 нс (≈ 902 мс)
Станд. отклонение: 1944457556237838848.0 нс
```

---

### 7. Низкая производительность

**Симптомы:**
- Медленные измерения
- Высокая загрузка CPU

**Оптимизация:**
```bash
# Уменьшите количество выборок
./shiwadiffphc-cli -d 0 -d 1 -s 5

# Увеличьте задержку между измерениями
./shiwadiffphc-cli -d 0 -d 1 -l 500000

# Используйте меньше итераций для тестирования
./shiwadiffphc-cli -d 0 -d 1 -c 10
```

---

### 8. Проблемы с сетью

**Симптомы:**
- Нестабильные результаты
- Большие разбросы в измерениях

**Диагностика:**
```bash
# Проверьте задержку сети
ping -c 10 <ptp_server>

# Проверьте качество PTP синхронизации
pmc -u -b 0 'GET CURRENT_DATA_SET'
```

**Решения:**
- Используйте выделенную сеть для PTP
- Настройте QoS для PTP трафика
- Проверьте настройки PTP master/slave

---

## 🔍 Диагностические команды

### Проверка системы
```bash
# Информация о системе
uname -a
cat /proc/version

# Проверка PTP устройств
ls -la /dev/ptp*
cat /sys/class/ptp/ptp*/name

# Проверка сетевых интерфейсов
ip link show
ethtool -i eth0
```

### Проверка ShiwaDiffPHC
```bash
# Список доступных устройств
sudo ./shiwadiffphc-cli --list

# Информация об устройствах
sudo ./shiwadiffphc-cli --info

# Тестовое измерение
sudo ./shiwadiffphc-cli -d 0 -d 1 -c 5 --verbose
```

---

## 📞 Получение помощи

### Логи и отладочная информация
```bash
# Соберите информацию для отчета об ошибке
sudo ./shiwadiffphc-cli --list > system_info.txt
sudo ./shiwadiffphc-cli --info >> system_info.txt
uname -a >> system_info.txt
ls -la /dev/ptp* >> system_info.txt
```

### Сообщество
- **GitHub Issues**: [Создать issue](https://github.com/SiwaNetwork/ShiwaDiffPHC/issues)
- **Документация**: [README.md](README.md)
- **План развития**: [ROADMAP.md](ROADMAP.md)

---

## 📋 Чек-лист для диагностики

- [ ] Запущено с sudo?
- [ ] PTP устройства доступны (`ls /dev/ptp*`)?
- [ ] Заголовочные файлы ядра установлены?
- [ ] Qt5 установлена (для GUI)?
- [ ] X11/Wayland работает?
- [ ] Сетевое оборудование поддерживает PTP?
- [ ] PTP драйверы загружены?

---

*Последнее обновление: $(date)*
