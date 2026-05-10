# Скрипт для парсингу інформації про тип плитки, що зберігається в файлі набору плиток *.tsx з програми Tiled.
# Скрипт перетворює дані в формат id=тип

import xml.etree.ElementTree as ET

TYPE_PROP_NAME = "tp"           # ім'я властивості, яку шукаємо
INPUT_FILE = "test.tsx"         # шлях до XML-файлу набору плиток
OUTPUT_FILE = "tiles.meta"      # куди зберегти результат

tree = ET.parse(INPUT_FILE)
root = tree.getroot()

missing = []

with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
    for tile in root.findall("tile"):
        tid = tile.get("id")
        prop_value = None

        props = tile.find("properties")
        if props is not None:
            for p in props.findall("property"):
                if p.get("name") == TYPE_PROP_NAME:
                    prop_value = (p.get("value") or "").strip()
                    break

        if prop_value:
            out.write(f"{tid}={prop_value}\n")
        else:
            missing.append(tid)

if missing:
    print(f"Відсутня властивість '{TYPE_PROP_NAME}' у плиток з id:")
    print(", ".join(map(str, missing)))
else:
    print(f"Усі плитки мають властивість '{TYPE_PROP_NAME}'.")

print(f"Файл '{OUTPUT_FILE}' успішно створено.")
