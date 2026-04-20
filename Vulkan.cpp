#include "Vulkan.h"
#include "Global.h"
#include "ErrorDialog.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

// validation layers inline functions
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {

    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }

}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {

    auto app = static_cast<Vulkan*>(glfwGetWindowUserPointer(window));
    app->SetFramebufferResized(true);

}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::string msg = "Validation Layer\n\n"
        "ID:" + std::string(pCallbackData->pMessageIdName) + "\n\n"
        "Message:\n" + std::string(pCallbackData->pMessage);
        ShowErrorDialog(msg);
    }
    return VK_FALSE;
}

Vulkan::Vulkan() {

    m_instance = VK_NULL_HANDLE;
    m_debugMessenger = VK_NULL_HANDLE;
    m_surface = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_presentQueue = VK_NULL_HANDLE;
    m_swapChain = VK_NULL_HANDLE;
    m_renderPass = VK_NULL_HANDLE;
    m_scenePipelineLayout = VK_NULL_HANDLE;
    m_scenePipeline = VK_NULL_HANDLE;
    m_msaaImage = VK_NULL_HANDLE;
    m_msaaImageView = VK_NULL_HANDLE;
    m_msaaMemory = VK_NULL_HANDLE;
    m_offscreenImage = VK_NULL_HANDLE;
    m_offscreenImageView = VK_NULL_HANDLE;
    m_offscreenMemory = VK_NULL_HANDLE;
    m_offscreenRenderPass = VK_NULL_HANDLE;
    m_msaaRenderPass = VK_NULL_HANDLE;
    m_framebuffer = VK_NULL_HANDLE;
    m_descriptorSetLayout = VK_NULL_HANDLE;
    m_postPipelineLayout = VK_NULL_HANDLE;
    m_postPipeline = VK_NULL_HANDLE;
    m_sampler = VK_NULL_HANDLE;
    m_descriptorPool = VK_NULL_HANDLE;
    m_descriptorSet = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;

}

Vulkan::~Vulkan() = default;

void Vulkan::Initialize(GLFWwindow *window) {

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface(window);
    CreateSupportedVideoModes();
    PickPhysicalDevice();
    CreateSupportedSampleCounts(m_physicalDevice);
    CreateLogicalDevice();
    CreateSwapChain(window);
    CreateImageViews();
    CreateOffscreenResources();
    CreateMSAAResources();
    CreateOffscreenRenderPass();
    CreateMSAARenderPass();
    CreateFramebuffer();
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateScenePipeline();
    CreatePostPipeline();
    CreateSampler();
    CreateDescriptorPool();
    CreateDescriptorSet();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();

}

void Vulkan::Shutdown() {
    if (m_msaaImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_msaaImageView, nullptr);
        m_msaaImageView = VK_NULL_HANDLE;
    }

    if (m_msaaImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_msaaImage, nullptr);
        m_msaaImage = VK_NULL_HANDLE;
    }

    if (m_msaaMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_msaaMemory, nullptr);
        m_msaaMemory = VK_NULL_HANDLE;
    }

    for (size_t i=0; i < SETTINGS.MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        m_renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        m_imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        m_inFlightFences[i] = VK_NULL_HANDLE;
    }

    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    for (auto framebuffer : m_swapChainFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
    }

    if (m_scenePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_scenePipeline, nullptr);
        m_scenePipeline = VK_NULL_HANDLE;
    }

    if (m_scenePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_scenePipelineLayout, nullptr);
        m_scenePipelineLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    if (m_postPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_postPipeline, nullptr);
        m_postPipeline = VK_NULL_HANDLE;
    }

    if (m_postPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_postPipelineLayout, nullptr);
        m_postPipelineLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    if (m_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
        m_framebuffer = VK_NULL_HANDLE;
    }

    if (m_msaaRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_msaaRenderPass, nullptr);
        m_msaaRenderPass = VK_NULL_HANDLE;
    }

    if (m_offscreenRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_offscreenRenderPass, nullptr);
        m_offscreenRenderPass = VK_NULL_HANDLE;
    }

    if (m_offscreenImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_offscreenImage, nullptr);
        m_offscreenImage = VK_NULL_HANDLE;
    }

    if (m_offscreenMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_offscreenMemory, nullptr);
        m_offscreenMemory = VK_NULL_HANDLE;
    }

    if (m_offscreenImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_offscreenImageView, nullptr);
        m_offscreenImageView = VK_NULL_HANDLE;
    }

    for (auto imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }

    if (m_swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        m_swapChain = VK_NULL_HANDLE;
    }

    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

}

