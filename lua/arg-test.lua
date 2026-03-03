---@diagnostic disable: undefined-global

if #arg == 0 then
	print('no args passed!')
end

for key, value in ipairs(arg) do
	print(key .. ': ' .. value)
end
