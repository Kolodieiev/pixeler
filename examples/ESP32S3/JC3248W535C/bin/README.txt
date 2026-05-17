1. Виконати команду у терміналі: 
sudo apt install esptool

2. Підключити плату до ПК в режимі прошивки.

3. Виконати команду у терміналі:
esptool --chip esp32s3 flash_id

та переконатися, що МК підключено і його пам'ять читається.

4. Виконати команду у терміналі:
esptool --chip esp32s3 erase_flash 

5. Відкрити термінал у поточному каталозі та виконати команду:
python -m esptool --chip esp32s3 --baud 460800 --before default-reset --after hard-reset write-flash -z 0x0 pixeler4_JC3248W535C.bin

6. Дочекатися завантаження прошивки та автоматичного перезавантаження плати.
