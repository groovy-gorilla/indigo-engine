#include "Graphics.h"

Graphics::Graphics() {

    m_vulkan = nullptr;

}

Graphics::~Graphics() = default;

void Graphics::Initialize(GLFWwindow *window) {

    m_vulkan = new Vulkan;
    m_vulkan->Initialize(window);

    Texture texture;
    texture.Initialize(m_vulkan->GetContext(), "test.ktx");
    texture.Shutdown(m_vulkan->GetContext());

}

void Graphics::Shutdown() {

    m_vulkan->Shutdown();
    delete m_vulkan;
    m_vulkan = nullptr;

}
