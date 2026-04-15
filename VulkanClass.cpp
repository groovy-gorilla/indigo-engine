#include "VulkanClass.h"
#include "Settings.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>

VulkanClass::VulkanClass() {

    instance = VK_NULL_HANDLE;
    debugMessenger = VK_NULL_HANDLE;
    surface = VK_NULL_HANDLE;
    physicalDevice = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
    graphicsQueue = VK_NULL_HANDLE;
    presentQueue = VK_NULL_HANDLE;
    swapChain = VK_NULL_HANDLE;
    renderPass = VK_NULL_HANDLE;
    scenePipelineLayout = VK_NULL_HANDLE;
    scenePipeline = VK_NULL_HANDLE;
    msaaImage = VK_NULL_HANDLE;
    msaaImageView = VK_NULL_HANDLE;
    msaaMemory = VK_NULL_HANDLE;
    offscreenImage = VK_NULL_HANDLE;
    offscreenImageView = VK_NULL_HANDLE;
    offscreenMemory = VK_NULL_HANDLE;
    offscreenRenderPass = VK_NULL_HANDLE;
    msaaRenderPass = VK_NULL_HANDLE;
    framebuffer = VK_NULL_HANDLE;
    descriptorSetLayout = VK_NULL_HANDLE;
    postPipelineLayout = VK_NULL_HANDLE;
    postPipeline = VK_NULL_HANDLE;
    sampler = VK_NULL_HANDLE;
    descriptorPool = VK_NULL_HANDLE;
    descriptorSet = VK_NULL_HANDLE;
    commandPool = VK_NULL_HANDLE;

}

VulkanClass::~VulkanClass() = default;

void VulkanClass::Initialize(GLFWwindow *window) {

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface(window);
    PickPhysicalDevice();
    CreateSupportedSampleCounts(physicalDevice);
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

void VulkanClass::Shutdown() {

    if (msaaImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, msaaImageView, nullptr);
        msaaImageView = VK_NULL_HANDLE;
    }

    if (msaaImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, msaaImage, nullptr);
        msaaImage = VK_NULL_HANDLE;
    }

    if (msaaMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, msaaMemory, nullptr);
        msaaMemory = VK_NULL_HANDLE;
    }

    for (size_t i=0; i < SETTINGS.MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        vkDestroyFence(device, inFlightFences[i], nullptr);
        inFlightFences[i] = VK_NULL_HANDLE;
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    commandPool = VK_NULL_HANDLE;

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }

    vkDestroyPipeline(device, scenePipeline, nullptr);
    scenePipeline = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(device, scenePipelineLayout, nullptr);
    scenePipelineLayout = VK_NULL_HANDLE;

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;

    vkDestroySampler(device, sampler, nullptr);
    sampler = VK_NULL_HANDLE;

    vkDestroyPipeline(device, postPipeline, nullptr);
    postPipeline = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(device, postPipelineLayout, nullptr);
    postPipelineLayout = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    descriptorSetLayout = VK_NULL_HANDLE;

    vkDestroyRenderPass(device, renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;

    vkDestroyFramebuffer(device, framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;

    vkDestroyRenderPass(device, offscreenRenderPass, nullptr);
    offscreenRenderPass = VK_NULL_HANDLE;

    vkDestroyImage(device, offscreenImage, nullptr);
    offscreenImage = VK_NULL_HANDLE;

    vkFreeMemory(device, offscreenMemory, nullptr);
    offscreenMemory = VK_NULL_HANDLE;

    vkDestroyImageView(device, offscreenImageView, nullptr);
    offscreenImageView = VK_NULL_HANDLE;

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    swapChain = VK_NULL_HANDLE;

    vkDestroyDevice(device, nullptr);
    device = VK_NULL_HANDLE;

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    surface = VK_NULL_HANDLE;

    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;

}

VkDevice VulkanClass::getDevice() {

    return device;

}

void VulkanClass::drawFrame(GLFWwindow* window) {

    if (framebufferResized) {
        RecreateSwapChain(window);
        framebufferResized = false;
    }

    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain(window);
        return;
    }

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapChain(window);
        return;
    }

    currentFrame = (currentFrame + 1) % SETTINGS.MAX_FRAMES_IN_FLIGHT;

}

