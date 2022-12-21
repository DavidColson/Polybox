// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "LuaCommon.h"
#include "Mesh.h"

#include <light_string.h>

struct Node : public LuaObject {
    Node() {
        m_id = s_nodeIdCounter++;
    }

    Vec3f GetLocalPosition();
    Vec3f GetWorldPosition();
    void SetLocalPosition(Vec3f translation);

    Vec3f GetLocalRotation();
    Vec3f GetWorldRotation();
    void SetLocalRotation(Vec3f rotation);
    void SetLocalRotation(Quatf rotation);

    Vec3f GetLocalScale();
    Vec3f GetWorldScale();
    void SetLocalScale(Vec3f scale);

    // TODO: Set world transforms
    // Get and set matrices directly

    Node* GetParent();
    int GetNumChildren();
    Node* GetChild(int index);

    // User table info
    String m_name;
    uint32_t m_meshId;

    // Tree data
    // TODO: Make private when we have proper tree editing API
    Node* m_pParent { nullptr };
    ResizableArray<Node*> m_children;

    uint64_t m_id;
    static uint64_t s_nodeIdCounter;
    Matrixf m_localTransform;
    Matrixf m_worldTransform;

   private:
    void UpdateWorldTransforms();
};

struct Scene : public LuaObject {
    virtual ~Scene();

    int GetNumNodes();
    Node* GetNode(int index);

    // TODO: Note that we currently cannot add or modify this tree without invalidating the pointers inside the nodes
    // what the fuck do we do
    ResizableArray<Node> m_nodes;

    static Scene* LoadScene(const char* filePath);
};