VkDevice Vulkan::GetDevice() {

    return m_device;

}

void Vulkan::DrawFrame(GLFWwindow* window) {

    VkResult result;

    if (m_framebufferResized) {
        RecreateSwapChain(window);
        m_framebufferResized = false;
    }

    VK_CHECK(vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX));

    VK_CHECK(vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]));

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain(window);
        return;
    }

    VK_CHECK(vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0));

    RecordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapChain(window);
        return;
    }

    m_currentFrame = (m_currentFrame + 1) % SETTINGS.MAX_FRAMES_IN_FLIGHT;

}

void Vulkan::SetResolution(GLFWwindow* window, uint32_t width, uint32_t height) {

    SETTINGS.WIDTH = width;
    SETTINGS.HEIGHT = height;

    float scaleX, scaleY;
    glfwGetWindowContentScale(window, &scaleX, &scaleY);

    if (!SETTINGS.FULLSCREEN) {
        glfwSetWindowMonitor(window, nullptr, 0, 0, static_cast<int>(static_cast<float>(SETTINGS.WIDTH) / scaleX), static_cast<int>(static_cast<float>(SETTINGS.HEIGHT) / scaleY), GLFW_DONT_CARE);
    }

    RecreateSwapChain(window);
    RecreatePipeline();

}

void Vulkan::SetMSAA(VkSampleCountFlagBits msaa) {

    if (find(m_msaaSamples.begin(), m_msaaSamples.end(), msaa) == m_msaaSamples.end()) {
        throw std::runtime_error("MSAA " + std::to_string(msaa) + "x does not supported!");
    }

    SETTINGS.MSAA_SAMPLES = msaa;

    RecreatePipeline();

}

void Vulkan::SetFilter(VkFilter filter) {

    SETTINGS.FILTER = filter;

    RecreateSampler();

}

void Vulkan::SetFramebufferResized(bool value) {

    m_framebufferResized = value;

}

void Vulkan::SetAspectRatioEnabled(bool value) {

    SETTINGS.KEEP_ASPECT_RATIO = value;

}

void Vulkan::SetFullscreenEnabled(GLFWwindow* window, bool value) {

    SETTINGS.FULLSCREEN = value;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    float scaleX, scaleY;
    glfwGetWindowContentScale(window, &scaleX, &scaleY);

    if (SETTINGS.FULLSCREEN) {
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, nullptr, 0, 0, static_cast<int>(static_cast<float>(SETTINGS.WIDTH) / scaleX), static_cast<int>(static_cast<float>(SETTINGS.HEIGHT)/ scaleY), GLFW_DONT_CARE);
    }

    RecreateSwapChain(window);

}

std::vector<Vulkan::Resolution> Vulkan::GetVideoModes() {

    return m_videoModes;

}

void Vulkan::CreateInstance() {

    if (enableValidationLayers && !CheckValidationLayerSupport()) {
        throw std::runtime_error("Validation layers are not available!");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "Indigo";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));

}

bool Vulkan::CheckValidationLayerSupport() {

    uint32_t layerCount;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;

}

std::vector<const char*> Vulkan::GetRequiredExtensions() {

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if constexpr (enableValidationLayers) {
        for (const char* ext : extensions) {
            std::cout << "EXTENSION: " << ext << std::endl;
        }
    }

    return extensions;

}

void Vulkan::SetupDebugMessenger() {

    if (enableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);
        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

}

void Vulkan::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {

    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;

}

void Vulkan::CreateSurface(GLFWwindow *window) {

    VK_CHECK(glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface));

}

