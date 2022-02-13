
function RecursePrintNode(node, stringEdge)
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

function Start()
    tankMeshes = LoadMeshes("Assets/tank.gltf")
    tankImages = LoadTextures("Assets/tank.gltf")
    tankScene = LoadScene("Assets/tank.gltf")

    x = 50
    y = 50

    camX = 0.0
    camY = 3.6

    --EnableMouseRelativeMode(true)

    text = ""
end

function Update(deltaTime)
    SetClearColor(0.25, 0.25, 0.25, 1.0)

    MatrixMode("Projection")
    Perspective(320, 240, 1, 20, 60)

    camX = camX + deltaTime * GetAxis(Axis.RightY)
    camY = camY + deltaTime * GetAxis(Axis.RightX)

    MatrixMode("View")
    Rotate(camX, camY, 0)
    Translate(1, -2.5, 6.5)

    NormalsMode("Custom")
    EnableLighting(true)
    Ambient(0.4, 0.4, 0.4)
    Light(0, -1, 1, 0, 1, 1, 1)

    EnableFog(true)
    SetFogStart(3.0)
    SetFogEnd(15.0)

    for i = 1, tankScene:GetNumNodes(), 1 do
        local node = tankScene:GetNode(i)

        MatrixMode("Model")
        Identity()
        Translate(node:GetWorldPosition())
        Rotate(node:GetWorldRotation())
        Scale(node:GetWorldScale())

        if node:GetPropertyTable().meshId ~= nil then 
            if tankMeshes[node:GetPropertyTable().meshId]:GetName() == "BlobShadow" then goto continue end
            local prim = tankMeshes[node:GetPropertyTable().meshId]:GetPrimitive(1)
            
            BindTexture(tankImages[prim:GetMaterialTextureId()])
            
            BeginObject3D("Triangles")
            for i = 1, prim:GetNumVertices(), 1 do
                Normal(prim:GetVertexNormal(i))
                Color(prim:GetVertexColor(i))
                TexCoord(prim:GetVertexTexCoord(i))
                Vertex(prim:GetVertexPosition(i))
            end
            
            EndObject3D()
        end
        ::continue::
    end

    for i = 1, tankScene:GetNumNodes(), 1 do
        local node = tankScene:GetNode(i)

        MatrixMode("Model")
        Identity()
        Translate(node:GetWorldPosition())
        Rotate(node:GetWorldRotation())
        Scale(node:GetWorldScale())

        if node:GetPropertyTable().meshId ~= nil then 
            if tankMeshes[node:GetPropertyTable().meshId]:GetName() ~= "BlobShadow" then goto continue end
            local prim = tankMeshes[node:GetPropertyTable().meshId]:GetPrimitive(1)
            
            BindTexture(tankImages[prim:GetMaterialTextureId()])
            
            BeginObject3D("Triangles")
            for i = 1, prim:GetNumVertices(), 1 do
                Normal(prim:GetVertexNormal(i))
                Color(prim:GetVertexColor(i))
                TexCoord(prim:GetVertexTexCoord(i))
                Vertex(prim:GetVertexPosition(i))
            end
            
            EndObject3D()
        end
        ::continue::
    end
    SetClearColor(0.25, 0.25, 0.25, 0)

    UnbindTexture()

    MatrixMode("Model")
    Identity()

    MatrixMode("View")
    Identity()

    -- if GetButton(Button.DpadRight) then
    --     x = x+1
    -- end
    -- if GetButton(Button.DpadLeft) then
    --     x = x-1
    -- end
    -- if GetButton(Button.DpadDown) then
    --     y = y-1
    -- end
    -- if GetButton(Button.DpadUp) then
    --     y = y+1
    -- end

    x = x + GetAxis(Axis.RightX)
    y = y - GetAxis(Axis.RightY)

    text = text .. InputString()

    --DrawCircle(x, y, 50, 1, 1, 1, 1)
    DrawText("LeftX: " .. GetAxis(Axis.LeftX), 20, 20, 20)
    DrawText("LeftY: " .. GetAxis(Axis.LeftY), 20, 40, 20)
    DrawText("RightX: " .. GetAxis(Axis.RightX), 20, 60, 20)
    DrawText("RightY: " .. GetAxis(Axis.RightY), 20, 80, 20)
    DrawText("TriggerLeft: " .. GetAxis(Axis.TriggerLeft), 20, 100, 20)
    DrawText("TriggerRight: " .. GetAxis(Axis.TriggerRight), 20, 120, 20)
    DrawText("Text Input: " .. text, 20, 140, 20)
end

function End()
    print("Program end, about to print hangar mesh id")
end