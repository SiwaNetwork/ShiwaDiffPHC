#!/bin/bash

echo "=== Тест GUI с графиками ==="
echo ""

echo "1. Запуск GUI..."
sudo ./shiwadiffphc-gui &
GUI_PID=$!
sleep 2

echo "2. Проверка процесса GUI..."
if ps -p $GUI_PID > /dev/null; then
    echo "✅ GUI запущен (PID: $GUI_PID)"
else
    echo "❌ GUI не запущен"
    exit 1
fi

echo ""
echo "3. Инструкции для тестирования:"
echo "   - Откройте GUI (должно появиться окно)"
echo "   - Выберите 2 или более PTP устройства"
echo "   - Нажмите 'Начать измерение'"
echo "   - Перейдите на вкладку 'Графики'"
echo "   - Проверьте логи в вкладке 'Log'"
echo ""
echo "4. Для остановки GUI:"
echo "   sudo kill $GUI_PID"
echo ""
echo "5. Проверка логов в реальном времени:"
echo "   tail -f /tmp/shiwadiffphc_gui.log 2>/dev/null || echo 'Логи не найдены'" 