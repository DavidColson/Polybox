--!strict
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

local state = {}

state.camX = 0.0
state.camY = 3.6
state.text = ""

function Start()
	state.tankMeshes = LoadMeshes("assets/tank.gltf")
	state.tankImages = LoadTextures("assets/tank.gltf")
	state.tankScene = LoadScene("assets/tank.gltf")

    state.x = 50
    state.y = 50

	local num = math.asin(0.234)
	local time = os.clock()
	print(math.sin(1.24))

	local buf = NewBuffer("f32", 4, 4)
	buf:Set2D(0,0, 	1,2,3,4,
					5,6,7,8);
	print("buffer test:")
	print(buf:Get2D(2, 1, 1))
	print(buf.y)
	print(buf.a)
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
