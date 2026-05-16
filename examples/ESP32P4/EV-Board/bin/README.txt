1. Виконати команду у терміналі: 
sudo apt install esptool

2. Підключити плату до ПК в режимі прошивки.

3. Виконати команду у терміналі:
esptool --chip esp32p4 flash_id

та переконатися, що МК підключено і його пам'ять читається.

4. Виконати команду у терміналі:
esptool --chip esp32p4 erase_flash 

5. Відкрити термінал у поточному каталозі та виконати команду:
esptool --chip esp32p4 --baud 460800 --before default_reset --after hard_reset write_flash -z 0x0 pixeler_p4fev.bin

6. Дочекатися завантаження прошивки та автоматичного перезавантаження плати.