void VulkanClass::SetResolution(uint32_t width, uint32_t height) {

    SETTINGS.WIDTH = width;
    SETTINGS.HEIGHT = height;

    RecreatePipeline();

}

void VulkanClass::SetMSAA(VkSampleCountFlagBits msaa) {

    if (std::find(msaaSamples.begin(), msaaSamples.end(), msaa) == msaaSamples.end()) {
        throw std::runtime_error("MSAA " + std::to_string(msaa) + "x does not supported!");
    }

    SETTINGS.MSAA_SAMPLES = msaa;

    RecreatePipeline();

}

void VulkanClass::SetFilter(VkFilter filter) {

    SETTINGS.FILTER = filter;

    RecreateSampler();

}

void VulkanClass::CreateInstance() {

    if (enableValidationLayers && !CheckValidationLayerSupport()) {
        throw std::runtime_error("Validation layers are not available!");
    } else if (enableValidationLayers) std::cout << "Validation layers are available!" << std::endl;

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

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance!");
    } else  if (enableValidationLayers) std::cout << "Instance created!" << std::endl;

}

bool VulkanClass::CheckValidationLayerSupport() {

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

std::vector<const char*> VulkanClass::GetRequiredExtensions() {

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if (enableValidationLayers) {
        for (const char* ext : extensions) {
            std::cout << "EXTENSION: " << ext << std::endl;
        }
    }

    return extensions;

}

void VulkanClass::SetupDebugMessenger() {

    if (enableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up debug messenger!");
        } else if (enableValidationLayers) std::cout << "Debug messenger set up successfully!" << std::endl;
    }

}

void VulkanClass::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {

    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;

}

void VulkanClass::CreateSurface(GLFWwindow *window) {

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create surface!");
    } else if (enableValidationLayers) std::cout << "Surface created!" << std::endl;

}

void VulkanClass::PickPhysicalDevice() {

    // Wylicza katy graficzne wspierające Vulkan
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // Jeśli nie ma to błąd
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    // Tworzy listę kart graficznych wspierających Vulkan
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Sprawdza każde znalezione urządzenie po kolei
    VkPhysicalDevice integrated = VK_NULL_HANDLE;
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);
        if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physicalDevice = device; // Najlepsza opcja to dedykowana karta
            break;
        }
        if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            integrated = device; // Zintegrowana tylko jak nie ma discrete gpu
        }
    }

    // Jeśli jest zinegrowana a nie ma dedykowanej to wybiera zintegrowaną
    if (physicalDevice == VK_NULL_HANDLE && integrated != VK_NULL_HANDLE) {
        physicalDevice = integrated;
    }

    // Sprawdza czy w wybranym urzadzeniu GPU jest rodzina kolejek
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
    if (!indices.isComplete()) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    std::cout << "Physical device selected: " << physicalDeviceProperties.deviceName << std::endl;

    // Sprawdza dodatkowe cechy karty np. obsługa geometry shader
    // vertex shader i fragment shader to podstawa - nie sprawdza się tego
    //VkPhysicalDeviceFeatures deviceFeatures;
    //vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    //if (!deviceFeatures.geometryShader) {
    //    throw std::runtime_error("Geometry shader not supported!");
    //} else if (enableValidationLayers) std::cout << "Geometry shader supported!" << std::endl;

    // Sprawdza czy karta obsługuje rozszerzenia z listy deviceExtensions
    bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
    if (!extensionsSupported) {
        throw std::runtime_error("Extensions not supported!");
    } else if (enableValidationLayers) std::cout << "Extensions supported!" << std::endl;

    // Sprawdza obsługę swapchain (formaty i tryby wyświetlania)
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    if (!swapChainAdequate) {
        throw std::runtime_error("SwapChain not supported!");
    } else if (enableValidationLayers) std::cout << "SwapChain supported!" << std::endl;

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

}

bool VulkanClass::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();

}

