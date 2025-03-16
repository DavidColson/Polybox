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
        rotate(quat_to_euler(v.rotation))

		iter(k,v)
		if v.children ~= nil then
			iterate_scene(v.children, iter)
		end

		pop_matrix()
	end
end

local testModule = include("testModule.luau")

function start()
	-- global state
	state = {}

	testModule.message();

	print("Starting mesh test")

	copy("tank.scene", "tank_copy.scene")
	make_directory("moved", false)
	move("tank_copy.scene", "moved/tank_copy.scene")
	-- @todo: potentially you can call load on a directory
	-- it will put all the entries it can load into one table

	-- scene
	state.scene = load("tank.scene")
	
	-- meshes
	state.hangarMesh = load("hangarMesh.mesh")
	state.tankMesh = load("tankMesh.mesh")
	state.crateMesh = load("crateMesh.mesh")
	state.paletteMesh = load("paletteMesh.mesh")
	state.BlobShadow = load("BlobShadow.mesh")

	-- textures
	state.Shed = load("Shed.texture")
	state.Palette = load("Palette.texture")
	state.NewNewTexture = load("NewNewTexture.texture")
	state.blob = load("blob.texture")
end

function quat_to_euler(q:UserData): (number,number,number)
	local x = math.atan2(2* (q.w*q.x + q.y*q.z), 1 - 2*(q.x*q.x+q.y*q.y))
	local y = math.asin(2*(q.w*q.y-q.z*q.x))
	local z = math.atan2(2* (q.w*q.z + q.x*q.y), 1 - 2*(q.y*q.y+q.z*q.z))
	return x, y, z
end

function update()
    set_clear_color(0.25, 0.25, 0.25, 1.0)

    matrix_mode("Projection")
    perspective(320, 180, 1, 20, 60)

    matrix_mode("View")
    rotate(0, 3.6, 0)
    translate(1, -2.5, 6.5)

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

	iterate_scene(state.scene, function(name:string, obj: any)
		if obj.mesh == "BlobShadow" then
			local lateDrawObj = {obj=obj, mat=get_matrix()}
			table.insert(state.lateDraw, lateDrawObj)
			return
		end
		local texture = state[state[obj.mesh].texture]
		bind_texture(texture.data)

		begin_object_3d("Triangles")
		local vertices = state[obj.mesh].vertices
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

		local texture = state[state[v.obj.mesh].texture]
		bind_texture(texture.data)

		begin_object_3d("Triangles")
		local vertices = state[v.obj.mesh].vertices
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
