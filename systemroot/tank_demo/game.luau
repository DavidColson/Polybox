function RecursePrintNode(node:Node, stringEdge:string)
    local meshId = node:GetPropertyTable().meshId
    
    if meshId == nil then
        print(stringEdge .. node:GetPropertyTable().name .. " meshId: nil")
    else
        print(stringEdge .. node:GetPropertyTable().name .. " meshId: " .. meshId)
    end
    
    for i = 1, node:GetNumChildren(), 1 do
        local child = node:GetChild(i)
        RecursePrintNode(child, stringEdge .. "- ")
    end
end

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

state.camX = 0.0
state.camY = 3.6
state.text = ""

function Start()
	state.tankMeshes = LoadMeshes("systemroot/tank_demo/tank.gltf")
	state.tankImages = LoadTextures("systemroot/tank_demo/tank.gltf")
	state.tankScene = LoadScene("systemroot/tank_demo/tank.gltf")

    state.x = 50
    state.y = 50

	local num = math.asin(0.234)
	local time = os.clock()
	print(math.sin(1.24))

	local bufferFloats = buffer("f32", 5)
	bufferFloats:set(0, 22.1, 54.2, 87.51213, 99999.1337, 2.43)

	local bufferInts = buffer("i16", 4)
	bufferInts:set(0, 32, 1, 44, 87)

	-- metadata test
	local data = --[[poly, type=2]]{i=432}
	print(data.i)
	
	local testTable = { first=43, second=23, [77]="hello", buffer=bufferFloats, mybool=true, floating=3.25, false, 878}
	local testMeta = {metaType="test", size=33}

	-- print("\nSerialized as text:")
	-- local text = serialize(testTable, 0x0, testMeta)
	-- print(text)
	-- print("\nDeserialized from text:")
	-- local parsed, meta = deserialize(text)
	-- printTable(parsed)
	-- print("metadata:")
	-- printTable(meta)
	--
	-- print("\nSerialized:")
	-- local bin = serialize(testTable, 0x1, testMeta)
	-- print(bin)
	-- print("\nDeserialized:")
	-- local parsed, meta = deserialize(bin)
	-- printTable(parsed)
	-- print("metadata:")
	-- printTable(meta)
	--
	-- print("\nSerialized Compressed:")
	-- local binSmall = serialize(testTable, 0x3, testMeta)
	-- print(binSmall)
	-- print("\nDeserialized Compressed:")
	-- parsed, meta = deserialize(binSmall)
	-- printTable(parsed)
	-- print("metadata:")
	-- printTable(meta)

	store("test.pff", testTable, 0x0, testMeta)
	local result, meta = load("test.pff")
	printTable(result)
	printTable(meta)

	local pastedBuffer = buffer("f32",5,1,"22.100000,54.200001,87.512131,99999.132812,2.430000")
	print(pastedBuffer)
	-- print("\nSerialized Compressed b64:")
	-- local binB64 = serialize(testTable, 0x7, testMeta)
	-- print(binB64)
	-- print("\nDeserialized Compressed b64:")
	-- parsed, meta = deserialize(binB64)
	-- printTable(parsed)
	-- print("metadata:")
	-- printTable(meta)
	--
	-- print("\nSerialized non compressed b64:")
	-- local binDB64 = serialize(testTable, 0x5, testMeta)
	-- print(binDB64)
	-- print("\nDeserialized non compressed b64:")
	-- parsed, meta = deserialize(binDB64)
	-- printTable(parsed)
	-- print("metadata:")
	-- printTable(meta)
	--
    -- EnableMouseRelativeMode(true)
end

function Update(deltaTime: number)
    SetClearColor(0.25, 0.25, 0.25, 1.0)

    MatrixMode("Projection")
    Perspective(320, 240, 1, 20, 60)

    state.camX = state.camX + deltaTime * GetAxis(Axis.RightY)
    state.camY = state.camY + deltaTime * GetAxis(Axis.RightX)

    MatrixMode("View")
    Rotate(state.camX, state.camY, 0)
    Translate(1, -2.5, 6.5)

    NormalsMode("Custom")
    EnableLighting(true)
    Ambient(0.4, 0.4, 0.4)
    Light(0, -1, 1, 0, 1, 1, 1)

    EnableFog(true)
    SetFogStart(3.0)
    SetFogEnd(15.0)
	SetFogColor(0.25, 0.25, 0.25)

    for i = 1, state.tankScene:GetNumNodes(), 1 do
        local node: Node = state.tankScene:GetNode(i)

        MatrixMode("Model")
        Identity()
        Translate(node:GetWorldPosition())
        Rotate(node:GetWorldRotation())
        Scale(node:GetWorldScale())

        if node:GetPropertyTable().meshId ~= nil and state.tankMeshes[node:GetPropertyTable().meshId]:GetName() ~= "BlobShadow" then 
            local prim = state.tankMeshes[node:GetPropertyTable().meshId]:GetPrimitive(1)
            
            BindTexture(state.tankImages[prim:GetMaterialTextureId()])
            
            BeginObject3D("Triangles")
            for i = 1, prim:GetNumVertices(), 1 do
                Normal(prim:GetVertexNormal(i))
                Color(prim:GetVertexColor(i))
                TexCoord(prim:GetVertexTexCoord(i))
                Vertex(prim:GetVertexPosition(i))
            end
            
            EndObject3D()
        end
    end

    for i = 1, state.tankScene:GetNumNodes(), 1 do
        local node = state.tankScene:GetNode(i)

        MatrixMode("Model")
        Identity()
        Translate(node:GetWorldPosition())
        Rotate(node:GetWorldRotation())
        Scale(node:GetWorldScale())

        if node:GetPropertyTable().meshId ~= nil and state.tankMeshes[node:GetPropertyTable().meshId]:GetName() == "BlobShadow" then 
            local prim = state.tankMeshes[node:GetPropertyTable().meshId]:GetPrimitive(1)
            
            BindTexture(state.tankImages[prim:GetMaterialTextureId()])
            
            BeginObject3D("Triangles")
            for i = 1, prim:GetNumVertices(), 1 do
                Normal(prim:GetVertexNormal(i))
                Color(prim:GetVertexColor(i))
                TexCoord(prim:GetVertexTexCoord(i))
                Vertex(prim:GetVertexPosition(i))
            end
            
            EndObject3D()
        end
    end

    UnbindTexture()

    MatrixMode("Model")
    Identity()

    MatrixMode("View")
    Identity()

    if GetButton(Button.DpadRight) then
        state.x = state.x+1
    end
    if GetButton(Button.DpadLeft) then
        state.x = state.x-1
    end
    if GetButton(Button.DpadDown) then
        state.y = state.y-1
    end
    if GetButton(Button.DpadUp) then
        state.y = state.y+1
    end

    -- x = x + GetAxis(Axis.RightX)
    -- y = y - GetAxis(Axis.RightY)

    state.text = state.text .. InputString()

    if GetKeyDown(Key.Backspace) then
        print("hello")
        state.text = state.text:sub(1, state.text:len()-1)
    end

    state.x, state.y = GetMousePosition()

    DrawCircle(state.x, state.y, 5, 1, 1, 1, 1)
    DrawText("MouseX: " .. state.x, 20, 20, 20)
    DrawText("MouseY: " .. state.y, 20, 40, 20)
    DrawText("Text Input: " .. state.text, 20, 140, 20)
end

function End()
    print("Program end, about to print hangar mesh id")
end
