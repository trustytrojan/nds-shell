---@diagnostic disable: undefined-global

-- Discord WebSocket Gateway connection test
-- Set your Discord bot token in the DISCORD_TOKEN environment variable
local token = ctx:GetEnv('DISCORD_TOKEN', '')
if string.len(token) == 0 then
    printerr('\x1b[91mDISCORD_TOKEN env var not set!\x1b[39m')
    printerr('Set it with: env DISCORD_TOKEN="Bot YOUR_TOKEN_HERE"')
    printerr('This test only establishes the connection - full Discord protocol requires WebSocket message support.')
    return
end

local done = false

-- Discord Gateway URL
local gateway_url = 'wss://gateway.discord.gg/?v=10&encoding=json'

local ws, err = websocket(gateway_url,
    {
        headers = {
            Authorization = token
        }
    },
    function(info, error_msg)
        if error_msg then
            printerr('Discord WebSocket error: ' .. error_msg)
            done = true
            return
        end
        
        if info then
            print('Connected to Discord Gateway!')
            print('URL: ' .. info.url)
            print('Status: ' .. info.status)
            if info.note then
                print('Note: ' .. info.note)
            end
            print('')
            print('Connection established successfully!')
            print('Full Discord bot functionality would require:')
            print('  1. Sending IDENTIFY payload')
            print('  2. Handling HELLO and heartbeat')
            print('  3. Processing events like READY')
            print('  4. Implementing message send/receive')
            print('')
            print('This demonstrates that WebSocket connections to Discord work.')
        end
        
        done = true
    end
)

if not ws then
    printerr('Failed to connect to Discord Gateway: ' .. tostring(err))
    return
end

print('Discord Gateway connection test started...')

-- Wait for the test to complete
while libnds.pmMainLoop() and not done do
    libnds.threadYield()
end

print('...Discord Gateway test finished')