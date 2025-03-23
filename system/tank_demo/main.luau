local function _printTable( t:any)
    local printTable_cache = {}
    local function sub_printTable( t:any, indent:any )
 
        if ( printTable_cache[tostring(t)] ) then
            print( indent .. "*" .. tostring(t) )
        else
            printTable_cache[tostring(t)] = true
            if ( type( t ) == "table" ) then
                for pos,val in pairs( t ) do
                    if ( type(val) == "table" ) then
                        print( indent .. "[" .. pos .. "] => " .. tostring( t ).. " {" )
                        sub_printTable( val, indent .. string.rep( " ", string.len(pos)+8 ) )
                        print( indent .. string.rep( " ", string.len(pos)+6 ) .. "}" )
                    elseif ( type(val) == "string" ) then
                        print( indent .. "[" .. pos .. '] => "' .. val .. '"' )
                    else
                        print( indent .. "[" .. pos .. "] => " .. tostring(val) )
                    end
                end
            else
                print( indent..tostring(t) )
            end
        end
    end
 
    if ( type(t) == "table" ) then
        print( tostring(t) .. " {" )
        sub_printTable( t, "  " )
        print( "}" )
    else
        sub_printTable( t, "  " )
    end

end

-- iterate over a heirarchy of objects (with children table) as though it was a flat array
-- concatenating transforms of parents and children as you go
function iterate_scene(scene:any, iter: (string,any)->())
	for k, v in scene do
        matrix_mode("Model")
		push_matrix()

		translate(v.position.x, v.position.y, v.position.z) 
		scale(v.scale.x, v.scale.y, v.scale.z) 

        rotate(quat_to_angle_axis(v.rotation))

		iter(k,v)
		if v.children ~= nil then
			iterate_scene(v.children, iter)
		end

		pop_matrix()
	end
end

local flycam = include("flycam.luau")

function start()
	-- global state
	state = {}

	flycam.camYaw = 3.6
	flycam.camX = 1
	flycam.camY = -2.5
	flycam.camZ = 6.5

	-- scene
	state.scene = load("tank.scene")
end

function quat_to_angle_axis(q:UserData): (number,number,number,number)
	local angle = 2*math.acos(q.w)
	local sqrtW = math.sqrt(1-q.w*q.w)
	local x = q.x / sqrtW
	local y = q.y / sqrtW
	local z = q.z / sqrtW
	if sqrtW <= 0.001 then 
		x = q.x
		y = q.y
		z = q.z
	end
	return angle, x, y, z
end

function update(deltaTime: number)
    set_clear_color(0.25, 0.25, 0.25, 1.0)

	flycam.updateCamera(deltaTime)

    matrix_mode("Projection")
    perspective(320, 180, 1, 20, 60)

    matrix_mode("View")
	rotate(flycam.camPitch, 1, 0, 0)
	rotate(flycam.camYaw, 0, 1, 0)
	translate(flycam.camX, flycam.camY, flycam.camZ)

    normals_mode("Custom")
    enable_lighting(true)
    ambient(0.4, 0.4, 0.4)
    light(0, -1, 1, 0, 1, 1, 1)

    enable_fog(true)
    set_fog_start(3.0)
    set_fog_end(15.0)
	set_fog_color(0.25, 0.25, 0.25)

	local floatsPerVertex = 12
	state.lateDraw = {}

	iterate_scene(state.scene.scene, function(name:string, obj: any)
		if obj.mesh == "BlobShadow" then
			local lateDrawObj = {obj=obj, mat=get_matrix()}
			table.insert(state.lateDraw, lateDrawObj)
			return
		end
		local texture = state.scene.textures[state.scene.meshes[obj.mesh].texture]
		bind_texture(texture.data)

		begin_object_3d("Triangles")
		local vertices = state.scene.meshes[obj.mesh].vertices
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
	end)

	-- the blob shadows have transpancy, so we must draw them after everything else
	-- from front to back
	local frontToBack = function(a, b)
		local aPos = vec(a.obj.position.x, a.obj.position.y, a.obj.position.z)
		local bPos = vec(b.obj.position.x, b.obj.position.y, b.obj.position.z)
		local camPos = vec(-1, 2.5, -6.5)
		local aCamDist = camPos:distance(aPos)
		local bCamDist = camPos:distance(bPos)
		return aCamDist > bCamDist
	end
	table.sort(state.lateDraw, frontToBack)

	for k, v in state.lateDraw do
		load_matrix(v.mat)

		local texture = state.scene.textures[state.scene.meshes[v.obj.mesh].texture]
		bind_texture(texture.data)

		begin_object_3d("Triangles")
		local vertices = state.scene.meshes[v.obj.mesh].vertices
		for i = 0, vertices:size()-1, floatsPerVertex do
			normal(vertices:get(i+9, 3))
			color(vertices:get(i+3, 4))
			texcoord(vertices:get(i+7, 2))
			vertex(vertices:get(i, 3))
		end
		end_object_3d()
	end
end

function close()
	print("Ending mesh test")
end
