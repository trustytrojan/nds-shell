---@diagnostic disable: undefined-global

-- use rxi/json.lua, it's very small and works!
-- make sure it's in the same directory
local json = require('json')

local encoded = json.encode({ 1, 2, 3, { x = 10 } })
print('encoded: ' .. encoded)

local decoded = json.decode(encoded)
for key, value in ipairs(decoded) do
	print(key .. ': ' .. value)
end
