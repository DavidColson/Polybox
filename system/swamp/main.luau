local flyCam = include("flycam.luau")

function start()
	state = {}

	state.mouseRelativeMode = true
	state.scene = load("swamp.scene")

	enable_mouse_relative_mode(state.mouseRelativeMode)
end

function update(deltaTime: number)
    set_clear_color(0.25, 0.25, 0.25, 1.0)
	set_cull_mode(2);

	if get_key_down(Key.escape) then 
		state.mouseRelativeMode = not state.mouseRelativeMode
		enable_mouse_relative_mode(state.mouseRelativeMode)
	end

	if state.mouseRelativeMode then
		flyCam.updateCamera(deltaTime)
	end

    matrix_mode("Projection")
    perspective(320, 180, 0.001, 20, 60)

    matrix_mode("View")
	rotate(flyCam.camPitch, 1, 0, 0)
	rotate(flyCam.camYaw, 0, 1, 0)
	translate(flyCam.camX, flyCam.camY, flyCam.camZ)

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
