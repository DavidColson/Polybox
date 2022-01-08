
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
end

function Update(deltaTime)
    SetClearColor(0.25, 0.25, 0.25, 1.0)

    MatrixMode("Projection")
    Perspective(320, 240, 1, 20, 60)

    MatrixMode("View")
    Rotate(0.0, 3.6, 0)
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
end

function End()
    print("Program end, about to print hangar mesh id")
end