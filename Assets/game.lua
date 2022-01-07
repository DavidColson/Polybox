
function Start()
    crateTexture = NewImage("Assets/crate.png")
    tankMeshes = LoadMeshes("Assets/tank.gltf")
    tankImages = LoadTextures("Assets/tank.gltf")
    print(#tankImages)

    for i, mesh in ipairs(tankMeshes) do
        local prim = mesh:GetPrimitive(1)
        print("Primitive num vertices")
        print(prim:GetNumVertices())
    end
end

rotation = 0.0

function Update()
    SetClearColor(0.25, 0.25, 0.25, 1.0)

    MatrixMode("Projection")
    Perspective(320, 240, 1, 20, 60)

    MatrixMode("View")
    Translate(-0.5, -1, -8)

    MatrixMode("Model")
    Rotate(0, rotation, 0)
    rotation = rotation + 0.01
    
    for i, mesh in ipairs(tankMeshes) do
        
        if mesh:GetName() == "tankMesh" then
        
            local prim = mesh:GetPrimitive(1)

            BindTexture(tankImages[prim:GetMaterialTextureId()])
            
            BeginObject3D("Triangles")
            
            for i = 1, prim:GetNumVertices(), 1 do
                TexCoord(prim:GetVertexTexCoord(i))
                Vertex(prim:GetVertexPosition(i))
            end
            
            EndObject3D()
        end
    end
end

function End()
    print("Program end")
end