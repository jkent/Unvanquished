local function inspect(t, depth)
	local inspect = require 'tools.inspect'
	for line in inspect(t, depth):gmatch('[^\r\n]+') do
		print(line)
	end
end

table.inspect = inspect

