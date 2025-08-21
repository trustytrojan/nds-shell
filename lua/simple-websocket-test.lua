---@diagnostic disable: undefined-global

-- Basic WebSocket connection test using a public echo server
local done = false

print('Testing WebSocket connection to echo.websocket.org...')

local ws, err = websocket('ws://echo.websocket.org/', 
    {
        timeout = 15  -- 15 second timeout
    },
    function(info, error_msg)
        if error_msg then
            printerr('WebSocket connection failed: ' .. error_msg)
        else
            print('SUCCESS: WebSocket connection established!')
            print('  URL: ' .. info.url)
            print('  Status: ' .. info.status)
            if info.note then
                print('  Note: ' .. info.note)
            end
            print('')
            print('This confirms that nds-shell can establish WebSocket connections.')
            print('For Discord bots or advanced use cases, you would need to implement')
            print('the WebSocket protocol (handshake, framing, etc.) on top of this')
            print('basic connection capability.')
        end
        
        done = true
    end
)

if not ws then
    printerr('Failed to create WebSocket: ' .. tostring(err))
    return
end

-- Wait for the test to complete
while libnds.pmMainLoop() and not done do
    libnds.threadYield()
end

print('WebSocket test completed.')