void VulkanClass::CreateSupportedSampleCounts(VkPhysicalDevice physicalDevice) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_1_BIT) msaaSamples.push_back(VK_SAMPLE_COUNT_1_BIT);
    if (counts & VK_SAMPLE_COUNT_2_BIT) msaaSamples.push_back(VK_SAMPLE_COUNT_2_BIT);
    if (counts & VK_SAMPLE_COUNT_4_BIT) msaaSamples.push_back(VK_SAMPLE_COUNT_4_BIT);
    if (counts & VK_SAMPLE_COUNT_8_BIT) msaaSamples.push_back(VK_SAMPLE_COUNT_8_BIT);
    if (counts & VK_SAMPLE_COUNT_16_BIT) msaaSamples.push_back(VK_SAMPLE_COUNT_16_BIT);
    if (counts & VK_SAMPLE_COUNT_32_BIT) msaaSamples.push_back(VK_SAMPLE_COUNT_32_BIT);
    if (counts & VK_SAMPLE_COUNT_64_BIT) msaaSamples.push_back(VK_SAMPLE_COUNT_64_BIT);

    for (auto s : msaaSamples) {
        std::cout << "MSAA " << s << "x supported!" << std::endl;
    }

}

void VulkanClass::CreateLogicalDevice() {

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

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

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    } else if (enableValidationLayers) std::cout << "Logical device created!" << std::endl;

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

}

void VulkanClass::CreateSwapChain(GLFWwindow *window) {

    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
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

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create SwapChain!");
    } else if (enableValidationLayers) std::cout << "SwapChain created!" << std::endl;

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

}

SwapChainSupportDetails VulkanClass::QuerySwapChainSupport(VkPhysicalDevice device) {

    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;

}

VkSurfaceFormatKHR VulkanClass::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];

}

VkPresentModeKHR VulkanClass::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;

}

VkExtent2D VulkanClass::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {

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

void VulkanClass::CreateImageViews() {

    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        } else if (enableValidationLayers) std::cout << "Image view created! (" << i << ")" << std::endl;
    }

}

void VulkanClass::CreateOffscreenResources() {

    // Tworzymy obraz (render target)
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = SETTINGS.WIDTH;
    imageInfo.extent.height = SETTINGS.HEIGHT;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = swapChainImageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &offscreenImage) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen image!");
    }

    // Pobranie wymagań pamięci
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, offscreenImage, &memRequirements);

    // Alokacja pamięci
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (vkAllocateMemory(device, &allocInfo, nullptr, &offscreenMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate offscreen memory!");
    }

    // Podpięcie pamięci do obrazu
    vkBindImageMemory(device, offscreenImage, offscreenMemory, 0);

    // Tworzymy ImageView
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = offscreenImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChainImageFormat;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &offscreenImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen image view!");
    }

}

void VulkanClass::CreateOffscreenRenderPass() {

    if (SETTINGS.MSAA_SAMPLES != VK_SAMPLE_COUNT_1_BIT) return;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
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

    vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenRenderPass);

}

void VulkanClass::CreateMSAAResources() {

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
    imageInfoMSAA.format = swapChainImageFormat;
    imageInfoMSAA.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfoMSAA.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfoMSAA.samples = SETTINGS.MSAA_SAMPLES;
    imageInfoMSAA.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfoMSAA.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfoMSAA, nullptr, &msaaImage) != VK_SUCCESS) {
        throw std::runtime_error("MSAA image failed!");
    }

    // memory
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, msaaImage, &memReq);

    VkMemoryAllocateInfo allocInfo2{};
    allocInfo2.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo2.allocationSize = memReq.size;
    allocInfo2.memoryTypeIndex = FindMemoryType(
        memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (vkAllocateMemory(device, &allocInfo2, nullptr, &msaaMemory) != VK_SUCCESS) {
        throw std::runtime_error("MSAA memory failed!");
    }

    vkBindImageMemory(device, msaaImage, msaaMemory, 0);

    // image view
    VkImageViewCreateInfo viewInfoMSAA{};
    viewInfoMSAA.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfoMSAA.image = msaaImage;
    viewInfoMSAA.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfoMSAA.format = swapChainImageFormat;

    viewInfoMSAA.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfoMSAA.subresourceRange.levelCount = 1;
    viewInfoMSAA.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfoMSAA, nullptr, &msaaImageView) != VK_SUCCESS) {
        throw std::runtime_error("MSAA view failed!");
    }

}

