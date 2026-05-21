1. Встановити esptool
sudo apt install pipx
pipx ensurepath
pipx install esptool
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
esptool version

2. Підключити плату до ПК в режимі прошивки.

3. Виконати команду у терміналі:
esptool --chip esp32p4 flash-id

та переконатися, що МК підключено і його пам'ять читається.

4. Виконати команду у терміналі:
esptool --chip esp32p4 erase-flash 

5. Відкрити термінал у поточному каталозі та виконати команду:
esptool --chip esp32p4 --baud 460800 --before default-reset --after hard-reset write-flash -z 0x0 pixeler_WT9932P4.bin

6. Дочекатися завантаження прошивки та автоматичного перезавантаження плати.