void Vulkan::CreateSupportedVideoModes() {

    // Tworzy listę typowych rozdzielczości (4:3, 16:9, 16:10, 21:9, 32:9, 5:4, 3:2)
    m_videoModes = {
        {320, 200, "(16:10)"},
        {320, 240, "(4:3)"},
        {640, 480, "(4:3)"},
        {720, 480, "(3:2)"},
        {800, 600, "(4:3)"},
        {960, 640, "(3:2)"},
        {1024, 768, "(4:3)"},
        {1280, 720, "(16:9)"},    // HD
        {1280, 800, "(16:10)"},
        {1280, 1024, "(5:4)"},
        {1360, 768, "(16:9)"},
        {1366, 768, "(16:9)"},
        {1400, 1050, "(4:3)"},
        {1440, 900, "(16:10)"},
        {1440, 960, "(3:2)"},
        {1600, 900, "(16:9)"},
        {1600, 1200, "(4:3)"},
        {1680, 1050, "(16:10)"},
        {1920, 1080, "(16:9)"},   // FullHD
        {1920, 1200, "(16:10)"},
        {2048, 1152, "(16:9)"},
        {2048, 1536, "(4:3)"},
        {2160, 1440, "(3:2)"},
        {2240, 1400, "(16:10)"},
        {2560, 1080, "(21:9)"},
        {2560, 1440, "(16:9)"},   // 2K
        {2560, 1600, "(16:10)"},
        {2560, 1920, "(4:3)"},
        {2560, 2048, "(5:4)"},
        {2880, 1800, "(16:10)"},
        {3000, 2000, "(3:2)"},
        {3200, 1800, "(16:9)"},   // 3K
        {3240, 2160, "(3:2)"},
        {3440, 1440, "(21:9)"},
        {3840, 1080, "(32:9)"},
        {3840, 1600, "(21:9)"},
        {3840, 2160, "(16:9)"},   // 4K
        {4500, 3000, "(3:2)"},
        {5120, 1440, "(32:9)"},
        {5120, 2160, "(21:9)"},
        {5120, 2880, "(16:9)"},   // 5K
        {6016, 3384, "(16:9)"},
        {6144, 3456, "(16:9)"},   // 6K
        {7680, 2160, "(32:9)"},
        {7680, 4320, "(16:9)"}    // 8K
    };

    // Pobiera rozdzielczość natywną monitora
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    // Leci po tablicy i usuwa ją oraz wszystkie wyższe.
    m_videoModes.erase(
        remove_if(m_videoModes.begin(), m_videoModes.end(),
            [&](const Resolution& res) {
                return res.w > mode->width || res.h > mode->height || (res.w == mode->width && res.h == mode->height);
            }),
        m_videoModes.end()
    );

    // Wylicza współczynnik proporcji obrazu (szuka najbliższe ratio)...
    float ratio = static_cast<float>(mode->width) / static_cast<float>(mode->height);

    struct R { float value; std::string name; };

    R known[] = {
        {4.0f/3.0f, "(4:3)"},
        {16.0f/10.0f, "(16:10)"},
        {16.0f/9.0f, "(16:9)"},
        {21.0f/9.0f, "(21:9)"},
        {32.0f/9.0f, "(32:9)"},
        {3.0f/2.0f, "(3:2)"},
        {5.0f/4.0f, "(5:4)"}
    };

    float bestDiff = std::numeric_limits<float>::max();
    std::string best = "custom";

    for (auto& k : known) {
        float diff = std::abs(ratio - k.value);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = k.name + " (native)";
        }
    }

    Resolution current = { mode->width, mode->height, best };

    // ...aby następnie dodać ją na koniec listy
    // (w przypadku gdy nie ma jej na liście to i tak doda ją na koniec listy jako rozdzielczość natywną monitora)
    m_videoModes.insert(m_videoModes.end(), current);


    for (auto& r : m_videoModes) {
        std::cout << "Video mode: " << r.w << "x" << r.h << " " << r.ratio << std::endl;
    }

}

