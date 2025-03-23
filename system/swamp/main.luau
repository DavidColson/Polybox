
function start()
	state = {}

	state.mouseRelativeMode = true
	state.camPitch = 0
	state.camYaw = 0
	state.camX = 0
	state.camY = 0
	state.camZ = 0
	state.scene = load("swamp.scene")

	enable_mouse_relative_mode(state.mouseRelativeMode)
end

function update(deltaTime: number)
    set_clear_color(0.25, 0.25, 0.25, 1.0)

	state.camPitch = state.camPitch + deltaTime * get_axis(Axis.right_y)
	state.camYaw = state.camYaw + deltaTime * get_axis(Axis.right_x)

	-- note this is maybe better generalized as us taking the basis vectors directly out of the
	-- rotation matrix, multiplying those by the input scalars and adding them all together

	-- forward back camera movement
	local speed = 5

	local forward = get_axis(Axis.left_y) * speed
	state.camZ = state.camZ - forward * math.cos(state.camYaw) * deltaTime
	state.camX = state.camX + forward * math.sin(state.camYaw) * deltaTime

	-- right left camera movement
	local right = get_axis(Axis.left_x) * speed
	state.camZ = state.camZ - right * math.sin(state.camYaw) * deltaTime
	state.camX = state.camX - right * math.cos(state.camYaw) * deltaTime

	-- up down camera movement
	local up = get_axis(Axis.trigger_right) * speed
	local down = get_axis(Axis.trigger_left) * speed
	state.camY = state.camY - deltaTime * up
	state.camY = state.camY + deltaTime * down

	if get_key_down(Key.escape) then 
		state.mouseRelativeMode = not state.mouseRelativeMode
		enable_mouse_relative_mode(state.mouseRelativeMode)
	end

    matrix_mode("Projection")
    perspective(320, 180, 1, 20, 60)

    matrix_mode("View")
	rotate(state.camPitch, 1, 0, 0)
	rotate(state.camYaw, 0, 1, 0)
	translate(state.camX, state.camY, state.camZ)

    normals_mode("Custom")
    enable_lighting(true)
    ambient(0.4, 0.4, 0.4)
    light(0, -1, 1, 0, 1, 1, 1)

    enable_fog(true)
    set_fog_start(3.0)
    set_fog_end(40.0)
	set_fog_color(0.25, 0.25, 0.25)

	local texture = state.scene.textures.image0
	local mesh = state.scene.meshes.mesh0

	local floatsPerVertex = 12
	bind_texture(texture.data)

	begin_object_3d("Triangles")
	local vertices = mesh.vertices
	for i = 0, vertices:size()-1, floatsPerVertex do
		-- 0-2 is position
		-- 3-6 is color
		-- 7-8 is texcoord
		-- 9-11 is normal
		normal(vertices:get(i+9, 3))
		color(vertices:get(i+3, 4))
		texcoord(vertices:get(i+7, 2))
		vertex(vertices:get(i, 3))
	end
	end_object_3d()
end

function close()
end
