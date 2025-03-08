// Copyright David Colson. All rights reserved.

struct String;

namespace Cpu {

void Init();
void CompileAndLoadProgram(String path);
void Start();
void Tick(f32 deltaTime);
void Close();

};
