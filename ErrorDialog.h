#pragma once

#include <string>
#include <vulkan/vulkan.h>

void ShowErrorDialog(const std::string& errorMessage);
const char* VkResultToString(VkResult r);
[[noreturn]] void VulkanThrowError(const char* call, VkResult result, const char* file, int line);
std::string EscapeMarkup(const std::string& text);
const char* GetFileName(const char* path);

#define VK_CHECK(call)                                                              \
    do {                                                                            \
        VkResult result = call;                                                     \
        if (result != VK_SUCCESS) {                                                 \
            VulkanThrowError(#call, result, GetFileName(__FILE__), __LINE__);       \
        }                                                                           \
    } while (0)

