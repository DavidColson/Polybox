
local state = {}

function Start()
	-- Load a mesh
	state.crate = load("systemroot/tank_demo/tankMesh.mesh")
	print("Starting mesh test")

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
    Translate(0, -3, -8)

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
