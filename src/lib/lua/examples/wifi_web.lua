local ssid = "YOUR_SSID"
local pwd = "YOUR_PWD"

-- 

local wifi = require("wifi")
local web = require("web")
local BTN_EXIT<const> = 2
local BTN_UP<const> = 5

-- 

local wifi_isnt_en_err<const> = "wifi не увімкнено"
local server_resp_err<const> = "Сервер повернув помилку"

-- 
local server_get = "http://httpbin.org/get";
local server_post = "http://httpbin.org/post";
-- 

function disableWifi()
    wifi.disable();
    print("WiFi вимкнено")
end

function freeRes()
    unrequire("web")
    web = nil

    unrequire("wifi")
    wifi = nil

    print("Ресурси wifi та web звільнено")
end

function getExample()
    if not wifi.isEnabled() then
        print(wifi_isnt_en_err)
        return
    end

    print("Виконую GET запит")

    local param_str = "?key1=value1&key2=value2"
    local resp_str = web.get(server_get .. param_str)

    if resp_str == "" then
        print(server_resp_err)
    else
        print(resp_str)
    end
end

function postExample()
    if not wifi.isEnabled() then
        print(wifi_isnt_en_err)
        return
    end

    print("Виконую POST запит")

    local post_data = "Lorem ipsum"
    local resp_str = web.post(server_post, post_data)

    if resp_str == "" then
        print(server_resp_err)
    else
        print(resp_str)
    end
end

function postJsonExample()
    if not wifi.isEnabled() then
        print(wifi_isnt_en_err)
        return
    end

    print("Виконую POST-json запит")

    local json_data = "{\"device\":\"ESP32\",\"temperature\":25.5,\"humidity\":60,\"status\":\"active\"}"
    local resp_str = web.jsonPost(server_post, json_data)

    if resp_str == "" then
        print(server_resp_err)
    else
        print(resp_str)
    end
end

function postFormExample()
    if not wifi.isEnabled() then
        print(wifi_isnt_en_err)
        return
    end

    print("Виконую POST-form запит")

    local form_data = "username=esp32user&password=secret123&temperature=25.5"
    local resp_str = web.formPost(server_post, form_data)

    if resp_str == "" then
        print(server_resp_err)
    else
        print(resp_str)
    end
end

function badReqExample()
    if not wifi.isEnabled() then
        print(wifi_isnt_en_err)
        return
    end

    print("Виконую некоректний запит")

    local resp_str = web.get("http://some_incorrect_url.com")

    if resp_str == "" then
        print(server_resp_err)
    else
        print(resp_str)
    end
end

-- 

function runTest()
    if not wifi.enable() then
        print("Помилка увімкнення WiFi")
    else
        print("WiFi увімкнено")

        wifi.tryConnectTo(ssid, pwd)

        while wifi.isBusy() do
            mcu.delay(1)
        end

        if not wifi.isConnected() then
            print("Помилка підключення до " .. ssid)
        else
            print("Підключено до " .. ssid)
            print("Виділений IP: " .. wifi.getLocalIP())

            mcu.delay(1)

            getExample()
            mcu.delay(1)

            postExample()
            mcu.delay(1)

            postJsonExample()
            mcu.delay(1)

            postFormExample()
            mcu.delay(1)

            badReqExample()
            mcu.delay(1)

            -- disableWifi()
            freeRes()
        end
    end
end

function update()
    if input.is_pressed(BTN_EXIT) then
        input.lock(BTN_EXIT, 1000)
        context.exit()
        return;
    end

    if input.is_released(BTN_UP) then
        input.lock(BTN_UP, 250)
        wifi.setPower(2) -- 0-2 де 2 Max
        runTest()
    end
end
