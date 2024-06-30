
struct SDL_Window;
struct sg_desc;
struct sg_environment;
struct sg_swapchain;

bool GraphicsBackendInit(SDL_Window* pWindow, int width, int height);
sg_environment SokolGetEnvironment();
sg_swapchain SokolGetSwapchain();
void SokolPresent();