void Vulkan::PickPhysicalDevice() {

    // Wylicza katy graficzne wspierające Vulkan
    uint32_t deviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));

    // Jeśli nie ma to błąd
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    // Tworzy listę kart graficznych wspierających Vulkan
    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()));

    // Sprawdza każde znalezione urządzenie po kolei
    VkPhysicalDevice integrated = VK_NULL_HANDLE;
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);
        if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_physicalDevice = device; // Najlepsza opcja to dedykowana karta
            break;
        }
        if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            integrated = device; // Zintegrowana tylko jak nie ma discrete gpu
        }
    }

    // Jeśli jest zinegrowana a nie ma dedykowanej to wybiera zintegrowaną
    if (m_physicalDevice == VK_NULL_HANDLE && integrated != VK_NULL_HANDLE) {
        m_physicalDevice = integrated;
    }

    // Sprawdza czy w wybranym urzadzeniu GPU jest rodzina kolejek
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    if (!indices.isComplete()) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);
    std::cout << "Physical device selected: " << physicalDeviceProperties.deviceName << std::endl;

    // Sprawdza dodatkowe cechy karty np. obsługa geometry shader
    // vertex shader i fragment shader to podstawa - nie sprawdza się tego
    //VkPhysicalDeviceFeatures deviceFeatures;
    //vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    //if (!deviceFeatures.geometryShader) {
    //    throw std::runtime_error("Geometry shader not supported!");
    //} else if (enableValidationLayers) std::cout << "Geometry shader supported!" << std::endl;

    // Sprawdza czy karta obsługuje rozszerzenia z listy deviceExtensions
    bool extensionsSupported = CheckDeviceExtensionSupport(m_physicalDevice);
    if (!extensionsSupported) {
        throw std::runtime_error("Extensions not supported!");
    }

    // Sprawdza obsługę swapchain (formaty i tryby wyświetlania)
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    if (!swapChainAdequate) {
        throw std::runtime_error("SwapChain not supported!");
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

}

bool Vulkan::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {

    uint32_t extensionCount;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr));

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data()));

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();

}

void Vulkan::CreateSupportedSampleCounts(VkPhysicalDevice physicalDevice) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_1_BIT) m_msaaSamples.push_back(VK_SAMPLE_COUNT_1_BIT);
    if (counts & VK_SAMPLE_COUNT_2_BIT) m_msaaSamples.push_back(VK_SAMPLE_COUNT_2_BIT);
    if (counts & VK_SAMPLE_COUNT_4_BIT) m_msaaSamples.push_back(VK_SAMPLE_COUNT_4_BIT);
    if (counts & VK_SAMPLE_COUNT_8_BIT) m_msaaSamples.push_back(VK_SAMPLE_COUNT_8_BIT);
    if (counts & VK_SAMPLE_COUNT_16_BIT) m_msaaSamples.push_back(VK_SAMPLE_COUNT_16_BIT);
    if (counts & VK_SAMPLE_COUNT_32_BIT) m_msaaSamples.push_back(VK_SAMPLE_COUNT_32_BIT);
    if (counts & VK_SAMPLE_COUNT_64_BIT) m_msaaSamples.push_back(VK_SAMPLE_COUNT_64_BIT);

    for (auto s : m_msaaSamples) {
        std::cout << "MSAA " << s << "x supported!" << std::endl;
    }

}

void Vulkan::CreateLogicalDevice() {

    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

    if constexpr (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

}

void Vulkan::CreateSwapChain(GLFWwindow *window) {

    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain));
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr));

    m_swapChainImages.resize(imageCount);

    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data()));

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

}

Vulkan::SwapChainSupportDetails Vulkan::QuerySwapChainSupport(VkPhysicalDevice device) {

    SwapChainSupportDetails details;

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities));

    uint32_t formatCount;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr));

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data()));
    }

    uint32_t presentModeCount;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr));

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data()));
    }

    return details;

}

VkSurfaceFormatKHR Vulkan::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

    // 1. Najlepszy wybór (sRGB)
    for (const auto& f : availableFormats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }

    // 2. Drugi wybór (UNORM + sRGB colorspace)
    for (const auto& f : availableFormats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }

    // 3. Ostateczność
    return availableFormats[0];

}

VkPresentModeKHR Vulkan::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;

}

VkExtent2D Vulkan::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

}

void Vulkan::CreateImageViews() {

    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]));

    }

}

void Vulkan::CreateOffscreenResources() {

    // Tworzymy obraz (render target)
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = SETTINGS.WIDTH;
    imageInfo.extent.height = SETTINGS.HEIGHT;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_swapChainImageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(m_device, &imageInfo, nullptr, &m_offscreenImage));

    // Pobranie wymagań pamięci
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_offscreenImage, &memRequirements);

    // Alokacja pamięci
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_offscreenMemory));

    // Podpięcie pamięci do obrazu
    VK_CHECK(vkBindImageMemory(m_device, m_offscreenImage, m_offscreenMemory, 0));

    // Tworzymy ImageView
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_offscreenImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_swapChainImageFormat;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_offscreenImageView));

}

void Vulkan::CreateOffscreenRenderPass() {

    if (SETTINGS.MSAA_SAMPLES != VK_SAMPLE_COUNT_1_BIT) return;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_offscreenRenderPass));

}

