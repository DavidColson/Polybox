
struct SDL_Window;
struct sg_desc;
struct sg_environment;
struct sg_swapchain;
struct sg_image;

// Backend platform
bool GraphicsBackendInit(SDL_Window* pWindow, int width, int height);
sg_environment SokolGetEnvironment();
sg_swapchain SokolGetSwapchain();
void SokolFlush();
void SokolPresent();

// Platform specific implementations of things
void ReadbackImagePixels(sg_image img_id, void* pixels);
void ReadbackPixels(int x, int y, int w, int h, void *pixels);
