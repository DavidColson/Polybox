// Copyright 2020-2022 David Colson. All rights reserved.

u64 Node::s_nodeIdCounter = 0;

// ***********************************************************************

Vec3f Node::GetLocalPosition() {
    return localTransform.GetTranslation();
}

// ***********************************************************************

Vec3f Node::GetWorldPosition() {
    return worldTransform.GetTranslation();
}

// ***********************************************************************

void Node::SetLocalPosition(Vec3f translation) {
    localTransform.SetTranslation(translation);
    UpdateWorldTransforms();
}

// ***********************************************************************

Vec3f Node::GetLocalRotation() {
    return localTransform.GetEulerRotation();
}

// ***********************************************************************

Vec3f Node::GetWorldRotation() {
    return worldTransform.GetEulerRotation();
}

// ***********************************************************************

void Node::SetLocalRotation(Vec3f rotation) {
    localTransform.SetEulerRotation(rotation);
    UpdateWorldTransforms();
}

// ***********************************************************************

void Node::SetLocalRotation(Quatf rotation) {
    localTransform.SetQuatRotation(rotation);
    UpdateWorldTransforms();
}

// ***********************************************************************

Vec3f Node::GetLocalScale() {
    return localTransform.GetScaling();
}

// ***********************************************************************

Vec3f Node::GetWorldScale() {
    return worldTransform.GetScaling();
}

// ***********************************************************************

void Node::SetLocalScale(Vec3f scale) {
    localTransform.SetScaling(scale);
    UpdateWorldTransforms();
}

// ***********************************************************************

Node* Node::GetParent() {
    return pParent;
}

// ***********************************************************************

int Node::GetNumChildren() {
    return (int)children.count;
}

// ***********************************************************************

Node* Node::GetChild(int index) {
    return children[index];
}

// ***********************************************************************

void Node::UpdateWorldTransforms() {
    // Recalculate local world matrix
    if (pParent) {
        worldTransform = pParent->worldTransform * localTransform;
    } else {
        worldTransform = localTransform;
    }

    if (!children.count) {
        for (Node* pChild : children) {
            pChild->worldTransform = worldTransform * pChild->localTransform;
        }
    }
}

// ***********************************************************************

Scene::~Scene() {
	ArenaFinished(pArena);
}

// ***********************************************************************

int Scene::GetNumNodes() {
    return (int)nodes.count;
}

// ***********************************************************************

Node* Scene::GetNode(int index) {
    return &nodes[index];
}

// ***********************************************************************

void ParseNodesRecursively(Scene* pScene, Node* pParent, ResizableArray<Node>& outNodes, JsonValue& nodeToParse, JsonValue& nodesData) {
    for (int i = 0; i < nodeToParse.Count(); i++) {
        i32 nodeId = nodeToParse[i].ToInt();
        JsonValue& jsonNode = nodesData[nodeId];

        // extract the nodes
        outNodes.PushBack(Node());
        Node& node = outNodes[outNodes.count - 1];
		node.children.pArena = pScene->pArena;

        String nodeName = jsonNode.HasKey("name") ? jsonNode["name"].ToString() : String("");
        node.name = CopyString(nodeName, pScene->pArena);

        node.meshId = UINT32_MAX;

        if (pParent) {
            pParent->children.PushBack(&node);
            node.pParent = pParent;
        }

        if (jsonNode.HasKey("mesh")) {
            node.meshId = jsonNode["mesh"].ToInt();
        }

        if (jsonNode.HasKey("rotation")) {
            Quatf rotation;
            rotation.x = f32(jsonNode["rotation"][0].ToFloat());
            rotation.y = f32(jsonNode["rotation"][1].ToFloat());
            rotation.z = f32(jsonNode["rotation"][2].ToFloat());
            rotation.w = f32(jsonNode["rotation"][3].ToFloat());
            node.SetLocalRotation(rotation);
        } else {
            node.SetLocalRotation(Quatf::Identity());
        }

        if (jsonNode.HasKey("translation")) {
            Vec3f translation;
            translation.x = f32(jsonNode["translation"][0].ToFloat());
            translation.y = f32(jsonNode["translation"][1].ToFloat());
            translation.z = f32(jsonNode["translation"][2].ToFloat());
            node.SetLocalPosition(translation);
        } else {
            node.SetLocalPosition(Vec3f(0.0f));
        }

        if (jsonNode.HasKey("scale")) {
            Vec3f scale;
            scale.x = f32(jsonNode["scale"][0].ToFloat());
            scale.y = f32(jsonNode["scale"][1].ToFloat());
            scale.z = f32(jsonNode["scale"][2].ToFloat());
            node.SetLocalScale(scale);
        } else {
            node.SetLocalScale(Vec3f(1.0f));
        }

		// this is duplicating entries!!!!!
        if (jsonNode.HasKey("children")) {
            ParseNodesRecursively(pScene, &node, outNodes, jsonNode["children"], nodesData);
        }
    }
}

// ***********************************************************************

Scene* Scene::LoadScene(const char* filePath) {
    SDL_RWops* pFileRead = SDL_RWFromFile(filePath, "rb");

	Arena* pArena = ArenaCreate();
	Scene* pScene = New(pArena, Scene);
	PlacementNew(pScene) Scene();
	pScene->Retain();
	pScene->pArena = pArena;
	pScene->nodes.pArena = pArena;

    u64 size = SDL_RWsize(pFileRead);
    char* pData = New(g_pArenaFrame, char, size);
    SDL_RWread(pFileRead, pData, size, 1);
    SDL_RWclose(pFileRead);

    String file;
    file.pData = pData;
    file.length = size;

    JsonValue parsed = ParseJsonFile(g_pArenaFrame, file);

    bool validGltf = parsed["asset"]["version"].ToString() == "2.0";
    if (!validGltf)
        return nullptr;

    pScene->nodes.Reserve(parsed["nodes"].Count());
    ParseNodesRecursively(pScene, nullptr, pScene->nodes, parsed["scenes"][0]["nodes"], parsed["nodes"]);

    return pScene;
}