void Vulkan::CreateMSAAResources() {

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) return;

    // ===== MSAA IMAGE =====
    VkImageCreateInfo imageInfoMSAA{};
    imageInfoMSAA.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfoMSAA.imageType = VK_IMAGE_TYPE_2D;
    imageInfoMSAA.extent.width = SETTINGS.WIDTH;
    imageInfoMSAA.extent.height = SETTINGS.HEIGHT;
    imageInfoMSAA.extent.depth = 1;
    imageInfoMSAA.mipLevels = 1;
    imageInfoMSAA.arrayLayers = 1;
    imageInfoMSAA.format = m_swapChainImageFormat;
    imageInfoMSAA.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfoMSAA.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfoMSAA.samples = SETTINGS.MSAA_SAMPLES;
    imageInfoMSAA.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfoMSAA.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(m_device, &imageInfoMSAA, nullptr, &m_msaaImage));

    // memory
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_device, m_msaaImage, &memReq);

    VkMemoryAllocateInfo allocInfo2{};
    allocInfo2.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo2.allocationSize = memReq.size;
    allocInfo2.memoryTypeIndex = FindMemoryType(
        memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    VK_CHECK(vkAllocateMemory(m_device, &allocInfo2, nullptr, &m_msaaMemory));

    VK_CHECK(vkBindImageMemory(m_device, m_msaaImage, m_msaaMemory, 0));

    // image view
    VkImageViewCreateInfo viewInfoMSAA{};
    viewInfoMSAA.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfoMSAA.image = m_msaaImage;
    viewInfoMSAA.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfoMSAA.format = m_swapChainImageFormat;

    viewInfoMSAA.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfoMSAA.subresourceRange.levelCount = 1;
    viewInfoMSAA.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device, &viewInfoMSAA, nullptr, &m_msaaImageView));

}

void Vulkan::CreateMSAARenderPass() {

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) return;

    // MSAA attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = SETTINGS.MSAA_SAMPLES;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // resolve (offscreenImage)
    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = m_swapChainImageFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveRef{};
    resolveRef.attachment = 1;
    resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pResolveAttachments = &resolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependency2{};
    dependency2.srcSubpass = 0;
    dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {
        colorAttachment,
        resolveAttachment
    };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    VkSubpassDependency dependencies[] = {dependency, dependency2};
    renderPassInfo.pDependencies = dependencies;

    VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_msaaRenderPass));

}

void Vulkan::CreateFramebuffer() {

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.width = SETTINGS.WIDTH;
    framebufferInfo.height = SETTINGS.HEIGHT;
    framebufferInfo.layers = 1;

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {
        framebufferInfo.renderPass = m_offscreenRenderPass;
    } else {
        framebufferInfo.renderPass = m_msaaRenderPass;
    }

    VkImageView attachments[2];

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {

        attachments[0] = m_offscreenImageView;

        framebufferInfo.attachmentCount = 1;

    } else {

        attachments[0] = m_msaaImageView;
        attachments[1] = m_offscreenImageView;

        framebufferInfo.attachmentCount = 2;
    }

    framebufferInfo.pAttachments = attachments;

    VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffer));

}

void Vulkan::CreateRenderPass() {

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass));

}

void Vulkan::CreateDescriptorSetLayout() {

    // Binding (czyli sampler2D tex)
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0; // musi się zgadzać z shaderem!
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding.pImmutableSamplers = nullptr;

    // Layout
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout));

}

void Vulkan::CreateScenePipeline() {

    auto vertShaderCode = ReadFile("../shaders/vert.spv");
    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);

    auto fragShaderCode = ReadFile("../shaders/frag.spv");
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = SETTINGS.MSAA_SAMPLES;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_scenePipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_scenePipelineLayout;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {
        pipelineInfo.renderPass = m_offscreenRenderPass;
    } else {
        pipelineInfo.renderPass = m_msaaRenderPass;
    }

    VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_scenePipeline));

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

}

void Vulkan::CreatePostPipeline() {

    auto vertShaderCode = ReadFile("../shaders/post_vert.spv");
    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);

    auto fragShaderCode = ReadFile("../shaders/post_frag.spv");
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertShaderModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragShaderModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // brak vertexów (fullscreen triangle)
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlend{};
    colorBlend.colorWriteMask = 0xF;
    colorBlend.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlend;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // TU DOJDZIE descriptor layout (za chwilę)
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;

    VK_CHECK(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_postPipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_postPipelineLayout;
    pipelineInfo.renderPass = m_renderPass;

    VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_postPipeline));

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

}

