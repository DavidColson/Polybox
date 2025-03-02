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

function Start()
	print("Starting mesh test")

	state.crate = load("systemroot/tank_demo/crateMesh.mesh")
	state.texture = load("systemroot/tank_demo/shed.texture")
	printTable(state.texture)

	local vertices = state.crate.vertices
	local floatsPerVertex = 12
	local nVertices = vertices:size() / floatsPerVertex
	print("nVertices " .. nVertices)
	print("vertices:size " .. vertices:size())
end

function Update()
    SetClearColor(0.25, 0.25, 0.25, 1.0)

    MatrixMode("Projection")
    Perspective(320, 180, 1, 20, 60)

    MatrixMode("View")
    Translate(0, 0, -3)

    NormalsMode("Custom")
    EnableLighting(true)
    Ambient(0.4, 0.4, 0.4)
    Light(0, -1, 1, 0, 1, 1, 1)

	MatrixMode("Model")
	Identity()

	local vertices = state.crate.vertices
	local floatsPerVertex = 12
	
	MatrixMode("Model")
	Identity()
	Rotate(0, 0.7, 0)

	BindTexture(state.texture.data)
	BeginObject3D("Triangles")
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
end

function End()
	print("Ending mesh test")
end
