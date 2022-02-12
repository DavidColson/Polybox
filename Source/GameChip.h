#pragma once

class GameChip
{
public:
    void Init();
    void Update();

    // API

    bool GetKey(int buttonCode);
    bool GetKeyDown(int buttonCode);
    bool GetKeyUp(int buttonCode);
};