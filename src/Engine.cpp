#include "silk/Engine.h"
#include "silk/Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>

namespace silk
{
    VkSurfaceFormatKHR getPhysicalDeviceSurfaceFormat(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
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

    VkFormat getDepthFormat(const VkPhysicalDevice physicalDevice)
    {
        const std::vector<VkFormat> depthFormatCandidates =
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        for (VkFormat format : depthFormatCandidates)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

            if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                return format;
            }
        }

        throw std::runtime_error("ERROR: failed to find depth format!");
    }

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

    VkResult createVkShaderModule(const VkDevice device, VkShaderModule& shaderModule, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = code.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        return vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    }

    tinygltf::Model loadGLTFModel(const std::string& filename)
    {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err;
        std::string warn;

        bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
        if (!warn.empty())
        {
            std::cerr << "Warning: " << warn << "\n";
        }

        if (!err.empty())
        {
            throw std::runtime_error("Error: failed to load glTF '" + filename + "'.\nError: " + err + "\n");
        }

        if (!res)
        {
            throw std::runtime_error("Error: failed to load glTF '" + filename + "'.\n");
        }
        else
        {
            std::cout << "Loaded glTF '" << filename << "'\n";
        }

        return model;
    }

    struct AccessorView
    {
        const unsigned char* data;
        const int stride;
        size_t count;
    };

    AccessorView getAccessorView(const tinygltf::Model& model, const std::string& accessorName)
    {
        const tinygltf::Mesh& mesh = model.meshes[0];
        const tinygltf::Primitive& primitive = mesh.primitives[0];

        const int accessorIndex = accessorName == "INDEX" ? primitive.indices : primitive.attributes.at(accessorName);
        const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        return {
            &buffer.data[bufferView.byteOffset + accessor.byteOffset],
            accessor.ByteStride(bufferView),
            accessor.count
        };
    }

    template <typename T>
    std::vector<T> readAccessorView(const AccessorView& accessorView)
    {
        std::vector<T> out(accessorView.count);

        for (size_t i = 0; i < accessorView.count; i++)
        {
            memcpy(&out[i], accessorView.data + i * accessorView.stride, sizeof(T));
        }

        return out;
    }

    std::vector<glm::vec3> getGLTFModelPositions(const tinygltf::Model& model) { return readAccessorView<glm::vec3>(getAccessorView(model, "POSITION")); }

    std::vector<glm::vec3> getGLTFModelNormals(const tinygltf::Model& model) { return readAccessorView<glm::vec3>(getAccessorView(model, "NORMAL")); }

    std::vector<glm::vec2> getGLTFModelTexCoords(const tinygltf::Model& model) { return readAccessorView<glm::vec2>(getAccessorView(model, "TEXCOORD_0")); }

    // TODO load image

    std::vector<uint16_t> getGLTFModelIndices(const tinygltf::Model& model)
    {
        AccessorView accessorView = getAccessorView(model, "INDEX");
        const uint16_t* data = reinterpret_cast<const uint16_t*>(accessorView.data);
        return std::vector<uint16_t>(data, data + accessorView.count);
    }

    VkResult allocateMemory(const VkPhysicalDevice physicalDevice, const VkDevice device, const VkMemoryRequirements &memoryRequirements, const VkMemoryPropertyFlags &propertyFlags, VkDeviceMemory &deviceMemory)
    {
        VkMemoryAllocateInfo memoryAllocateInfo{};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;

        // get memory type index
        uint32_t memoryTypeIndex = UINT32_MAX;
        {
            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

            for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
            {
                if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
                {
                    memoryTypeIndex = i;
                    break;
                }
            }

            if (memoryTypeIndex == UINT32_MAX)
            {
                throw std::runtime_error("ERROR: failed to find suitable memory type!");
            }
        }

        memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

        return vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory);
    }

    VkResult createBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags &usageFlags, const VkMemoryPropertyFlags &propertyFlags, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {
        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = usageFlags;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
        if (result != VK_SUCCESS)
        {
            return result;
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        result = allocateMemory(physicalDevice, device, memoryRequirements, propertyFlags, bufferMemory);
        if (result != VK_SUCCESS)
        {
            return result;
        }

        return vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    VkResult copyBuffer(const VkDevice device, const VkQueue graphicsQueue, const VkCommandPool commandPool, const VkBuffer srcBuffer, VkBuffer dstBuffer, const VkDeviceSize size)
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
        if (result != VK_SUCCESS)
        {
            return result;
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (result != VK_SUCCESS)
        {
            return result;
        }

            VkBufferCopy bufferCopy{};
            bufferCopy.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);

        result = vkEndCommandBuffer(commandBuffer);
        if (result != VK_SUCCESS)
        {
            return result;
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
        {
            return result;
        }
        result = vkQueueWaitIdle(graphicsQueue);
        if (result != VK_SUCCESS)
        {
            return result;
        }

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

        return VK_SUCCESS;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::cout << "\033[33m";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            std::cout << "\033[31m";
        }
        std::cout << "Validation Layer: " << pCallbackData->pMessage << "\033[39m" << std::endl;
        return VK_FALSE;
    }

    DeviceContext::DeviceContext(GLFWwindow* window, const DeviceContextCreateInfo& createInfo) : enableValidationLayers(createInfo.enableValidationLayers)
    {
        // create VkInstance
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
        {
            // check validation layer support
            if (createInfo.enableValidationLayers)
            {
                uint32_t layerCount;
                vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
                std::vector<VkLayerProperties> availableLayers(layerCount);
                vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

                std::set<std::string> requiredValidationLayers(createInfo.validationLayers.begin(), createInfo.validationLayers.end());
                for (const auto& layerProperties : availableLayers)
                {
                    requiredValidationLayers.erase(layerProperties.layerName);
                }

                if (!requiredValidationLayers.empty())
                {
                    throw std::runtime_error("Error: validation layers requested but not available!");
                }
            }

            VkApplicationInfo applicationInfo{};
            applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            applicationInfo.pApplicationName = createInfo.applicationName;
            applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            applicationInfo.pEngineName = "Silk Engine";
            applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            applicationInfo.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo instanceCreateInfo{};
            instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceCreateInfo.pApplicationInfo = &applicationInfo;

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
            if (createInfo.enableValidationLayers)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

            if (createInfo.enableValidationLayers)
            {
                instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(createInfo.validationLayers.size());
                instanceCreateInfo.ppEnabledLayerNames = createInfo.validationLayers.data();

                debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugMessengerCreateInfo.pfnUserCallback = debugCallback;

                instanceCreateInfo.pNext = &debugMessengerCreateInfo;
            }
            else
            {
                instanceCreateInfo.enabledLayerCount = 0;
                instanceCreateInfo.pNext = nullptr;
            }

            VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
        }

        // create VkDebugUtilsMessengerEXT
        if (createInfo.enableValidationLayers)
        {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                VK_CHECK(func(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger));
            }
        }

        // create VkSurfaceKHR
        VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

        // pick VkPhysicalDevice
        {
            uint32_t physicalDeviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
            if (physicalDeviceCount == 0)
            {
                throw std::runtime_error("Error: failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

            for (const auto& physDev : physicalDevices)
            {
                // queue family support
                std::optional<uint32_t> graphicsIndex, presentIndex;

                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);
                std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, queueFamilies.data());

                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    {
                        graphicsIndex = i;
                    }

                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR(physDev, i, surface, &presentSupport);
                    if (presentSupport)
                    {
                        presentIndex = i;
                    }

                    if (graphicsIndex.has_value() && presentIndex.has_value())
                    {
                        break;
                    }

                    i++;
                }

                // extension support
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, nullptr);
                std::vector<VkExtensionProperties> extensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, extensions.data());

                std::set<std::string> requiredExtensions(createInfo.deviceExtensions.begin(), createInfo.deviceExtensions.end());
                for (const auto& extension : extensions)
                {
                    requiredExtensions.erase(extension.extensionName);
                }

                // swapchain support
                uint32_t surfaceFormatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &surfaceFormatCount, nullptr);

                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, nullptr);

                if (graphicsIndex.has_value()
                    && presentIndex.has_value()
                    && requiredExtensions.empty()
                    && surfaceFormatCount != 0
                    && presentModeCount != 0)
                {
                    physicalDevice = physDev;
                    graphicsQueueFamilyIndex = graphicsIndex.value();
                    presentQueueFamilyIndex = presentIndex.value();
                    break;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE)
            {
                throw std::runtime_error("Error: failed to find a suitable GPU!");
            }
        }

        // create (logical) VkDevice
        {
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {
                graphicsQueueFamilyIndex,
                presentQueueFamilyIndex
            };

            float queuePriority = 1.0f;
            for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};

            VkDeviceCreateInfo deviceCreateInfo{};
            deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
            deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
            deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(createInfo.deviceExtensions.size());
            deviceCreateInfo.ppEnabledExtensionNames = createInfo.deviceExtensions.data();

            VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        }

        // create queues
        {
            vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
            vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
        }

        std::cout << "Create DeviceContext\n";
    }

    DeviceContext::~DeviceContext()
    {
        vkDeviceWaitIdle(device);

        // destroy VkDevice
        vkDestroyDevice(device, nullptr);

        // destroy VkSurfaceKHR
        vkDestroySurfaceKHR(instance, surface, nullptr);

        // destroy VkDebugUtilsMessengerEXT
        if (enableValidationLayers)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                func(instance, debugMessenger, nullptr);
            }
        }

        // destroy VkInstance
        vkDestroyInstance(instance, nullptr);

        std::cout << "Destroy DeviceContext\n";
    }

    VkSurfaceKHR DeviceContext::getSurface() const { return surface; }

    VkPhysicalDevice DeviceContext::getPhysicalDevice() const { return physicalDevice; }

    VkDevice DeviceContext::getDevice() const { return device; }

    VkQueue DeviceContext::getGraphicsQueue() const { return graphicsQueue; }

    uint32_t DeviceContext::getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }

    VkQueue DeviceContext::getPresentQueue() const { return presentQueue; }

    uint32_t DeviceContext::getPresentQueueFamilyIndex() const { return presentQueueFamilyIndex; }

    SwapchainContext::SwapchainContext(GLFWwindow* window, const DeviceContext& deviceContext, VkRenderPass renderPass) : device(deviceContext.getDevice()) { create(window, deviceContext, renderPass); }

    SwapchainContext::~SwapchainContext() { destroy(); }

    void SwapchainContext::recreate(GLFWwindow* window, const silk::DeviceContext& deviceContext, VkRenderPass renderPass)
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        destroy();
        create(window, deviceContext, renderPass);
    }

    const VkExtent2D& SwapchainContext::getExtent() const { return extent; }

    VkSwapchainKHR SwapchainContext::getSwapchain() const { return swapchain; }

    const std::vector<VkFramebuffer>& SwapchainContext::getFramebuffers() const { return framebuffers; }

    size_t SwapchainContext::getSwapchainImageCount() const { return swapchainImageViews.size(); }

    void SwapchainContext::create(GLFWwindow* window, const DeviceContext& deviceContext, VkRenderPass renderPass)
    {
        // create VkSwapchainKHR
        const VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();
        VkSurfaceFormatKHR surfaceFormat;
        {
            // surface format
            const VkSurfaceKHR& surface = deviceContext.getSurface();
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

            uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
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

            uint32_t queueFamilyIndices[] = { deviceContext.getGraphicsQueueFamilyIndex(), deviceContext.getPresentQueueFamilyIndex() };

            if (queueFamilyIndices[0] != queueFamilyIndices[1])
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

            VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain));
        }

        // create VkImageViews
        {
            std::vector<VkImage> swapchainImages;
            uint32_t imageCount;
            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
            swapchainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

            swapchainImageViews.resize(imageCount);
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

                VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]));
            }
        }

        // create depth image
        {
            VkFormat depthFormat = getDepthFormat(physicalDevice);

            // create depth VkImage
            VkImageCreateInfo depthImageCreateInfo{};
            depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            depthImageCreateInfo.pNext = nullptr;
            depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            depthImageCreateInfo.format = depthFormat;
            depthImageCreateInfo.extent.width = extent.width;
            depthImageCreateInfo.extent.height = extent.height;
            depthImageCreateInfo.extent.depth = 1;
            depthImageCreateInfo.mipLevels = 1;
            depthImageCreateInfo.arrayLayers = 1;
            depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            VK_CHECK(vkCreateImage(device, &depthImageCreateInfo, nullptr, &depthImage));

            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements(device, depthImage, &memoryRequirements);

            VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VK_CHECK(allocateMemory(physicalDevice, device, memoryRequirements, propertyFlags, depthImageMemory));

            VK_CHECK(vkBindImageMemory(device, depthImage, depthImageMemory, 0));

            // image views
            VkImageViewCreateInfo depthImageViewCreateInfo{};
            depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthImageViewCreateInfo.pNext = nullptr;
            depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthImageViewCreateInfo.image = depthImage;
            depthImageViewCreateInfo.format = depthFormat;
            depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            depthImageViewCreateInfo.subresourceRange.levelCount = 1;
            depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            depthImageViewCreateInfo.subresourceRange.layerCount = 1;
            depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            VK_CHECK(vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImageView));
        }

        // create VkFramebuffers
        {
            framebuffers.resize(swapchainImageViews.size());

            for (size_t i = 0; i < swapchainImageViews.size(); i++)
            {
                std::vector<VkImageView> attachments{ swapchainImageViews[i], depthImageView };

                VkFramebufferCreateInfo framebufferCreateInfo{};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = renderPass;
                framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferCreateInfo.pAttachments = attachments.data();
                framebufferCreateInfo.width = extent.width;
                framebufferCreateInfo.height = extent.height;
                framebufferCreateInfo.layers = 1;

                VK_CHECK(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[i]));
            }
        }

        std::cout << "Create SwapchainContext\n";
    }

    void SwapchainContext::destroy()
    {
        vkDeviceWaitIdle(device);

        // destroy VkFramebuffers
        for (auto framebuffer: framebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        // destroy depth image
        vkDestroyImageView(device, depthImageView, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
        vkDestroyImage(device, depthImage, nullptr);

        // destroy VkImageViews
        for (auto imageView : swapchainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        // destroy VkSwapchainKHR
        vkDestroySwapchainKHR(device, swapchain, nullptr);

        std::cout << "Destroy SwapchainContext\n";
    }

    PipelineContext::PipelineContext(VkDevice device, VkRenderPass renderPass, const PipelineContextCreateInfo& createInfo) : device(device)
    {
        // relative to binary dir
        std::vector<char> vertShaderCode = readFile("shaders/shader.vert.spv");
        std::vector<char> fragShaderCode = readFile("shaders/shader.frag.spv");

        VkShaderModule vertShaderModule;
        VK_CHECK(createVkShaderModule(device, vertShaderModule, vertShaderCode));

        VkShaderModule fragShaderModule;
        VK_CHECK(createVkShaderModule(device, fragShaderModule, fragShaderCode));

        VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
        vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageCreateInfo.module = vertShaderModule;
        vertShaderStageCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
        fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageCreateInfo.module = fragShaderModule;
        fragShaderStageCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportCreateInfo{};
        viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportCreateInfo.viewportCount = 1;
        viewportCreateInfo.scissorCount = 1;

        // TODO - Cull mode support: rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT OR VK_CULL_MODE_NONE

        VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
        rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationCreateInfo.lineWidth = 1.0f;
        rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationCreateInfo.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
        multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
        multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
        colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendCreateInfo.attachmentCount = 1;
        colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendCreateInfo.blendConstants[0] = 0.0f;
        colorBlendCreateInfo.blendConstants[1] = 0.0f;
        colorBlendCreateInfo.blendConstants[2] = 0.0f;
        colorBlendCreateInfo.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        bool depthTest = true, depthWrite = true;
        VkCompareOp compareOp = VK_COMPARE_OP_LESS;
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.pNext = nullptr;
        depthStencilStateCreateInfo.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
        depthStencilStateCreateInfo.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = createInfo.descriptorSetLayouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = createInfo.descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = createInfo.pushConstantRanges.size();
        pipelineLayoutCreateInfo.pPushConstantRanges = createInfo.pushConstantRanges.data();

        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.stageCount = 2;
        graphicsPipelineCreateInfo.pStages = shaderStages;
        graphicsPipelineCreateInfo.pVertexInputState = &createInfo.vertexInputCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
        graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        graphicsPipelineCreateInfo.layout = pipelineLayout;
        graphicsPipelineCreateInfo.renderPass = renderPass;
        graphicsPipelineCreateInfo.subpass = 0;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

        VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline));

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);

        std::cout << "Create PipelineContext\n";
    }
   
    PipelineContext::~PipelineContext()
    {
        vkDeviceWaitIdle(device);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        std::cout << "Destroy PipelineContext\n";
    }

    VkPipelineLayout PipelineContext::getPipelineLayout() const { return pipelineLayout; }

    VkPipeline PipelineContext::getPipeline() const { return pipeline; }

    DeviceLocalImageContext::DeviceLocalImageContext(const DeviceContext& deviceContext, const VkCommandPool commandPool, const tinygltf::Image& tinyImage) : device(deviceContext.getDevice())
    {
        const VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();

        // === create VkImage ===
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = tinyImage.width;
        imageCreateInfo.extent.height = tinyImage.height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device, image, &memoryRequirements);

        VkMemoryPropertyFlagBits propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VK_CHECK(allocateMemory(physicalDevice, device, memoryRequirements, propertyFlags, deviceMemory));

        VK_CHECK(vkBindImageMemory(device, image, deviceMemory, 0));

        // create staging buffer
        VkDeviceSize imageSize = tinyImage.bits / 8 * tinyImage.component * tinyImage.width * tinyImage.height;
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VK_CHECK(createBuffer(physicalDevice, device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory));

        void *stagingData;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &stagingData);
        memcpy(stagingData, tinyImage.image.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        // record command buffer
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer));

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

            // transition: UNDEFINED -> TRANSFER_DST_OPTIMALA
            {
                VkImageMemoryBarrier imageMemoryBarrier{};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.srcAccessMask = 0;
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.image = image;
                imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
                imageMemoryBarrier.subresourceRange.levelCount = 1;
                imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
                imageMemoryBarrier.subresourceRange.layerCount = 1;

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }
           
            // copy buffer to image
            VkBufferImageCopy bufferImageCopy{};
            bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferImageCopy.imageSubresource.layerCount = 1;
            bufferImageCopy.imageExtent = VkExtent3D {
                static_cast<uint32_t>(tinyImage.width),
                static_cast<uint32_t>(tinyImage.height),
                1
            };

            vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);

            // transition: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
            {
                VkImageMemoryBarrier imageMemoryBarrier{};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageMemoryBarrier.image = image;
                imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
                imageMemoryBarrier.subresourceRange.levelCount = 1;
                imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
                imageMemoryBarrier.subresourceRange.layerCount = 1;

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
            }

        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkQueue graphicsQueue = deviceContext.getGraphicsQueue();
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(graphicsQueue));

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

        // === create VkImageView ===
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView));

        // === create VkSampler ===
        VkSamplerCreateInfo samplerCreateInfo{};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.maxAnisotropy = 1.0f;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler));
    }

    DeviceLocalImageContext::~DeviceLocalImageContext()
    {
        vkDeviceWaitIdle(device);
        vkDestroySampler(device, sampler, nullptr);
        vkDestroyImageView(device, imageView, nullptr);
        vkFreeMemory(device, deviceMemory, nullptr);
        vkDestroyImage(device, image, nullptr);
        std::cout << "Destroy DeviceLocalImageContext\n";
    }

    VkSampler DeviceLocalImageContext::getSampler() const { return sampler; }

    VkImageView DeviceLocalImageContext::getImageView() const { return imageView; }

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
}