void Vulkan::CreateSampler() {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // NAJWAŻNIEJSZE — jakość skalowania
    samplerInfo.magFilter = SETTINGS.FILTER;
    samplerInfo.minFilter = SETTINGS.FILTER;

    // adresowanie (poza UV 0–1)
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler));

}

void Vulkan::CreateDescriptorPool() {

    // Typ descriptora (musi pasować do layoutu)
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    // Tworzenie poola
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool));

}

void Vulkan::CreateDescriptorSet() {

    // Alokacja descriptor seta
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet));

    // Opis obrazu (tekstury)
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_offscreenImageView;
    imageInfo.sampler = m_sampler;

    // Update descriptor seta
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0; // musi się zgadzać z shaderem
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

}

void Vulkan::UpdateDescriptorSet() {

    // Opis obrazu (tekstury)
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_offscreenImageView;
    imageInfo.sampler = m_sampler;

    // Update descriptor seta
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0; // musi się zgadzać z shaderem
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

}

void Vulkan::CreateFramebuffers() {

    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]));

    }

}

void Vulkan::CreateCommandPool() {

    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool));

}

void Vulkan::CreateCommandBuffers() {

    m_commandBuffers.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()));

}

void Vulkan::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    // =========================================================
    // PASS 1 — OFFSCREEN/MSAA
    // =========================================================

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = m_framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {SETTINGS.WIDTH, SETTINGS.HEIGHT};

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {
        renderPassInfo.renderPass = m_offscreenRenderPass;
    } else {
        renderPassInfo.renderPass = m_msaaRenderPass;
    }

    VkClearValue clearColorPass1 = {{{0.0f, 0.0f, 1.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColorPass1;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_scenePipeline);

    VkViewport viewport1{};
    viewport1.x = 0.0f;
    viewport1.y = 0.0f;
    viewport1.width = static_cast<float>(SETTINGS.WIDTH);
    viewport1.height = static_cast<float>(SETTINGS.HEIGHT);
    viewport1.minDepth = 0.0f;
    viewport1.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport1);

    VkRect2D scissor1{};
    scissor1.offset = {0, 0};
    scissor1.extent = {SETTINGS.WIDTH, SETTINGS.HEIGHT};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor1);

    // rysujesz scenę (np. trójkąt)
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    // =========================================================
    // PASS 2 — SWAPCHAIN (ekran)
    // =========================================================

    VkRenderPassBeginInfo renderPassInfo2{};
    renderPassInfo2.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo2.renderPass = m_renderPass;
    renderPassInfo2.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo2.renderArea.offset = {0, 0};
    renderPassInfo2.renderArea.extent = m_swapChainExtent;

    VkClearValue clearColorPass2 = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo2.clearValueCount = 1;
    renderPassInfo2.pClearValues = &clearColorPass2;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo2, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postPipeline);

    // descriptor set (tekstura)
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_postPipelineLayout,
        0,
        1,
        &m_descriptorSet,
        0,
        nullptr
    );

    if (SETTINGS.KEEP_ASPECT_RATIO) {

        float aspectRender = static_cast<float>(SETTINGS.WIDTH) / static_cast<float>(SETTINGS.HEIGHT);
        float aspectScreen = static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height);

        float width, height;

        if (aspectScreen > aspectRender) {
            // ekran szerszy → pasy po bokach
            height = static_cast<float>(m_swapChainExtent.height);
            width = height * aspectRender;
        } else {
            // ekran wyższy → pasy góra/dół
            width = static_cast<float>(m_swapChainExtent.width);
            height = width / aspectRender;
        }

        float x = (static_cast<float>(m_swapChainExtent.width) - width) / 2.0f;
        float y = (static_cast<float>(m_swapChainExtent.height) - height) / 2.0f;

        VkViewport viewport2{};
        viewport2.x = x;
        viewport2.y = y;
        viewport2.width = width;
        viewport2.height = height;
        viewport2.minDepth = 0.0f;
        viewport2.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport2);

        VkRect2D scissor2{};
        scissor2.offset = {static_cast<int>(x), static_cast<int>(y)};
        scissor2.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor2);

    } else {

        VkViewport viewport2{};
        viewport2.x = 0;
        viewport2.y = 0;
        viewport2.width = static_cast<float>(m_swapChainExtent.width);
        viewport2.height = static_cast<float>(m_swapChainExtent.height);
        viewport2.minDepth = 0.0f;
        viewport2.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport2);

        VkRect2D scissor2{};
        scissor2.offset = {0, 0};
        scissor2.extent = {m_swapChainExtent.width, m_swapChainExtent.height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor2);
    }

    // fullscreen triangle
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    // =========================================================

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

}

