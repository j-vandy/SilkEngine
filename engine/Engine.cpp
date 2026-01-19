#include "Engine.h"
#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>

namespace silk
{
    void validateVkResult(VkResult result, const char* msg)
    {
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(msg);
        }
    }

    VkSurfaceFormatKHR getPhysicalDeviceSurfaceFormat(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
    {
        uint32_t surfaceFormatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data());

        for (const auto& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return surfaceFormat;
            }
        }

        return surfaceFormats[0];
    }

    SwapchainContext::SwapchainContext(GLFWwindow* window, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex, const VkSurfaceKHR& surface, const VkDevice& device, const VkRenderPass& renderPass) : device(device)
    {
        // create VkSwapchainKHR
        VkSurfaceFormatKHR surfaceFormat;
        {
            // surface format
            surfaceFormat = getPhysicalDeviceSurfaceFormat(physicalDevice, surface);

            // present mode
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

            VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
            for (const auto& availablePresentMode : presentModes)
            {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    presentMode = availablePresentMode;
                }
            }

            // extent
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
            if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                extent = surfaceCapabilities.currentExtent;
            }
            else
            {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                extent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                extent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
                extent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
            }

            uint32_t imageCount= surfaceCapabilities.minImageCount + 1;
            if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
            {
                imageCount = surfaceCapabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR swapchainCreateInfo{};
            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.surface = surface;
            swapchainCreateInfo.minImageCount = imageCount;
            swapchainCreateInfo.imageFormat = surfaceFormat.format;
            swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
            swapchainCreateInfo.imageExtent = extent;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

            if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
            {
                swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchainCreateInfo.queueFamilyIndexCount = 2;
                swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
                swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }

            swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
            swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainCreateInfo.presentMode = presentMode;
            swapchainCreateInfo.clipped = VK_TRUE;
            swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

            validateVkResult(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain), "Error: failed to create VkSwapchainKHR!");
        }

        // create VkImageViews
        {
            std::vector<VkImage> swapchainImages;
            uint32_t imageCount;
            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
            swapchainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

            imageViews.resize(swapchainImages.size());

            for (size_t i = 0; i < swapchainImages.size(); i++)
            {
                VkImageViewCreateInfo imageViewCreateInfo{};
                imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewCreateInfo.image = swapchainImages[i];
                imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewCreateInfo.format = surfaceFormat.format;
                imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                imageViewCreateInfo.subresourceRange.layerCount = 1;

                validateVkResult(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageViews[i]), "Error: failed to create VkImageView!");
            }
        }

        // create VkFramebuffers
        {
            framebuffers.resize(imageViews.size());

            for (size_t i = 0; i < imageViews.size(); i++)
            {
                VkImageView attachments[] = { imageViews[i] };

                VkFramebufferCreateInfo framebufferCreateInfo{};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = renderPass;
                framebufferCreateInfo.attachmentCount = 1;
                framebufferCreateInfo.pAttachments = attachments;
                framebufferCreateInfo.width = extent.width;
                framebufferCreateInfo.height = extent.height;
                framebufferCreateInfo.layers = 1;

                validateVkResult(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[i]), "Error: failed to create VkFramebuffers");
            }
        }

        std::cout << "Create SwapchainContext\n";
    }

    SwapchainContext::~SwapchainContext()
    {
        vkDeviceWaitIdle(device);

        // destroy VkFramebuffers
        for (auto framebuffer: framebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        // destroy VkImageViews
        for (auto imageView : imageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        // destroy VkSwapchainKHR
        vkDestroySwapchainKHR(device, swapchain, nullptr);

        std::cout << "Destroy SwapchainContext\n";
    }

    const VkExtent2D& SwapchainContext::getExtent() const { return extent; }

    const VkSwapchainKHR& SwapchainContext::getSwapchain() const { return swapchain; }

    const std::vector<VkFramebuffer>& SwapchainContext::getFramebuffers() const { return framebuffers; }

    std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error(std::format("Error: failed to open {}!", filename));
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    VkResult createVkShaderModule(const VkDevice& device, VkShaderModule& shaderModule, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = code.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    }
    
    PipelineContext::~PipelineContext()
    {
        vkDeviceWaitIdle(device);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        std::cout << "Destroy PipelineContext\n";
    }

    const VkPipelineLayout& PipelineContext::getPipelineLayout() const { return pipelineLayout; }

    const VkPipeline& PipelineContext::getPipeline() const { return pipeline; }

    // glm::mat4 Camera::getOrthoMatrix(uint32_t screenWidth, uint32_t screenHeight) const
    // {
    //     float aspect = static_cast<float>(screenWidth) / screenHeight;
    //     float width = fovYAxis * aspect;
    //     const float Z_NEAR = -1.0f;
    //     const float Z_FAR = 1.0f;
    //     return glm::ortho(-width/2.0f, width/2.0f, -fovYAxis/2.0f, fovYAxis/2.0f, Z_NEAR, Z_FAR);
    // }

    // VkVertexInputBindingDescription InstanceData::getBindingDescription(uint32_t binding)
    // {
    //     VkVertexInputBindingDescription bindingDescription{};
    //     bindingDescription.binding = binding;
    //     bindingDescription.stride = sizeof(InstanceData);
    //     bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    //     return bindingDescription;
    // }

    // std::array<VkVertexInputAttributeDescription, 5> InstanceData::getAttributeDescriptions(uint32_t binding, uint32_t location)
    // {
    //     std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions;

    //     for (size_t i = 0; i < 4; i++)
    //     {
    //         attributeDescriptions[i].location = location + i;
    //         attributeDescriptions[i].binding = binding;
    //         attributeDescriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //         attributeDescriptions[i].offset = offsetof(InstanceData, model) + sizeof(glm::vec4) * i;
    //     }

    //     attributeDescriptions[4].location = location + 4;
    //     attributeDescriptions[4].binding = binding;
    //     attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    //     attributeDescriptions[4].offset = offsetof(InstanceData, tint);

    //     return attributeDescriptions;
    // }


    // void framebufferResizeCallback(GLFWwindow* window, int width __attribute__((unused)), int height __attribute__((unused)))
    // {
    //     Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    //     if (!engine)
    //     {
    //         throw std::runtime_error("Error: unable to cast GLFW window user pointer to Engine*!");
    //     }
    //     engine->shouldResizeFramebuffer();
    // }

    // static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity __attribute__((unused)), VkDebugUtilsMessageTypeFlagsEXT messageType __attribute__((unused)), const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData __attribute__((unused)))
    // {
    //     std::cout << "Validation Layer: " << pCallbackData->pMessage << std::endl;
    //     return VK_FALSE;
    // }

    // static std::vector<char> readFile(const std::string& filename)
    // {
    //     std::ifstream file(filename, std::ios::ate | std::ios::binary);
    //     if (!file.is_open())
    //     {
    //         throw std::runtime_error(std::format("Error: failed to open {}!", filename));
    //     }

    //     size_t fileSize = (size_t) file.tellg();
    //     std::vector<char> buffer(fileSize);

    //     file.seekg(0);
    //     file.read(buffer.data(), fileSize);
    //     file.close();

    //     return buffer;
    // }

    // Engine::Engine(int width, int height, const char* applicationName)
    // {
    //     // init glfw
    //     glfwInit();
    //     glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //     window = glfwCreateWindow(width, height, applicationName, nullptr, nullptr);
    //     glfwSetWindowUserPointer(window, this);
    //     glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    //     // create VkInstance
    //     VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
    //     {
    //         // check validation layer support
    //         if (ENABLE_VALIDATION_LAYERS)
    //         {
    //             uint32_t layerCount;
    //             vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    //             std::vector<VkLayerProperties> availableLayers(layerCount);
    //             vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    //             std::set<std::string> requiredValidationLayers(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
    //             for (const auto& layerProperties : availableLayers)
    //             {
    //                 requiredValidationLayers.erase(layerProperties.layerName);
    //             }

    //             if (!requiredValidationLayers.empty())
    //             {
    //                 throw std::runtime_error("Error: validation layers requested but not available!");
    //             }
    //         }

    //         VkApplicationInfo applicationInfo{};
    //         applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    //         applicationInfo.pApplicationName = applicationName;
    //         applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    //         applicationInfo.pEngineName = ENGINE_NAME;
    //         applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    //         applicationInfo.apiVersion = VK_API_VERSION_1_0;

    //         VkInstanceCreateInfo instanceCreateInfo{};
    //         instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    //         instanceCreateInfo.pApplicationInfo = &applicationInfo;

    //         uint32_t glfwExtensionCount = 0;
    //         const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    //         std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    //         if (ENABLE_VALIDATION_LAYERS)
    //         {
    //             extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    //         }

    //         instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    //         instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    //         if (ENABLE_VALIDATION_LAYERS)
    //         {
    //             instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    //             instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

    //             debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    //             debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    //             debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    //             debugMessengerCreateInfo.pfnUserCallback = debugCallback;

    //             instanceCreateInfo.pNext = &debugMessengerCreateInfo;
    //         }
    //         else
    //         {
    //             instanceCreateInfo.enabledLayerCount = 0;
    //             instanceCreateInfo.pNext = nullptr;
    //         }

    //         if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkInstance!");
    //         }
    //     }

    //     // create VkDebugUtilsMessengerEXT
    //     if (ENABLE_VALIDATION_LAYERS)
    //     {
    //         auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    //         if (func == nullptr || func(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkDebugUtilsMessengerEXT!");
    //         }
    //     }

    //     // create VkSurfaceKHR
    //     if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    //     {
    //         std::runtime_error("Error: failed to create VkSurfaceKHR!");
    //     }

    //     // pick VkPhysicalDevice
    //     physicalDevice = VK_NULL_HANDLE;
    //     {
    //         uint32_t physicalDeviceCount = 0;
    //         vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    //         if (physicalDeviceCount == 0)
    //         {
    //             std::runtime_error("Error: failed to find GPUs with Vulkan support!");
    //         }

    //         std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    //         vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    //         for (const auto& physDev : physicalDevices)
    //         {
    //             // queue family support
    //             std::optional<uint32_t> graphicsIndex, presentIndex;

    //             uint32_t queueFamilyCount = 0;
    //             vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);
    //             std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    //             vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, queueFamilies.data());

    //             int i = 0;
    //             for (const auto& queueFamily : queueFamilies)
    //             {
    //                 if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    //                 {
    //                     graphicsIndex = i;
    //                 }

    //                 VkBool32 presentSupport = false;
    //                 vkGetPhysicalDeviceSurfaceSupportKHR(physDev, i, surface, &presentSupport);
    //                 if (presentSupport)
    //                 {
    //                     presentIndex = i;
    //                 }

    //                 if (graphicsIndex.has_value() && presentIndex.has_value())
    //                 {
    //                     break;
    //                 }

    //                 i++;
    //             }

    //             // extension support
    //             uint32_t extensionCount;
    //             vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, nullptr);
    //             std::vector<VkExtensionProperties> extensions(extensionCount);
    //             vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, extensions.data());

    //             std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
    //             for (const auto& extension : extensions)
    //             {
    //                 requiredExtensions.erase(extension.extensionName);
    //             }

    //             // swapchain support
    //             uint32_t surfaceFormatCount;
    //             vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &surfaceFormatCount, nullptr);

    //             uint32_t presentModeCount;
    //             vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, nullptr);

    //             if (graphicsIndex.has_value()
    //                 && presentIndex.has_value()
    //                 && requiredExtensions.empty()
    //                 && surfaceFormatCount != 0
    //                 && presentModeCount != 0)
    //             {
    //                 physicalDevice = physDev;
    //                 graphicsQueueFamilyIndex = graphicsIndex.value();
    //                 presentQueueFamilyIndex = presentIndex.value();
    //                 break;
    //             }
    //         }

    //         if (physicalDevice == VK_NULL_HANDLE)
    //         {
    //             std::runtime_error("Error: failed to find a suitable GPU!");
    //         }
    //     }

    //     // create (logical) VkDevice
    //     {
    //         std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    //         std::set<uint32_t> uniqueQueueFamilies = {
    //             graphicsQueueFamilyIndex,
    //             presentQueueFamilyIndex
    //         };

    //         float queuePriority = 1.0f;
    //         for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
    //         {
    //             VkDeviceQueueCreateInfo queueCreateInfo{};
    //             queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    //             queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    //             queueCreateInfo.queueCount = 1;
    //             queueCreateInfo.pQueuePriorities = &queuePriority;
    //             queueCreateInfos.push_back(queueCreateInfo);
    //         }

    //         VkPhysicalDeviceFeatures deviceFeatures{};

    //         VkDeviceCreateInfo deviceCreateInfo{};
    //         deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //         deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    //         deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    //         deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    //         deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    //         deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    //         if (ENABLE_VALIDATION_LAYERS)
    //         {
    //             deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    //             deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    //         }
    //         else
    //         {
    //             deviceCreateInfo.enabledLayerCount = 0;
    //         }

    //         if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkDevice!");
    //         }

    //         vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    //         vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
    //     }

    //     // create VkSwapchainKHR
    //     VkFormat swapchainImageFormat;
    //     if (createSwapchain(swapchainImageFormat) != VK_SUCCESS)
    //     {
    //         std::runtime_error("Error: failed to create VkSwapchainKHR!");
    //     }

    //     // create VkImageViews
    //     if (createImageViews(swapchainImageFormat) != VK_SUCCESS)
    //     {
    //         std::runtime_error("Error: failed to create VkImageViews!");
    //     }

    //     // create VkRenderPass
    //     {
    //         VkAttachmentDescription colorAttachmentDescription{};
    //         colorAttachmentDescription.format = swapchainImageFormat;
    //         colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    //         colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //         colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //         colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //         colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //         colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //         colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    //         VkAttachmentReference colorAttachmentReference{};
    //         colorAttachmentReference.attachment = 0;
    //         colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //         VkSubpassDescription subpass{};
    //         subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //         subpass.colorAttachmentCount = 1;
    //         subpass.pColorAttachments = &colorAttachmentReference;

    //         VkRenderPassCreateInfo renderPassCreateInfo{};
    //         renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //         renderPassCreateInfo.attachmentCount = 1;
    //         renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
    //         renderPassCreateInfo.subpassCount = 1;
    //         renderPassCreateInfo.pSubpasses = &subpass;

    //         if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkRenderPass!");
    //         }
    //     }

    //     // create VkDescriptorSetLayout
    //     {
    //         VkDescriptorSetLayoutBinding uboLayoutBinding{};
    //         uboLayoutBinding.binding = 0;
    //         uboLayoutBinding.descriptorCount = 1;
    //         uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //         uboLayoutBinding.pImmutableSamplers = nullptr;
    //         uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //         VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    //         descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //         descriptorSetLayoutCreateInfo.bindingCount = 1;
    //         descriptorSetLayoutCreateInfo.pBindings = &uboLayoutBinding;

    //         if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkDescriptorSetLayout!");
    //         }
    //     }

    //     // create (graphics) VkPipeline
    //     {
    //         // relative to binary dir
    //         std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
    //         std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

    //         VkShaderModule vertShaderModule;
    //         if (createShaderModule(vertShaderModule, vertShaderCode) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create vertex VkShaderModule!");
    //         }

    //         VkShaderModule fragShaderModule;
    //         if (createShaderModule(fragShaderModule, fragShaderCode) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create fragment VkShaderModule!");
    //         }

    //         VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
    //         vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    //         vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    //         vertShaderStageCreateInfo.module = vertShaderModule;
    //         vertShaderStageCreateInfo.pName = "main";

    //         VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
    //         fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    //         fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    //         fragShaderStageCreateInfo.module = fragShaderModule;
    //         fragShaderStageCreateInfo.pName = "main";

    //         VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo};

    //         VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    //         vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    //         std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    //         vertexBindingDescriptions.push_back(Vertex::getBindingDescription());

    //         uint32_t instanceBinding = 1;
    //         uint32_t instanceLocation = 1;
    //         vertexBindingDescriptions.push_back(InstanceData::getBindingDescription(instanceBinding));

    //         std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    //         for (auto vertexAttributeDescription : Vertex::getAttributeDescriptions())
    //         {
    //             vertexAttributeDescriptions.push_back(vertexAttributeDescription);
    //         }
    //         for (auto instanceAttributeDescription : InstanceData::getAttributeDescriptions(instanceBinding, instanceLocation))
    //         {
    //             vertexAttributeDescriptions.push_back(instanceAttributeDescription);
    //         }

    //         vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
    //         vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    //         vertexInputCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
    //         vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    //         VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    //         inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    //         inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    //         inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    //         VkPipelineViewportStateCreateInfo viewportCreateInfo{};
    //         viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    //         viewportCreateInfo.viewportCount = 1;
    //         viewportCreateInfo.scissorCount = 1;

    //         VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
    //         rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    //         rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    //         rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    //         rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    //         rasterizationCreateInfo.lineWidth = 1.0f;
    //         // TODO: rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    //         rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE;
    //         rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    //         rasterizationCreateInfo.depthBiasEnable = VK_FALSE;

    //         VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
    //         multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    //         multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    //         multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //         VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    //         colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    //         colorBlendAttachment.blendEnable = VK_FALSE;

    //         VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
    //         colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    //         colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    //         colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    //         colorBlendCreateInfo.attachmentCount = 1;
    //         colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
    //         colorBlendCreateInfo.blendConstants[0] = 0.0f;
    //         colorBlendCreateInfo.blendConstants[1] = 0.0f;
    //         colorBlendCreateInfo.blendConstants[2] = 0.0f;
    //         colorBlendCreateInfo.blendConstants[3] = 0.0f;

    //         std::vector<VkDynamicState> dynamicStates = {
    //             VK_DYNAMIC_STATE_VIEWPORT,
    //             VK_DYNAMIC_STATE_SCISSOR
    //         };
    //         VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    //         dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //         dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    //         dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    //         VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    //         pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    //         pipelineLayoutCreateInfo.setLayoutCount = 1;
    //         pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    //         pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

    //         if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkPipelineLayout!");
    //         }

    //         VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    //         graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    //         graphicsPipelineCreateInfo.stageCount = 2;
    //         graphicsPipelineCreateInfo.pStages = shaderStages;
    //         graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    //         graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    //         graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
    //         graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    //         graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    //         graphicsPipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    //         graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    //         graphicsPipelineCreateInfo.layout = pipelineLayout;
    //         graphicsPipelineCreateInfo.renderPass = renderPass;
    //         graphicsPipelineCreateInfo.subpass = 0;
    //         graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    //         if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkPipeline!");
    //         }

    //         vkDestroyShaderModule(device, vertShaderModule, nullptr);
    //         vkDestroyShaderModule(device, fragShaderModule, nullptr);
    //     }

    //     // create VkFramebuffers
    //     if (createFramebuffers() != VK_SUCCESS)
    //     {
    //         std::runtime_error("Error: failed to create VkFramebuffers!");
    //     }

    //     // create VkCommandPool
    //     {
    //         VkCommandPoolCreateInfo commandPoolCreateInfo{};
    //         commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //         commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //         commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

    //         if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkCommandPool!");
    //         }
    //     }

    //     // create (vertex) VkBuffer
    //     {
    //         VkDeviceSize vertexBufferSize = sizeof(QUAD_VERTICES[0]) * QUAD_VERTICES.size();

    //         VkBuffer stagingBuffer;
    //         VkDeviceMemory stagingBufferMemory;
    //         if (createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create vertex staging buffer!");
    //         }

    //         void* data;
    //         vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    //             memcpy(data, QUAD_VERTICES.data(), static_cast<size_t>(vertexBufferSize));
    //         vkUnmapMemory(device, stagingBufferMemory);

    //         if (createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create vertex buffer!");
    //         }

    //         if (copyBuffer(stagingBuffer, vertexBuffer, vertexBufferSize) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to copy buffer!");
    //         }

    //         vkDestroyBuffer(device, stagingBuffer, nullptr);
    //         vkFreeMemory(device, stagingBufferMemory, nullptr);
    //     }

    //     // create (instance) VkBuffer
    //     instanceCounts.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    //     if (createInstanceBuffers() != VK_SUCCESS)
    //     {
    //         std::runtime_error("Error: failed to create instance buffers!");
    //     }

    //     // create (index) VkBuffer
    //     {
    //         indexCount = QUAD_INDICES.size();
    //         VkDeviceSize indexBufferSize = sizeof(QUAD_INDICES[0]) * QUAD_INDICES.size(); 

    //         VkBuffer stagingBuffer;
    //         VkDeviceMemory stagingBufferMemory;
    //         if (createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create index staging buffer!");
    //         }

    //         void* data;
    //         vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
    //             memcpy(data, QUAD_INDICES.data(), static_cast<size_t>(indexBufferSize));
    //         vkUnmapMemory(device, stagingBufferMemory);

    //         if (createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create index buffer!");
    //         }

    //         if (copyBuffer(stagingBuffer, indexBuffer, indexBufferSize) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to copy buffer!");
    //         }

    //         vkDestroyBuffer(device, stagingBuffer, nullptr);
    //         vkFreeMemory(device, stagingBufferMemory, nullptr);
    //     }

    //     // create (uniform) VkBuffer
    //     {
    //         uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //         uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    //         uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    //         VkDeviceSize bufferSize = sizeof(CameraUBO);
    //         for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //         {
    //             if (createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]) != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to create uniform buffers!");
    //             }
    //             if (vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]) != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to map uniform buffers memory!");
    //             }
    //         }
    //     }

    //     // create VkDescriptorPool
    //     {
    //         VkDescriptorPoolSize descriptorPoolSize{};
    //         descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //         descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    //         VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    //         descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    //         descriptorPoolCreateInfo.poolSizeCount = 1;
    //         descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
    //         descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    //         if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkDescriptorPool!");
    //         }
    //     }

    //     // create VkDescriptorSets
    //     {
    //         std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    //         VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    //         descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    //         descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    //         descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    //         descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

    //         descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    //         if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets.data()) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create VkDescriptorSets!");
    //         }

    //         for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //         {
    //             VkDescriptorBufferInfo descriptorBufferInfo{};
    //             descriptorBufferInfo.buffer = uniformBuffers[i];
    //             descriptorBufferInfo.offset = 0;
    //             descriptorBufferInfo.range = sizeof(CameraUBO);

    //             VkWriteDescriptorSet writeDescriptorSet{};
    //             writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //             writeDescriptorSet.dstSet = descriptorSets[i];
    //             writeDescriptorSet.dstBinding = 0;
    //             writeDescriptorSet.dstArrayElement = 0;
    //             writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //             writeDescriptorSet.descriptorCount = 1;
    //             writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    //             vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    //         }
    //     }

    //     // allocate VkCommandBuffer
    //     {
    //         commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    //         VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    //         commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //         commandBufferAllocateInfo.commandPool = commandPool;
    //         commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //         commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    //         if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to allocate VkCommandBuffer!");
    //         }
    //     }

    //     // create synchronization objects
    //     {
    //         imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    //         renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    //         inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    //         VkSemaphoreCreateInfo semaphoreCreateInfo{};
    //         semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    //         VkFenceCreateInfo fenceCreateInfo{};
    //         fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //         fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    //         for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //         {
    //             if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
    //                 || vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to create VkSemaphores!");
    //             }
    //             if (vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to create VkFence!");
    //             }
    //         }
    //     }
    // }

    // Engine::~Engine()
    // {
    //     vkDeviceWaitIdle(device);
    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         vkDestroyFence(device, inFlightFences[i], nullptr);
    //         vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    //         vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    //     }
    //     vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    //         vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    //     }
    //     vkFreeMemory(device, indexBufferMemory, nullptr);
    //     vkDestroyBuffer(device, indexBuffer, nullptr);
    //     cleanupInstanceBuffers();
    //     vkFreeMemory(device, vertexBufferMemory, nullptr);
    //     vkDestroyBuffer(device, vertexBuffer, nullptr);
    //     vkDestroyCommandPool(device, commandPool, nullptr);
    //     vkDestroyPipeline(device, graphicsPipeline, nullptr);
    //     vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    //     vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    //     vkDestroyRenderPass(device, renderPass, nullptr);
    //     cleanupSwapchain();
    //     vkDestroyDevice(device, nullptr);
    //     vkDestroySurfaceKHR(instance, surface, nullptr);
    //     if (ENABLE_VALIDATION_LAYERS)
    //     {
    //         auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    //         if (func != nullptr)
    //         {
    //             func(instance, debugMessenger, nullptr);
    //         }
    //     }
    //     vkDestroyInstance(instance, nullptr);
    //     glfwDestroyWindow(window);
    //     glfwTerminate();
    // }

    // void Engine::run()
    // {
    //     auto previousTime = std::chrono::high_resolution_clock::now();

    //     while(!glfwWindowShouldClose(window))
    //     {
    //         glfwPollEvents();

    //         auto currentTime = std::chrono::high_resolution_clock::now();
    //         float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
    //         previousTime = currentTime;

    //         // TODO: move this update callback once we have a frame?
    //         for (std::function<void(float)> fn : updateCallbacks)
    //         {
    //             fn(deltaTime);
    //         }

    //         // physics 

    //         // draw frame
    //         {
    //             vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    //             uint32_t imageIndex;
    //             VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    //             if (result == VK_ERROR_OUT_OF_DATE_KHR)
    //             {
    //                 if (recreateSwapchain() != VK_SUCCESS)
    //                 {
    //                     std::runtime_error("Error: failed to recreate swapchain!");
    //                 }
    //                 continue;
    //             }
    //             else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    //             {
    //                 std::runtime_error("Error: failed to aquire next swapchain image!");
    //             }

    //             vkResetFences(device, 1, &inFlightFences[currentFrame]);

    //             vkResetCommandBuffer(commandBuffers[currentFrame], 0);
                
    //             // record command buffer
    //             VkCommandBufferBeginInfo commandBufferBeginInfo{};
    //             commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    //             if (vkBeginCommandBuffer(commandBuffers[currentFrame], &commandBufferBeginInfo) != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to begin command buffer!");
    //             }

    //             VkRenderPassBeginInfo renderPassBeginInfo{};
    //             renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    //             renderPassBeginInfo.renderPass = renderPass;
    //             renderPassBeginInfo.framebuffer = swapchainFramebuffers[imageIndex];
    //             renderPassBeginInfo.renderArea.offset = {0, 0};
    //             renderPassBeginInfo.renderArea.extent = swapchainExtent;

    //             VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    //             renderPassBeginInfo.clearValueCount = 1;
    //             renderPassBeginInfo.pClearValues = &clearColor;

    //             vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    //                 vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    //                 VkViewport viewport{};
    //                 viewport.x = 0.0f;
    //                 viewport.y = 0.0f;
    //                 viewport.width = static_cast<float>(swapchainExtent.width);
    //                 viewport.height = static_cast<float>(swapchainExtent.height);
    //                 viewport.minDepth = 0.0f;
    //                 viewport.maxDepth = 1.0f;
    //                 vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

    //                 VkRect2D scissor{};
    //                 scissor.offset = {0, 0};
    //                 scissor.extent = swapchainExtent;
    //                 vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

    //                 VkBuffer vertexBuffers[] = { vertexBuffer, instanceBuffers[currentFrame] };
    //                 VkDeviceSize offsets[] = { 0, 0 };
    //                 vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 2, vertexBuffers, offsets);

    //                 vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    //                 vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    //                 vkCmdDrawIndexed(commandBuffers[currentFrame], indexCount, instanceCounts[currentFrame], 0, 0, 0);

    //             vkCmdEndRenderPass(commandBuffers[currentFrame]);

    //             if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to begin command buffer!");
    //             }

    //             VkSubmitInfo submitInfo{};
    //             submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //             VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    //             VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    //             submitInfo.waitSemaphoreCount = 1;
    //             submitInfo.pWaitSemaphores = waitSemaphores;
    //             submitInfo.pWaitDstStageMask = waitStages;
    //             submitInfo.commandBufferCount = 1;
    //             submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    //             VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    //             submitInfo.signalSemaphoreCount = 1;
    //             submitInfo.pSignalSemaphores = signalSemaphores;

    //             if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to submit queue!");
    //             }

    //             VkPresentInfoKHR presentInfo{};
    //             presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //             presentInfo.waitSemaphoreCount = 1;
    //             presentInfo.pWaitSemaphores = signalSemaphores;

    //             VkSwapchainKHR swapchains[] = { swapchain };
    //             presentInfo.swapchainCount = 1;
    //             presentInfo.pSwapchains = swapchains;
    //             presentInfo.pImageIndices = &imageIndex;

    //             result = vkQueuePresentKHR(presentQueue, &presentInfo);
    //             if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    //             {
    //                 framebufferResized = false;
    //                 if (recreateSwapchain() != VK_SUCCESS)
    //                 {
    //                     std::runtime_error("Error: failed to recreate swapchain!");
    //                 }
    //             }
    //             else if (result != VK_SUCCESS)
    //             {
    //                 std::runtime_error("Error: failed to present swapchain image!");
    //             }

    //             currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    //         }
    //     }
    // }

    // void Engine::shouldResizeFramebuffer()
    // {
    //     framebufferResized = true;
    // }

    // void Engine::memcpyCameraUBO(const CameraUBO& ubo)
    // {
    //     memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    // }

    // void Engine::updateInstanceBuffer(const std::vector<InstanceData>& instances)
    // {
    //     size_t instancesSize = instances.size();
    //     if (instancesSize > maxInstances)
    //     {
    //         std::cout << "RECREATE INSTANCE BUFFER!" << std::endl;
    //         vkDeviceWaitIdle(device);
    //         cleanupInstanceBuffers();

    //         maxInstances = instancesSize * 2;
    //         if (createInstanceBuffers() != VK_SUCCESS)
    //         {
    //             std::runtime_error("Error: failed to create instance buffers!");
    //         }
    //     }

    //     instanceCounts[currentFrame] = instancesSize;
    //     memcpy(instanceBuffersMapped[currentFrame], instances.data(), instancesSize * sizeof(instances[0]));
    // }

    // const VkExtent2D& Engine::getSwapchainExtent() const
    // {
    //     return swapchainExtent;
    // }

    // void Engine::getCursorWorldSpace(silk::Scene& scene, const silk::Entity& cam, float* x, float* y) const
    // {
    //     double screenPosX, screenPosY;
    //     glfwGetCursorPos(window, &screenPosX, &screenPosY);
    //     glm::vec4 screenSpace = glm::vec4(static_cast<float>(screenPosX), static_cast<float>(screenPosY), 0.0f, 1.0f);

    //     glm::mat4 invViewport = glm::scale(glm::mat4(1.0f), glm::vec3(2/static_cast<float>(swapchainExtent.width), -2/static_cast<float>(swapchainExtent.height),1.0f));
    //     invViewport[3] = glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f);

    //     glm::mat4 invProj = glm::inverse(scene.getComponent<Camera>(cam).getOrthoMatrix(swapchainExtent.width, swapchainExtent.height));
    //     glm::mat4 invView = scene.getComponent<silk::Transform>(cam).getMatrix();
    //     glm::vec4 worldSpace = invView * invProj * invViewport * screenSpace;

    //     *x = worldSpace.x;
    //     *y = worldSpace.y;
    // }

    // GLFWkeyfun Engine::setKeyCallback(GLFWkeyfun callback)
    // {
    //     return glfwSetKeyCallback(window, callback);
    // }

    // GLFWmousebuttonfun Engine::setMouseButtonCallback(GLFWmousebuttonfun callback)
    // {
    //     return glfwSetMouseButtonCallback(window, callback);
    // }

    // VkResult Engine::createSwapchain(VkFormat& swapchainImageFormat)
    // {
    //     // swapchain surface format
    //     uint32_t surfaceFormatCount;
    //     vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
    //     std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    //     vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data());

    //     VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    //     for (const auto& availableSurfaceFormat : surfaceFormats)
    //     {
    //         if (availableSurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    //         {
    //             surfaceFormat = availableSurfaceFormat;
    //         }
    //     }

    //     swapchainImageFormat = surfaceFormat.format;

    //     // swapchain present mode
    //     uint32_t presentModeCount;
    //     vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    //     std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    //     vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    //     VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    //     for (const auto& availablePresentMode : presentModes)
    //     {
    //         if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    //         {
    //             presentMode = availablePresentMode;
    //         }
    //     }

    //     // swapchain extent
    //     VkSurfaceCapabilitiesKHR surfaceCapabilities;
    //     vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

    //     if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    //     {
    //         swapchainExtent= surfaceCapabilities.currentExtent;
    //     }
    //     else
    //     {
    //         int width, height;
    //         glfwGetFramebufferSize(window, &width, &height);

    //         swapchainExtent= {
    //             static_cast<uint32_t>(width),
    //             static_cast<uint32_t>(height)
    //         };

    //         swapchainExtent.width = std::clamp(swapchainExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    //         swapchainExtent.height = std::clamp(swapchainExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    //     }

    //     uint32_t imageCount= surfaceCapabilities.minImageCount + 1;
    //     if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
    //     {
    //         imageCount = surfaceCapabilities.maxImageCount;
    //     }

    //     VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    //     swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //     swapchainCreateInfo.surface = surface;
    //     swapchainCreateInfo.minImageCount = imageCount;
    //     swapchainCreateInfo.imageFormat = surfaceFormat.format;
    //     swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    //     swapchainCreateInfo.imageExtent = swapchainExtent;
    //     swapchainCreateInfo.imageArrayLayers = 1;
    //     swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //     uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

    //     if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    //     {
    //         swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //         swapchainCreateInfo.queueFamilyIndexCount = 2;
    //         swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    //     }
    //     else
    //     {
    //         swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //     }

    //     swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    //     swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    //     swapchainCreateInfo.presentMode = presentMode;
    //     swapchainCreateInfo.clipped = VK_TRUE;
    //     swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    //     return vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
    // }

    // VkResult Engine::createImageViews(const VkFormat swapchainImageFormat)
    // {
    //     std::vector<VkImage> swapchainImages;
    //     uint32_t imageCount;
    //     vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    //     swapchainImages.resize(imageCount);
    //     vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    //     swapchainImageViews.resize(swapchainImages.size());

    //     for (size_t i = 0; i < swapchainImages.size(); i++)
    //     {
    //         VkImageViewCreateInfo imageViewCreateInfo{};
    //         imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    //         imageViewCreateInfo.image = swapchainImages[i];
    //         imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //         imageViewCreateInfo.format = swapchainImageFormat;
    //         imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    //         imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    //         imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    //         imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    //         imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //         imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    //         imageViewCreateInfo.subresourceRange.levelCount = 1;
    //         imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    //         imageViewCreateInfo.subresourceRange.layerCount = 1;

    //         VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);
    //         if (result != VK_SUCCESS)
    //         {
    //             return result;
    //         }
    //     }

    //     return VK_SUCCESS;
    // }

    // VkResult Engine::createShaderModule(VkShaderModule& shaderModule, const std::vector<char>& code)
    // {
    //     VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    //     shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    //     shaderModuleCreateInfo.codeSize = code.size();
    //     shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    //     return vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    // }

    // VkResult Engine::createFramebuffers()
    // {
    //     swapchainFramebuffers.resize(swapchainImageViews.size());

    //     for (size_t i = 0; i < swapchainImageViews.size(); i++)
    //     {
    //         VkImageView attachments[] = { swapchainImageViews[i] };

    //         VkFramebufferCreateInfo framebufferCreateInfo{};
    //         framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    //         framebufferCreateInfo.renderPass = renderPass;
    //         framebufferCreateInfo.attachmentCount = 1;
    //         framebufferCreateInfo.pAttachments = attachments;
    //         framebufferCreateInfo.width = swapchainExtent.width;
    //         framebufferCreateInfo.height = swapchainExtent.height;
    //         framebufferCreateInfo.layers = 1;

    //         VkResult result =  vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i]);
    //         if (result != VK_SUCCESS)
    //         {
    //             return result;
    //         }
    //     }
    //     return VK_SUCCESS;
    // }

    // VkResult Engine::createInstanceBuffers()
    // {
    //     VkDeviceSize instanceBufferSize = sizeof(InstanceData) * maxInstances;
    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         VkResult result = createBuffer(instanceBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instanceBuffers[i], instanceBuffersMemory[i]);
    //         if (result != VK_SUCCESS)
    //         {
    //             return result;
    //         }
    //         result = vkMapMemory(device, instanceBuffersMemory[i], 0, instanceBufferSize, 0, &instanceBuffersMapped[i]);
    //         if (result != VK_SUCCESS)
    //         {
    //             return result;
    //         }
    //     }
    //     return VK_SUCCESS;
    // }

    // VkResult Engine::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    // {
    //     VkBufferCreateInfo bufferCreateInfo{};
    //     bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    //     bufferCreateInfo.size = size;
    //     bufferCreateInfo.usage = usageFlags;
    //     bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    //     VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    //     if (result != VK_SUCCESS)
    //     {
    //         return result;
    //     }

    //     VkMemoryRequirements memoryRequirements;
    //     vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

    //     VkMemoryAllocateInfo memoryAllocateInfo{};
    //     memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    //     memoryAllocateInfo.allocationSize = memoryRequirements.size;

    //     VkPhysicalDeviceMemoryProperties memoryProperties;
    //     vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    //     std::optional<uint32_t> memoryTypeIndex;
    //     for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    //     {
    //         if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
    //         {
    //             memoryTypeIndex = i;
    //         }
    //     }

    //     if (!memoryTypeIndex.has_value())
    //     {
    //         return VK_ERROR_UNKNOWN;
    //     }

    //     memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex.value();

    //     result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &bufferMemory);
    //     if (result != VK_SUCCESS)
    //     {
    //         return result;
    //     }

    //     return vkBindBufferMemory(device, buffer, bufferMemory, 0);
    // }

    // VkResult Engine::copyBuffer(const VkBuffer srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size)
    // {
    //     VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    //     commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //     commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //     commandBufferAllocateInfo.commandPool = commandPool;
    //     commandBufferAllocateInfo.commandBufferCount = 1;

    //     VkCommandBuffer commandBuffer;
    //     VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
    //     if (result != VK_SUCCESS)
    //     {
    //         return result;
    //     }

    //     VkCommandBufferBeginInfo beginInfo{};
    //     beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //     beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    //     result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    //     if (result != VK_SUCCESS)
    //     {
    //         return result;
    //     }

    //         VkBufferCopy bufferCopy{};
    //         bufferCopy.size = size;
    //         vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

    //     result = vkEndCommandBuffer(commandBuffer);
    //     if (result != VK_SUCCESS)
    //     {
    //         return result;
    //     }

    //     VkSubmitInfo submitInfo{};
    //     submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //     submitInfo.commandBufferCount = 1;
    //     submitInfo.pCommandBuffers = &commandBuffer;

    //     result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    //     if (result != VK_SUCCESS)
    //     {
    //         return result;
    //     }
    //     result = vkQueueWaitIdle(graphicsQueue);
    //     if (result != VK_SUCCESS)
    //     {
    //         return result;
    //     }

    //     vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    //     return VK_SUCCESS;
    // }

    // VkResult Engine::recreateSwapchain()
    // {
    //     int width = 0, height = 0;
    //     glfwGetFramebufferSize(window, &width, &height);
    //     while (width == 0 || height == 0)
    //     {
    //         glfwWaitEvents();
    //         glfwGetFramebufferSize(window, &width, &height);
    //     }

    //     vkDeviceWaitIdle(device);
    //     cleanupSwapchain();

    //     VkResult result;
    //     VkFormat swapchainImageFormat;
    //     result = createSwapchain(swapchainImageFormat);
    //     if (result != VK_SUCCESS) { return result; }
    //     result = createImageViews(swapchainImageFormat);
    //     if (result != VK_SUCCESS) { return result; }
    //     result = createFramebuffers();
    //     if (result != VK_SUCCESS) { return result; }
    //     return VK_SUCCESS;
    // }

    // void Engine::cleanupSwapchain()
    // {
    //     for (auto framebuffer: swapchainFramebuffers)
    //     {
    //         vkDestroyFramebuffer(device, framebuffer, nullptr);
    //     }
    //     for (auto imageView : swapchainImageViews)
    //     {
    //         vkDestroyImageView(device, imageView, nullptr);
    //     }
    //     vkDestroySwapchainKHR(device, swapchain, nullptr);
    // }

    // void Engine::cleanupInstanceBuffers()
    // {
    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         vkFreeMemory(device, instanceBuffersMemory[i], nullptr);
    //         vkDestroyBuffer(device, instanceBuffers[i], nullptr);
    //     }
    // }
}