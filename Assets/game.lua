
function Start()
    print("Program start, pigeon width:")
    local pigeon = NewImage("Assets/Pigeon.png")
    
    print(pigeon.GetWidth(pigeon))
    print(pigeon:GetType())
end

function Update()
    BeginObject2D("Triangles")
        Vertex(100.0, 100.0)
        Vertex(100.0, 150.0)
        Vertex(150.0, 150.0)
    EndObject2D()
end

function End()
    print("Program end")
end