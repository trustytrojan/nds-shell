---@diagnostic disable: undefined-global
libnds.setBrightness(1, -8)
libnds.setBrightness(2, 8)

print('hello world')
print(string.format('is shell focused: %q', ctx.shell.focused))
print('arguments: ')
for i = 1, #ctx.args do
	local arg = ctx.args[i]
	print(string.format('arg %d: %q', i, arg))
end

local help_output = ctx.shell:run('help')
print('help output: ' .. help_output)
