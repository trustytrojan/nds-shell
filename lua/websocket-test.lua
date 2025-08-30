---@diagnostic disable: undefined-global

local closed = false
local ws = WebSocket.new('wss://echo.websocket.events')
ws.connectTimeoutSeconds = 10
ws.caFile = 'tls-ca-bundle.pem'

ws.on_open = function()
	print('connected!')
	ws:send('hello')
	print("we sent 'hello'")
end

ws.on_message = function(msg)
	print('received message: ' .. msg)
	if msg == 'hello' then
		print('received hello back, closing')
		ws:close()
	end
end

ws.on_close = function(code, reason)
	closed = true
	print('ws closed!\ncode: ' .. tostring(code) .. '\nreason: ' .. reason)
end

ws:connect()

while libnds.pmMainLoop() and not closed do
	libnds.threadYield()
	ws:poll()
end
