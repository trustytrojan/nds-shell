---@diagnostic disable: undefined-global

-- Simple WebSocket connection test
local done = false

local ws, err = websocket('wss://echo.websocket.org/', 
    {},
    function(info, error_msg)
        if error_msg then
            printerr('WebSocket error: ' .. error_msg)
            done = true
            return
        end
        
        if info then
            print('WebSocket connected!')
            print('URL: ' .. info.url)
            print('Status: ' .. info.status)
            if info.note then
                print('Note: ' .. info.note)
            end
        end
        
        done = true
    end
)

if not ws then
    printerr('Failed to create WebSocket: ' .. tostring(err))
    return
end

print('WebSocket test started...')

-- Wait for the test to complete
while libnds.pmMainLoop() and not done do
    libnds.threadYield()
end

print('...WebSocket test finished')