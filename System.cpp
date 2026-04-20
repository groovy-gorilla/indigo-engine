#include "System.h"

#include <algorithm>
#include <iostream>
#include "Global.h"

System::System() {
    m_window = nullptr;
    m_graphics = nullptr;
    m_input = nullptr;
}

System::~System() = default;

void System::Initialize() {

    InitializeWindow();

    m_graphics = new Graphics();
    m_graphics->Initialize(m_window);

    m_input = new Input();
    m_input->Initialize();

}

void System::Shutdown() {

    m_graphics->Shutdown();
    delete m_graphics;
    m_graphics = nullptr;

    m_input->Shutdown();
    delete m_input;
    m_input = nullptr;

    ShutdownWindow();

}

void System::Run() {

    Loop();

}

void System::InitializeWindow() {

    setenv("GTK_THEME", "HighContrastInverse", 1);

    glfwInit();
    if (!glfwVulkanSupported()) {
        std::cerr << "GLFW Vulkan not supported!" << std::endl;
    }

    if (SETTINGS.FULLSCREEN) {

        // FULLSCREEN
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE , GLFW_FALSE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        m_window = glfwCreateWindow(mode->width, mode->height, SETTINGS.TITLE.c_str(), monitor, nullptr);

        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    } else {

        // WINDOWED
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        m_window = glfwCreateWindow(1, 1, SETTINGS.TITLE.c_str(), nullptr, nullptr);

        float scaleX, scaleY;
        glfwGetWindowContentScale(m_window, &scaleX, &scaleY);

        int xpos = (mode->width - SETTINGS.WIDTH) / 2;
        int ypos = (mode->height - SETTINGS.HEIGHT) / 2;

        glfwSetWindowMonitor(m_window, nullptr, xpos, ypos, SETTINGS.WIDTH / scaleX, SETTINGS.HEIGHT / scaleY, GLFW_DONT_CARE);

        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    }

}

void System::ShutdownWindow() {

    glfwDestroyWindow(m_window);
    glfwTerminate();

}

void System::Loop() {

    while (!glfwWindowShouldClose(m_window)) {

        m_graphics->m_vulkan->DrawFrame(m_window);

        glfwPollEvents();
        m_input->BeginProcessInput(m_window);

        // SCREEN RESOLUTION
        int size = m_graphics->m_vulkan->GetVideoModes().size();
        std::vector<Vulkan::Resolution> m = m_graphics->m_vulkan->GetVideoModes();
        auto it = std::find_if(m.begin(), m.end(),
            [&](const Vulkan::Resolution& r) {
                return r.w == SETTINGS.WIDTH && r.h == SETTINGS.HEIGHT;
            });
        static int i = -1;
        if (it != m_graphics->m_vulkan->GetVideoModes().end()) {
            i = std::distance(m.begin(), it);
        }
        if (m_input->IsPressed(GLFW_KEY_EQUAL)) {
            i++;
            if (i > (size-1)) i = (size-1);
            m_graphics->m_vulkan->SetResolution(m_window, m_graphics->m_vulkan->GetVideoModes().at(i).w, m_graphics->m_vulkan->GetVideoModes().at(i).h);
            std::cout << SETTINGS.WIDTH << "x" << SETTINGS.HEIGHT << std::endl;
        }

        if (m_input->IsPressed(GLFW_KEY_MINUS)){
            i--;
            if (i < 0) i = 0;
            m_graphics->m_vulkan->SetResolution(m_window, m_graphics->m_vulkan->GetVideoModes().at(i).w, m_graphics->m_vulkan->GetVideoModes().at(i).h);
            std::cout << SETTINGS.WIDTH << "x" << SETTINGS.HEIGHT << std::endl;
        }

        // MSAA
        static bool msaa = false;
        if (m_input->IsPressed(GLFW_KEY_M)) {
            if (msaa) {
                m_graphics->m_vulkan->SetMSAA(VK_SAMPLE_COUNT_1_BIT);
                msaa = false;
            } else {
                m_graphics->m_vulkan->SetMSAA(VK_SAMPLE_COUNT_16_BIT);
                msaa = true;
            }
        }

        // FILTER
        static bool filter = false;
        if (m_input->IsPressed(GLFW_KEY_F)) {
            if (filter) {
                m_graphics->m_vulkan->SetFilter(VK_FILTER_NEAREST);
                filter = false;
            } else {
                m_graphics->m_vulkan->SetFilter(VK_FILTER_LINEAR);
                filter = true;
            }
        }

        // ASPECT RATIO
        static bool aspect = false;
        if (m_input->IsPressed(GLFW_KEY_A)) {
            if (aspect) {
                m_graphics->m_vulkan->SetAspectRatioEnabled(false);
                aspect = false;
            } else {
                m_graphics->m_vulkan->SetAspectRatioEnabled(true);
                aspect = true;
            }
        }

        // FULLSCREEN/WINDOWED
        static bool fullscreen = true;
        if (m_input->IsPressed(GLFW_KEY_W)) {
            if (fullscreen) {
                m_graphics->m_vulkan->SetFullscreenEnabled(m_window, false);
                fullscreen = false;
            } else {
                m_graphics->m_vulkan->SetFullscreenEnabled(m_window, true);
                fullscreen = true;
            }
        }

        // QUIT
        if (m_input->IsPressed(GLFW_KEY_ESCAPE)) break;


        m_input->EndProcessInput();

    }

    vkDeviceWaitIdle(m_graphics->m_vulkan->GetDevice());

}

