local BTN_EXIT <const> = 2

local FILE_PATH <const> = "/test.txt"

initType("IWidgetContainer")

local main_layout = context.getLayout() 
main_layout:setBackColor(COLOR_GREY) 

local SD = require("sd")

print("Запис в файл")
SD.write(FILE_PATH, "Lorem ipsum dolor sit amet")

print("Просте читання")
local file_size = SD.fileSize(FILE_PATH)
print("File size:")
print(file_size)
local read_str = SD.read(FILE_PATH, file_size)
print(read_str)
read_str = ""

print("Читання через вказівник на файл")
local file_d = SD.open(FILE_PATH, "r");

if file_d == nil then
  print("Помилка відкриття файлу")
else
  print("Перші два байти в файлі:")
  read_str = SD.readFrom(file_d, 2)
  print(read_str)
  print("Наступні 3 байти:")
  read_str = SD.readFrom(file_d, 3, 2)
  print(read_str)
  SD.close(file_d)
end

unrequire("sd")
sd = nil

print("Ресурси SD повністю звільнено")

function update()
    if input.is_pressed(BTN_EXIT) then
        input.lock(BTN_EXIT, 1000)
		context.exit()
        return;
	end
end