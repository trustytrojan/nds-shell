---@diagnostic disable: undefined-global

local prompt = CliPrompt.new()
prompt.prompt = 'myluascript> '
prompt.ostr = ctx.out
prompt:printFullPrompt(false)
prompt:prepareForNextLine()

while libnds.pmMainLoop() do
	libnds.threadYield()

	if not ctx.shell.focused then
		goto continue
	end

	prompt:update()

	if prompt.enterPressed then
		print('you entered: ' .. prompt.input)
		prompt:printFullPrompt(false)
		prompt:prepareForNextLine()
	end

	if prompt.foldPressed then
		break
	end

	::continue::
end
