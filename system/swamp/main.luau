
function start()
	state = {}

	state.scene = load("swamp.scene")
	state.mesh = load("mesh0.mesh")
	state.texture = load("image0.texture")
end

function update()
    set_clear_color(0.25, 0.25, 0.25, 1.0)

    matrix_mode("Projection")
    perspective(320, 180, 1, 20, 60)

    matrix_mode("View")
	-- we need to sort this shit out
	rotate(0, -1, 0)
	translate(-9, -2, -10)

    normals_mode("Custom")
    enable_lighting(true)
    ambient(0.4, 0.4, 0.4)
    light(0, -1, 1, 0, 1, 1, 1)

    enable_fog(true)
    set_fog_start(3.0)
    set_fog_end(20.0)
	set_fog_color(0.25, 0.25, 0.25)

	local floatsPerVertex = 12
	bind_texture(state.texture.data)

	begin_object_3d("Triangles")
	local vertices = state.mesh.vertices
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
