function main()
	updateAttack()
	setExecNum("interacted", getExecNum("interacted") + getExecNum("interacting"))
end

function updateAttack()
	if (getExecNum("attacked") == 1) then
		path = getExecStr("attackSound")
		posX = getNum("lvl-current-camX")
		posY = getNum("lvl-current-camY")
		distance = 1000 
		exec("world playSound "..path.." "..posX.." "..posY.." "..distance)
	end
end