void VulkanClass::CreateMSAARenderPass() {

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) return;

    // MSAA attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = SETTINGS.MSAA_SAMPLES;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // resolve (offscreenImage)
    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = swapChainImageFormat;
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

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &msaaRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create MSAA render pass!");
    }

}

void VulkanClass::CreateFramebuffer() {

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.width = SETTINGS.WIDTH;
    framebufferInfo.height = SETTINGS.HEIGHT;
    framebufferInfo.layers = 1;

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {
        framebufferInfo.renderPass = offscreenRenderPass;
    } else {
        framebufferInfo.renderPass = msaaRenderPass;
    }

    VkImageView attachments[2];

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {

        attachments[0] = offscreenImageView;

        framebufferInfo.attachmentCount = 1;

    } else {

        attachments[0] = msaaImageView;
        attachments[1] = offscreenImageView;

        framebufferInfo.attachmentCount = 2;
    }

    framebufferInfo.pAttachments = attachments;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen framebuffer!");
    }

}

void VulkanClass::CreateRenderPass() {

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
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

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    } else if (enableValidationLayers) std::cout << "Render pass created!" << std::endl;

}

void VulkanClass::CreateDescriptorSetLayout() {

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

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

}

void VulkanClass::CreateScenePipeline() {

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

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &scenePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    } else if (enableValidationLayers) std::cout << "Pipeline layout created!" << std::endl;

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
    pipelineInfo.layout = scenePipelineLayout;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {
        pipelineInfo.renderPass = offscreenRenderPass;
    } else {
        pipelineInfo.renderPass = msaaRenderPass;
    }

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &scenePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    } else if (enableValidationLayers) std::cout << "Graphics pipeline created!" << std::endl;

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

}

void VulkanClass::CreatePostPipeline() {

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
    dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    // TU DOJDZIE descriptor layout (za chwilę)
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &postPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create post pipeline layout!");
    }

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
    pipelineInfo.layout = postPipelineLayout;
    pipelineInfo.renderPass = renderPass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &postPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create post pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

}

void VulkanClass::CreateSampler() {

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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create sampler!");
    }

}

void VulkanClass::CreateDescriptorPool() {

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

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

}

void VulkanClass::CreateDescriptorSet() {

    // Alokacja descriptor seta
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set!");
    }

    // Opis obrazu (tekstury)
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = offscreenImageView;
    imageInfo.sampler = sampler;

    // Update descriptor seta
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0; // musi się zgadzać z shaderem
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

}

void VulkanClass::CreateFramebuffers() {

    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        } else if (enableValidationLayers) std::cout << "Framebuffer created! (" << i << ")" << std::endl;
    }

}

void VulkanClass::CreateCommandPool() {

    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    } else if (enableValidationLayers) std::cout << "Command pool created!" << std::endl;

}

void VulkanClass::CreateCommandBuffers() {

    commandBuffers.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    } else if (enableValidationLayers) std::cout << "Command buffer created!" << std::endl;

}

void VulkanClass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // =========================================================
    // PASS 1 — OFFSCREEN/MSAA
    // =========================================================

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {SETTINGS.WIDTH, SETTINGS.HEIGHT};

    if (SETTINGS.MSAA_SAMPLES == VK_SAMPLE_COUNT_1_BIT) {
        renderPassInfo.renderPass = offscreenRenderPass;
    } else {
        renderPassInfo.renderPass = msaaRenderPass;
    }

    VkClearValue clearColorPass1 = {{{0.0f, 0.0f, 1.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColorPass1;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);

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
    renderPassInfo2.renderPass = renderPass;
    renderPassInfo2.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo2.renderArea.offset = {0, 0};
    renderPassInfo2.renderArea.extent = swapChainExtent;

    VkClearValue clearColorPass2 = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo2.clearValueCount = 1;
    renderPassInfo2.pClearValues = &clearColorPass2;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo2, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postPipeline);

    // descriptor set (tekstura)
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        postPipelineLayout,
        0,
        1,
        &descriptorSet,
        0,
        nullptr
    );

    if (SETTINGS.ASPECT_RATIO) {

        float aspectRender = static_cast<float>(SETTINGS.WIDTH) / SETTINGS.HEIGHT;
        float aspectScreen = static_cast<float>(swapChainExtent.width) / swapChainExtent.height;

        float width, height;

        if (aspectScreen > aspectRender) {
            // ekran szerszy → pasy po bokach
            height = static_cast<float>(swapChainExtent.height);
            width = height * aspectRender;
        } else {
            // ekran wyższy → pasy góra/dół
            width = static_cast<float>(swapChainExtent.width);
            height = width / aspectRender;
        }

        float x = (swapChainExtent.width - width) / 2.0f;
        float y = (swapChainExtent.height - height) / 2.0f;

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
        scissor2.extent = {(uint32_t)width, (uint32_t)height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor2);

    } else {

        VkViewport viewport2{};
        viewport2.x = 0;
        viewport2.y = 0;
        viewport2.width = swapChainExtent.width;
        viewport2.height = swapChainExtent.height;
        viewport2.minDepth = 0.0f;
        viewport2.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport2);

        VkRect2D scissor2{};
        scissor2.offset = {0, 0};
        scissor2.extent = {swapChainExtent.width, swapChainExtent.height};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor2);
    }

    // fullscreen triangle
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    // =========================================================

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }

}

