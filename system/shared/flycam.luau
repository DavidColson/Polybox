local module = {}

module.camPitch = 0
module.camYaw = 0
module.camX = 0
module.camY = 0
module.camZ = 0

function module.updateCamera(deltaTime: number)
	module.camPitch = module.camPitch + deltaTime * get_axis(Axis.right_y)
	module.camYaw = module.camYaw + deltaTime * get_axis(Axis.right_x)

	-- note this is maybe better generalized as us taking the basis vectors directly out of the
	-- rotation matrix, multiplying those by the input scalars and adding them all together

	-- forward back camera movement
	local speed = 5

	local forward = get_axis(Axis.left_y) * speed
	module.camZ = module.camZ - forward * math.cos(module.camYaw) * deltaTime
	module.camX = module.camX + forward * math.sin(module.camYaw) * deltaTime

	-- right left camera movement
	local right = get_axis(Axis.left_x) * speed
	module.camZ = module.camZ - right * math.sin(module.camYaw) * deltaTime
	module.camX = module.camX - right * math.cos(module.camYaw) * deltaTime

	-- up down camera movement
	local up = get_axis(Axis.trigger_right) * speed
	local down = get_axis(Axis.trigger_left) * speed
	module.camY = module.camY - deltaTime * up
	module.camY = module.camY + deltaTime * down
end

return module
