-- Скрипт демонструє обробку стану кнопок.

-- Номера пінів кнопок, як вказано в прошивці Pixeler.
BTN_UP = 5
BTN_LEFT = 3
BTN_RIGHT = 4
BTN_EXIT = 2 

function update() -- Функція викликається автоматично контекстом кожен кадр.
	if input.is_pressed(BTN_EXIT) then 
        print("EXIT");
		context.exit()
    elseif input.is_pressed(BTN_UP) then -- Обробка стану is_pressed повинна завжди виконуватися ДО is_holded на одній кнопці.
        print("Кнопка \"вгору\" затиснута тривалий час");
    elseif input.is_released(BTN_UP) then
        print("Відпущено кнопку \"вгору\"");
    elseif input.is_holded(BTN_RIGHT) then
        print("Утримується кнопка \"вправо\"");
    end
end
