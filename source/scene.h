// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "lua_common.h"
#include "mesh.h"

#include <light_string.h>

template<typename T>
struct Quat;
typedef Quat<f32> Quatf;

struct Node : public LuaObject {
    Node() {
        id = s_nodeIdCounter++;
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
    i32 GetNumChildren();
    Node* GetChild(int index);

    // User table info
    String name;
    u32 meshId;

    // Tree data
    // TODO: Make private when we have proper tree editing API
    Node* pParent { nullptr };
    ResizableArray<Node*> children;

    u64 id;
    static u64 s_nodeIdCounter;
    Matrixf localTransform;
    Matrixf worldTransform;

   private:
    void UpdateWorldTransforms();
};

struct Scene : public LuaObject {
    virtual ~Scene();

	Arena* pArena;

    i32 GetNumNodes();
    Node* GetNode(int index);

    // TODO: Note that we currently cannot add or modify this tree without invalidating the pointers inside the nodes
    // what the fuck do we do
    ResizableArray<Node> nodes;

    static Scene* LoadScene(const char* filePath);
};