void VulkanClass::CreateSyncObjects() {

    imageAvailableSemaphores.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(SETTINGS.MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < SETTINGS.MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
            } else if (enableValidationLayers) std::cout << "Synchronization objects created!" << std::endl;

    }

}

void VulkanClass::RecreateSwapChain(GLFWwindow* window) {

    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    // cleanup starego
    CleanupSwapChain();

    // tworzenie od nowa
    CreateSwapChain(window);
    CreateImageViews();
    CreateFramebuffers();

}

void VulkanClass::RecreatePipeline() {

    vkDeviceWaitIdle(device);

    // Niszczenie starego
    vkDestroyFramebuffer(device, framebuffer, nullptr);

    if (offscreenRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, offscreenRenderPass, nullptr);
        offscreenRenderPass = VK_NULL_HANDLE;
    }

    if (msaaRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, msaaRenderPass, nullptr);
        msaaRenderPass = VK_NULL_HANDLE;
    }

    vkDestroyPipeline(device, scenePipeline, nullptr);
    vkDestroyPipelineLayout(device, scenePipelineLayout, nullptr);

    vkDestroyImage(device, offscreenImage, nullptr);
    vkFreeMemory(device, offscreenMemory, nullptr);
    vkDestroyImageView(device, offscreenImageView, nullptr);

    if (msaaImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, msaaImageView, nullptr);
        msaaImageView = VK_NULL_HANDLE;
    }
    if (msaaImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, msaaImage, nullptr);
        msaaImage = VK_NULL_HANDLE;
    }
    if (msaaMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, msaaMemory, nullptr);
        msaaMemory = VK_NULL_HANDLE;
    }

    // Tworzenie nowego
    CreateOffscreenResources();
    CreateMSAAResources();
    CreateOffscreenRenderPass();
    CreateMSAARenderPass();
    CreateFramebuffer();
    CreateScenePipeline();

    // DESCRIPTORY (bo imageView się zmienił)
    vkResetDescriptorPool(device, descriptorPool, 0);
    CreateDescriptorSet();

}

void VulkanClass::RecreateSampler() {

    vkDeviceWaitIdle(device);

    // Niszczenie starego
    vkDestroySampler(device, sampler, nullptr);

    // Tworzenie nowego
    CreateSampler();
    vkResetDescriptorPool(device, descriptorPool, 0);
    CreateDescriptorSet();

}

QueueFamilyIndices VulkanClass::FindQueueFamilies(VkPhysicalDevice device) {

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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

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

std::vector<char> VulkanClass::ReadFile(const std::string& filename) {

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    } else if (enableValidationLayers) std::cout << filename << " file opened!" << std::endl;

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;

}

VkShaderModule VulkanClass::CreateShaderModule(const std::vector<char>& code) {

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    } else if (enableValidationLayers) std::cout << "Created shader module!" << std::endl;

    return shaderModule;

}

void VulkanClass::CleanupSwapChain() {

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);

}

uint32_t VulkanClass::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

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
