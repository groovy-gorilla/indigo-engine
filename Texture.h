#ifndef TEXTURECLASS_H
#define TEXTURECLASS_H

#include "Global.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class Texture {

public:
    Texture();
    ~Texture();

    void Initialize(const VulkanContext& ctx, const std::string& filename);
    void Shutdown(const VulkanContext& ctx);

private:
    VkImage m_image;
    VkDeviceMemory m_memory;
    VkImageView m_imageView;
    VkSampler m_sampler;
    uint32_t m_mipLevels;

    void CreateBuffer(const VulkanContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CreateImage(const VulkanContext& ctx, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& memory);
    void TransitionImageLayout(const VulkanContext& ctx, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void CopyBufferToImage(const VulkanContext& ctx, VkBuffer buffer, VkImage image, const std::vector<VkBufferImageCopy>& regions);
    void CreateImageView(VkDevice device, VkImage image, VkFormat format, uint32_t mipLevels, VkImageView& imageView);
    uint32_t FindMemoryType(const VulkanContext& ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkCommandBuffer BeginSingleTimeCommands(const VulkanContext& ctx);
    void EndSingleTimeCommands(const VulkanContext& ctx, VkCommandBuffer commandBuffer);

};

#endif //TEXTURECLASS_H
