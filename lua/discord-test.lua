---@diagnostic disable: undefined-global

-- remember ctx is an **object** of type CommandContext, and GetEnv is a method on said object...
local token = ctx:GetEnv('TOKEN', '')
local channel = ctx:GetEnv('CHANNEL', '')
if string.len(token) == 0 or string.len(channel) == 0 then
	printerr('\x1b[91mTOKEN and/or CHANNEL env var not set!\x1b[39m')
	return
end

local done = false
local started, msg = fetch(
	'https://discord.com/api/v10/channels/' .. channel .. '/messages',
	{
		method = 'POST',
		headers = {
			['Content-Type'] = 'application/json',
			Authorization = token
		},
		body = '{"content":"hi from nds-shell"}'
	},
	function(code, body, err)
		print(string.format('code: %q\nbody: %q\nerr: %q', code, body, err))
		done = true
	end
)

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
