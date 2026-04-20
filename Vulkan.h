#ifndef VULKANCLASS_H
#define VULKANCLASS_H

#include "Global.h"
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <fstream>

class Vulkan {

public:
    struct Resolution { int w; int h; std::string ratio; };

    Vulkan();
    ~Vulkan();

    void Initialize(GLFWwindow *window);
    void Shutdown();

    VkDevice GetDevice();
    void DrawFrame(GLFWwindow* window);
    void SetResolution(GLFWwindow* window, uint32_t width, uint32_t height);
    void SetMSAA(VkSampleCountFlagBits msaa);
    void SetFilter(VkFilter filter);
    void SetFramebufferResized(bool value);
    void SetAspectRatioEnabled(bool value);
    void SetFullscreenEnabled(GLFWwindow* window, bool value);
    std::vector<Resolution> GetVideoModes();
    VulkanContext GetContext() const;

private:

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};


    #ifdef NDEBUG
        static constexpr bool enableValidationLayers = false;
    #else
        static constexpr bool enableValidationLayers = true;
    #endif

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    VkImage m_offscreenImage;
    VkDeviceMemory m_offscreenMemory;
    VkImageView m_offscreenImageView;
    VkImage m_msaaImage;
    VkDeviceMemory m_msaaMemory;
    VkImageView m_msaaImageView;
    VkRenderPass m_offscreenRenderPass;
    VkRenderPass m_msaaRenderPass;
    VkFramebuffer m_framebuffer;
    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_scenePipelineLayout;
    VkPipeline m_scenePipeline;
    VkPipelineLayout m_postPipelineLayout;
    VkPipeline m_postPipeline;
    VkSampler m_sampler;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkSampleCountFlagBits> m_msaaSamples;
    std::vector<Resolution> m_videoModes;
    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;

    void CreateInstance();
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
    void SetupDebugMessenger();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void CreateSurface(GLFWwindow *window);
    void CreateSupportedVideoModes();
    void PickPhysicalDevice();
    bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
    void CreateSupportedSampleCounts(VkPhysicalDevice physicalDevice);
    void CreateLogicalDevice();
    void CreateSwapChain(GLFWwindow* window);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow* window);
    void CreateImageViews();
    void CreateOffscreenResources();
    void CreateOffscreenRenderPass();
    void CreateMSAAResources();
    void CreateMSAARenderPass();
    void CreateFramebuffer();
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreateScenePipeline();
    void CreatePostPipeline();
    void CreateSampler();
    void CreateDescriptorPool();
    void CreateDescriptorSet();
    void UpdateDescriptorSet();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void CreateSyncObjects();
    void RecreateSwapChain(GLFWwindow* window);
    void RecreatePipeline();
    void RecreateSampler();
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    std::vector<char> ReadFile(const std::string &filename);
    VkShaderModule CreateShaderModule(const std::vector<char> &code);
    void CleanupSwapChain();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

};



#endif //VULKANCLASS_H