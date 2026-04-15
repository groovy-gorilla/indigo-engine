#include "SystemClass.h"

#include <thread>


SystemClass::SystemClass() {
    window = nullptr;
    graphics = nullptr;
    input = nullptr;
}

SystemClass::~SystemClass() = default;

void SystemClass::Initialize() {
    InitializeWindow();

    graphics = new GraphicsClass();
    graphics->Initialize(window);

    input = new InputClass();
    input->Initialize();
}

void SystemClass::Shutdown() {
    graphics->Shutdown();
    delete graphics;
    graphics = nullptr;

    input->Shutdown();
    delete input;
    input = nullptr;

    ShutdownWindow();
}

void SystemClass::Run() {
    Loop();
}

void SystemClass::InitializeWindow() {

    setenv("GTK_THEME", "HighContrastInverse", 1);

    glfwInit();
    if (!glfwVulkanSupported()) {
        std::cerr << "GLFW Vulkan not supported!" << std::endl;
    }

    if (SETTINGS.DISPLAY_MODE == DISPLAY_MODE::WINDOWED) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
        glfwWindowHint(GLFW_PLATFORM_X11, GLFW_TRUE);
        window = glfwCreateWindow(SETTINGS.WIDTH, SETTINGS.HEIGHT, "Vulkan", nullptr, nullptr);
    }

    if (SETTINGS.DISPLAY_MODE == DISPLAY_MODE::FULLSCREEN) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE , GLFW_FALSE);
        glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        window = glfwCreateWindow(SETTINGS.WIDTH, SETTINGS.HEIGHT, "Vulkan", monitor, nullptr);
    }
}

void SystemClass::ShutdownWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void SystemClass::Loop() {
    while (!glfwWindowShouldClose(window)) {

        graphics->vulkan->drawFrame(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            graphics->vulkan->SetResolution(320, 240);
        }

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            graphics->vulkan->SetResolution(3440, 1440);
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            graphics->vulkan->SetMSAA(VK_SAMPLE_COUNT_1_BIT);
        }

        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            graphics->vulkan->SetMSAA(VK_SAMPLE_COUNT_16_BIT);
        }

        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
            graphics->vulkan->SetFilter(VK_FILTER_NEAREST);
        }

        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
            graphics->vulkan->SetFilter(VK_FILTER_LINEAR);
        }


    }

    vkDeviceWaitIdle(graphics->vulkan->getDevice());
}