void Vulkan::CreateSyncObjects() {

    m_imageAvailableSemaphores.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < SETTINGS.MAX_FRAMES_IN_FLIGHT; ++i) {
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]));
        VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]));
    }

}

void Vulkan::RecreateSwapChain(GLFWwindow* window) {

    //int width = 0, height = 0;
    //while (width == 0 || height == 0) {
    //    glfwGetFramebufferSize(window, &width, &height);
    //    glfwWaitEvents();
    //}

    vkDeviceWaitIdle(m_device);

    // cleanup starego
    CleanupSwapChain();

    // tworzenie od nowa
    CreateSwapChain(window);
    CreateImageViews();
    CreateFramebuffers();

}

void Vulkan::RecreatePipeline() {

    vkDeviceWaitIdle(m_device);

    // Niszczenie starego
    vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);

    if (m_offscreenRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_offscreenRenderPass, nullptr);
        m_offscreenRenderPass = VK_NULL_HANDLE;
    }

    if (m_msaaRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_msaaRenderPass, nullptr);
        m_msaaRenderPass = VK_NULL_HANDLE;
    }

    vkDestroyPipeline(m_device, m_scenePipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_scenePipelineLayout, nullptr);

    vkDestroyImage(m_device, m_offscreenImage, nullptr);
    vkFreeMemory(m_device, m_offscreenMemory, nullptr);
    vkDestroyImageView(m_device, m_offscreenImageView, nullptr);

    if (m_msaaImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_msaaImageView, nullptr);
        m_msaaImageView = VK_NULL_HANDLE;
    }
    if (m_msaaImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_msaaImage, nullptr);
        m_msaaImage = VK_NULL_HANDLE;
    }
    if (m_msaaMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_msaaMemory, nullptr);
        m_msaaMemory = VK_NULL_HANDLE;
    }

    // Tworzenie nowego
    CreateOffscreenResources();
    CreateMSAAResources();
    CreateOffscreenRenderPass();
    CreateMSAARenderPass();
    CreateFramebuffer();
    CreateScenePipeline();

    // DESCRIPTORY (bo imageView się zmienił)
    VK_CHECK(vkResetDescriptorPool(m_device, m_descriptorPool, 0));

    CreateDescriptorSet();

}

void Vulkan::RecreateSampler() {

    VK_CHECK(vkWaitForFences(m_device, SETTINGS.MAX_FRAMES_IN_FLIGHT, m_inFlightFences.data(), VK_TRUE, UINT64_MAX));

    // Niszczenie starego
    vkDestroySampler(m_device, m_sampler, nullptr);

    // Tworzenie nowego
    CreateSampler();

    // Update descriptora
    UpdateDescriptorSet();

}

Vulkan::QueueFamilyIndices Vulkan::FindQueueFamilies(VkPhysicalDevice device) {

    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport));

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;

}

std::vector<char> Vulkan::ReadFile(const std::string& filename) {

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    file.close();

    return buffer;

}

VkShaderModule Vulkan::CreateShaderModule(const std::vector<char> &code) {

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VK_CHECK(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule));

    return shaderModule;

}

void Vulkan::CleanupSwapChain() {

    for (auto framebuffer_ : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer_, nullptr);
    }

    for (auto imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

}

uint32_t Vulkan::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {

        // sprawdzamy czy typ pamięci jest dozwolony
        if (typeFilter & (1 << i)) {

            // sprawdzamy czy ma wymagane właściwości
            if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");

}

VulkanContext Vulkan::GetContext() const {

    VulkanContext ctx{};
    ctx.device = m_device;
    ctx.physicalDevice = m_physicalDevice;
    ctx.commandPool = m_commandPool;
    ctx.graphicsQueue = m_graphicsQueue;
    return ctx;

}
