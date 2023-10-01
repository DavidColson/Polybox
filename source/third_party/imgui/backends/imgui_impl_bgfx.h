
union SDL_Event;
struct SDL_Window;

namespace ImGuiBackend
{
    #define IMGUI_FLAGS_NONE        UINT8_C(0x00)
    #define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

    void Initialize(SDL_Window* pWindow);
    bool ProcessEvent(SDL_Event& event);
    void NewFrame();
    void Render();
    void Destroy();
}