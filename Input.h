#ifndef INPUTCLASS_H
#define INPUTCLASS_H
#include <GLFW/glfw3.h>

class Input {

public:
    Input();
    ~Input();
    void Initialize();
    void Shutdown();
    void BeginProcessInput(GLFWwindow* window);
    void EndProcessInput();
    bool IsPressed(int key);
    bool IsHeld(int key);
    bool IsReleased(int key);

private:
    bool m_prev[GLFW_KEY_LAST] = {};
    bool m_curr[GLFW_KEY_LAST] = {};

};

#endif //INPUTCLASS_H