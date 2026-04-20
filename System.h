#ifndef SYSTEMCLASS_H
#define SYSTEMCLASS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Graphics.h"
#include "Input.h"

class System {

public:
    System();
    ~System();
    void Initialize();
    void Shutdown();
    void Run();

private:
    GLFWwindow *m_window;
    Graphics *m_graphics;
    Input *m_input;

    void InitializeWindow();
    void ShutdownWindow();

    void Loop();

};

#endif //SYSTEMCLASS_H