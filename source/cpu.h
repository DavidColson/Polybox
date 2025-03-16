// Copyright David Colson. All rights reserved.

struct String;

namespace Cpu {

void Init();
void LoadApp(String appName);
void Start();
void Tick(f32 deltaTime);
void Close();
String GetAppName();

};
