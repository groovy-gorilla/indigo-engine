#ifndef GRAPHICSCLASS_H
#define GRAPHICSCLASS_H
#include <GLFW/glfw3.h>

#include "Vulkan.h"
#include "Texture.h"

class Graphics {

public:
    Graphics();
    ~Graphics();
    void Initialize(GLFWwindow *window);
    void Shutdown();

    Vulkan *m_vulkan;

private:


};

#endif //GRAPHICSCLASS_H