// Copyright David Colson. All rights reserved.
#pragma once

struct lua_State;
struct String;

enum class Type:u8 {
	Float32,
	Int32,
	Int16,
	Uint8
};

struct UserData {
	i32 width;
	i32 height;
	Type type;
	u8* pData;

	// used when the userdata contains an image
	sg_image img;
	bool dirty;
	bool dynamic;
};

UserData* AllocUserData(lua_State* L, Type type, i32 width, i32 height);
i64 GetUserDataSize(UserData* pUserData);
void UpdateUserDataImage(UserData* pUserData);
void ParseUserDataString(lua_State* L, String dataString, UserData* pUserData);
void BindUserData(lua_State* L);
