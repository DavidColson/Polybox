
function Start()
    crateTexture = NewImage("Assets/crate.png")
end

function Update()
    SetClearColor(0.25, 0.25, 0.25, 1.0)

    MatrixMode("Projection")
    Identity()
    Perspective(320, 240, 1, 20, 60)

    MatrixMode("View")
    Identity()
    Translate(-0.5, -0.5, -2)

    BindTexture(crateTexture)

    local width = 1.0
    local height = 1.0
    BeginObject3D("Triangles")
        TexCoord(0, 0)
        Vertex(0, 0, 0)
        TexCoord(1, 0)
        Vertex(width, 0, 0)
        TexCoord(1, 1)
        Vertex(width, height, 0)

        TexCoord(0, 0)
        Vertex(0, 0, 0)
        TexCoord(1, 1)
        Vertex(width, height)
        TexCoord(0, 1)
        Vertex(0, height, 0)
    EndObject3D()
end

function End()
    print("Program end")
end