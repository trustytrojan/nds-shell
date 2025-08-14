---@diagnostic disable: undefined-global

local done = false
local started, msg = fetch('https://api.trustytrojan.dev/ping', {}, function(code, body, err)
	print(string.format('code: %q\nbody: %q\nerr: %q', code, body, err))
	done = true
end)

if not started then
	printerr(msg)
	return
end

print('request started...')

-- wait for the request to complete
while libnds.pmMainLoop() and not done do
	libnds.threadYield()
end

print('...request finished')
