local function printTable( t:any)
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

local state = {}

-- iterate over a heirarchy of objects (with children table) as though it was a flat array
-- concatenating transforms of parents and children as you go
function iterate_scene(scene:any, iter: (string,any)->())
	for k, v in scene do
        MatrixMode("Model")
		PushMatrix()

		if v.position ~= nil then Translate(v.position.x, v.position.y, v.position.z) end
		if v.scale ~= nil then Scale(v.scale.x, v.scale.y, v.scale.z) end
        if v.rotation ~= nil then Rotate(quatToEuler(v.rotation)) end

		iter(k,v)
		if v.children ~= nil then
			iterate_scene(v.children, iter)
		end

		PopMatrix()
	end
end

function Start()
	print("Starting mesh test")

	-- @todo: potentially you can call load on a directory
	-- it will put all the entries it can load into one table
	-- will also help when we have VFS, so you need smaller paths

	-- scene
	state.scene = load("systemroot/tank_demo/tank.scene")
	
	-- meshes
	state.hangarMesh = load("systemroot/tank_demo/hangarMesh.mesh")
	state.tankMesh = load("systemroot/tank_demo/tankMesh.mesh")
	state.crateMesh = load("systemroot/tank_demo/crateMesh.mesh")
	state.paletteMesh = load("systemroot/tank_demo/paletteMesh.mesh")
	state.BlobShadow = load("systemroot/tank_demo/BlobShadow.mesh")

	-- textures
	state.Shed = load("systemroot/tank_demo/Shed.texture")
	state.Palette = load("systemroot/tank_demo/Palette.texture")
	state.NewNewTexture = load("systemroot/tank_demo/NewNewTexture.texture")
	state.blob = load("systemroot/tank_demo/blob.texture")

	-- testing matrices
	local mat = buffer("f32", 4, 4)
	mat:set2D(0, 0,
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1)

	print(mat)

	local vertices = state.tankMesh.vertices
	local floatsPerVertex = 12
	local nVertices = vertices:size() / floatsPerVertex
	print("nVertices " .. nVertices)
	print("vertices:size " .. vertices:size())
end

function quatToEuler(q:{x:number,y:number,z:number,w:number}): (number,number,number)
	local x = math.atan2(2* (q.w*q.x + q.y*q.z), 1 - 2*(q.x*q.x+q.y*q.y))
	local y = math.asin(2*(q.w*q.y-q.z*q.x))
	local z = math.atan2(2* (q.w*q.z + q.x*q.y), 1 - 2*(q.y*q.y+q.z*q.z))
	return x, y, z
end

function Update()
    SetClearColor(0.25, 0.25, 0.25, 1.0)

    MatrixMode("Projection")
    Perspective(320, 180, 1, 20, 60)

    MatrixMode("View")
    Rotate(0, 3.6, 0)
    Translate(1, -2.5, 6.5)

    NormalsMode("Custom")
    EnableLighting(true)
    Ambient(0.4, 0.4, 0.4)
    Light(0, -1, 1, 0, 1, 1, 1)

	-- couple of interesting issues
	-- we have parent child relationships now in the scene
	-- but we don't have any global matrix calculation?
	-- how do you draw the scene now? Can't loop through all objects

	-- start simple iterate the scene
	-- can easily draw all the top level items
	-- if there are children iterate the children
	-- need to create a matrix from their transform data
	-- pass it into recursive draw object function

	-- how do we order the drawing? We want the blob shadows to be drawn second

	-- two ideas
	-- create an iterate scene function which takes a function to call for every object
	-- passes the object and it's parent transform matrix
	-- can again then just iterate twice

	-- better idea for blobs
	-- as you encounter them in your iteration, put them in a passTwo table in state
	-- iterate that at the end

	local floatsPerVertex = 12

	iterate_scene(state.scene, function(name:string, obj: any)
		local texture = state[state[obj.mesh].texture]
		BindTexture(texture.data)
		BeginObject3D("Triangles")
		local vertices = state[obj.mesh].vertices
		for i = 0, vertices:size(), floatsPerVertex do
			-- 0-2 is position
			-- 3-6 is color
			-- 7-8 is texcoord
			-- 9-11 is normal
			Normal(vertices:get(i+9, 3))
			Color(vertices:get(i+3, 4))
			TexCoord(vertices:get(i+7, 2))
			Vertex(vertices:get(i, 3))
		end
		EndObject3D()
	end)
end

function End()
	print("Ending mesh test")
end
