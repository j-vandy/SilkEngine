#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>

#include <vector>
#include <array>
#include <functional>
#include <iostream>
#include <format>

#include <tiny_gltf.h>

namespace silk
{
    constexpr const char* toString(VkResult result)
    {
        switch (result)
        {
            case VkResult::VK_SUCCESS: return "VK_SUCCESS";
            case VkResult::VK_NOT_READY: return "VK_NOT_READY";
            case VkResult::VK_TIMEOUT: return "VK_TIMEOUT";
            case VkResult::VK_EVENT_SET: return "VK_EVENT_SET";
            case VkResult::VK_EVENT_RESET: return "VK_EVENT_RESET";
            case VkResult::VK_INCOMPLETE: return "VK_INCOMPLETE";
            case VkResult::VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VkResult::VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VkResult::VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
            case VkResult::VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
            case VkResult::VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
            case VkResult::VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
            case VkResult::VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VkResult::VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VkResult::VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VkResult::VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
            case VkResult::VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VkResult::VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
            case VkResult::VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
            case VkResult::VK_ERROR_VALIDATION_FAILED: return "VK_ERROR_VALIDATION_FAILED";
            case VkResult::VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
            case VkResult::VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
            case VkResult::VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
            case VkResult::VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
            case VkResult::VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
            case VkResult::VK_ERROR_NOT_PERMITTED: return "VK_ERROR_NOT_PERMITTED";
            case VkResult::VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
            case VkResult::VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VkResult::VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
            case VkResult::VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
            case VkResult::VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
            case VkResult::VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
            case VkResult::VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
            case VkResult::VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
            case VkResult::VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
            case VkResult::VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
            case VkResult::VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
            case VkResult::VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
            case VkResult::VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
            case VkResult::VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT: return "VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT";
            case VkResult::VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
            case VkResult::VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
            case VkResult::VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
            case VkResult::VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
            case VkResult::VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
            case VkResult::VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
            case VkResult::VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
            case VkResult::VK_INCOMPATIBLE_SHADER_BINARY_EXT: return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
            case VkResult::VK_PIPELINE_BINARY_MISSING_KHR: return "VK_PIPELINE_BINARY_MISSING_KHR";
            case VkResult::VK_ERROR_NOT_ENOUGH_SPACE_KHR: return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
            case VkResult::VK_RESULT_MAX_ENUM: return "VK_RESULT_MAX_ENUM";
            default: return "UNKNOWN";
        }
    }

    #define VK_CHECK(x)                                                                                                                                                                                                      \
    do {                                                                                                                                                                                                                     \
        VkResult result = (x);                                                                                                                                                                                               \
        if (result != VK_SUCCESS)                                                                                                                                                                                            \
        {                                                                                                                                                                                                                    \
            throw std::runtime_error(std::format("\n\tError:      {} ({})\n\tExpression: {}\n\tFunction:   {}\n\tFile:       {}:{}\n", silk::toString(result), static_cast<int>(result), #x, __func__, __FILE__, __LINE__)); \
        }                                                                                                                                                                                                                    \
    } while (0)

    VkSurfaceFormatKHR getPhysicalDeviceSurfaceFormat(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface);

    VkFormat getDepthFormat(const VkPhysicalDevice physicalDevice);

    std::vector<char> readFile(const std::string& filename);

    VkResult createVkShaderModule(const VkDevice device, VkShaderModule& shaderModule, const std::vector<char>& code);

    tinygltf::Model loadGLTFModel(const std::string& filename);

    std::vector<glm::vec3> getGLTFModelPositions(const tinygltf::Model& model);

    std::vector<glm::vec3> getGLTFModelNormals(const tinygltf::Model& model);

    std::vector<glm::vec2> getGLTFModelTexCoords(const tinygltf::Model& model);

    std::vector<uint16_t> getGLTFModelIndices(const tinygltf::Model& model);

    VkResult allocateMemory(const VkPhysicalDevice physicalDevice, const VkDevice device, const VkMemoryRequirements& memoryRequirements, const VkMemoryPropertyFlags& propertyFlags, VkDeviceMemory& deviceMemory);

    VkResult createBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags& usageFlags, const VkMemoryPropertyFlags& propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkResult copyBuffer(const VkDevice device, const VkQueue graphicsQueue, const VkCommandPool commandPool, const VkBuffer srcBuffer, VkBuffer dstBuffer, const VkDeviceSize size);

    struct DeviceContextCreateInfo
    {
        const char* applicationName;
        bool enableValidationLayers = true;
        std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };

    // NOTE: does not need to be rebuilt at runtime
    class DeviceContext
    {
    public:
        DeviceContext(GLFWwindow* window, const DeviceContextCreateInfo& createInfo);
        ~DeviceContext();
        VkSurfaceKHR getSurface() const;
        VkPhysicalDevice getPhysicalDevice() const;
        VkDevice getDevice() const;
        VkQueue getGraphicsQueue() const;
        uint32_t getGraphicsQueueFamilyIndex() const;
        VkQueue getPresentQueue() const;
        uint32_t getPresentQueueFamilyIndex() const;
    private:
        bool enableValidationLayers;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;
        VkQueue graphicsQueue;
        uint32_t graphicsQueueFamilyIndex;
        VkQueue presentQueue;
        uint32_t presentQueueFamilyIndex;
    };

    struct ImageViewContextCreateInfo
    {
        VkImage image;
        VkFormat format;
        VkImageAspectFlags aspectMask;
    };

    class ImageViewContext
    {
    public:
        ImageViewContext(const VkDevice device, const ImageViewContextCreateInfo& imageViewCreateInfo);
        ~ImageViewContext();
        VkImageView getImageView() const;
    private:
        VkDevice device;
        VkImageView imageView;
    };
    
    class SwapchainContext
    {
    public:
        SwapchainContext(GLFWwindow* window, const DeviceContext& deviceContext, VkRenderPass renderPass);
        ~SwapchainContext();
        void recreate(GLFWwindow* window, const silk::DeviceContext& deviceContext, VkRenderPass renderPass);
        const VkExtent2D& getExtent() const;
        VkSwapchainKHR getSwapchain() const;
        const std::vector<VkFramebuffer>& getFramebuffers() const;
        size_t getSwapchainImageCount() const;
    private:
        VkDevice device;
        VkExtent2D extent;
        VkSwapchainKHR swapchain;
        std::vector<ImageViewContext> swapchainImageViews;
        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;
        std::vector<VkFramebuffer> framebuffers;
        void create(GLFWwindow* window, const DeviceContext& deviceContext, VkRenderPass renderPass);
        void destroy();
    };

    template <typename T>
    concept VertexInput = requires {
        { T::getBindingDescription() } -> std::same_as<VkVertexInputBindingDescription>;
        { T::getAttributeDescriptions() } -> std::same_as<std::vector<VkVertexInputAttributeDescription>>;
    };

    template <typename T>
    concept PushConstant = requires {
        { T::getStageFlags() } -> std::same_as<VkShaderStageFlags>;
    };

    class PipelineContextCreateInfo
    {
    public:
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkPushConstantRange> pushConstantRanges;
        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;

        template <typename VertexInputPack, typename PushConstantPack>
            requires(std::tuple_size_v<VertexInputPack> > 0)
        static PipelineContextCreateInfo build(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
        {
            PipelineContextCreateInfo createInfo{};
            createInfo.descriptorSetLayouts = descriptorSetLayouts;

            // push constant ranges
            [&createInfo]<std::size_t... PCIndices>(std::index_sequence<PCIndices...>)
            {
                (
                    createInfo.pushConstantRanges.push_back(
                        VkPushConstantRange
                        {
                            std::tuple_element_t<PCIndices, PushConstantPack>::getStageFlags(),
                            0,
                            static_cast<uint32_t>(sizeof(std::tuple_element_t<PCIndices, PushConstantPack>))
                        }
                    ),
                    ...
                );
            }(std::make_index_sequence<std::tuple_size_v<PushConstantPack>>{});

            // vertex input create info
            createInfo.vertexInputCreateInfo = VkPipelineVertexInputStateCreateInfo{};
            createInfo.vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            // Process vertex inputs
            [&createInfo]<std::size_t... VIIndices>(std::index_sequence<VIIndices...>) {
                (
                    [&] {
                        using VI = std::tuple_element_t<VIIndices, VertexInputPack>;
                        createInfo.vertexBindingDescriptions.push_back(VI::getBindingDescription());
                        auto attributes = VI::getAttributeDescriptions();
                        createInfo.vertexAttributeDescriptions.insert(createInfo.vertexAttributeDescriptions.end(), attributes.begin(), attributes.end());
                    }(),
                    ...
                );
            }(std::make_index_sequence<std::tuple_size_v<VertexInputPack>>{});

            if (createInfo.vertexBindingDescriptions.size() <= 0 || createInfo.vertexAttributeDescriptions.size() <= 0)
            {
                throw std::runtime_error("Error: failed to get VkVertexInputBindingDescriptions or VkVertexInputAttributeDescription!");
            }

            createInfo.vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfo.vertexBindingDescriptions.size());
            createInfo.vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(createInfo.vertexAttributeDescriptions.size());
            createInfo.vertexInputCreateInfo.pVertexBindingDescriptions = createInfo.vertexBindingDescriptions.data();
            createInfo.vertexInputCreateInfo.pVertexAttributeDescriptions = createInfo.vertexAttributeDescriptions.data();

            return createInfo;
        }
    private:
        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    };

    // NOTE: does not need to be rebuilt at runtime
    class PipelineContext
    {
    public:
        PipelineContext(VkDevice device, VkRenderPass renderPass, const PipelineContextCreateInfo& createInfo);
        ~PipelineContext();
        VkPipelineLayout getPipelineLayout() const;
        VkPipeline getPipeline() const;
    private:
        VkDevice device;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
    };

    // NOTE: does not need to be rebuilt at runtime
    template <typename T>
    class DeviceLocalBufferContext
    {
    public:
        DeviceLocalBufferContext(const DeviceContext& deviceContext, const VkCommandPool commandPool, const std::vector<T>& data, const VkBufferUsageFlags& usageFlags) : device(deviceContext.getDevice())
        {
            VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();
            VkDeviceSize bufferSize = sizeof(T) * data.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            VK_CHECK(createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory));

            void *stagingData;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &stagingData);
            memcpy(stagingData, data.data(), static_cast<size_t>(bufferSize));
            vkUnmapMemory(device, stagingBufferMemory);

            VK_CHECK(createBuffer(physicalDevice, device, bufferSize, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory));
            VK_CHECK(copyBuffer(device, deviceContext.getGraphicsQueue(), commandPool, stagingBuffer, buffer, bufferSize));

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
            std::cout << "Create DeviceLocalBufferContext\n";
        }
        ~DeviceLocalBufferContext()
        {
            vkDeviceWaitIdle(device);
            vkFreeMemory(device, bufferMemory, nullptr);
            vkDestroyBuffer(device, buffer, nullptr);
            std::cout << "Destroy DeviceLocalBufferContext\n";
        }
        VkBuffer getBuffer() const { return buffer; }
    private:
        VkDevice device;
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
    };

    // NOTE: does not need to be rebuilt at runtime
    template <typename T>
    class HostVisibleBufferContext
    {
    public:
        HostVisibleBufferContext(const DeviceContext& deviceContext) : device(deviceContext.getDevice())
        {
            VkDeviceSize bufferSize = sizeof(T);
            VK_CHECK(createBuffer(deviceContext.getPhysicalDevice(), device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, bufferMemory));
            VK_CHECK(vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &bufferMapped));
            std::cout << "Create HostVisibleBufferContext\n";
        }
        ~HostVisibleBufferContext()
        {
            vkDeviceWaitIdle(device);
            vkUnmapMemory(device, bufferMemory);
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, bufferMemory, nullptr);
            std::cout << "Destroy HostVisibleBufferContext\n";
        }
        VkDescriptorBufferInfo getVkDescriptorBufferInfos() const
        {
            VkDescriptorBufferInfo descriptorBufferInfo{};
            descriptorBufferInfo.buffer = buffer;
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(T);
            return descriptorBufferInfo;
        }
        void memcpy(const T* data) const { std::memcpy(bufferMapped, data, sizeof(T)); }
    private:
        VkDevice device;
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
        void* bufferMapped;
    };

    // NOTE: does not need to be rebuilt at runtime
    class DeviceLocalImageContext
    {
    public:
        DeviceLocalImageContext(const DeviceContext& deviceContext, const VkCommandPool commandPool, const tinygltf::Image& tinyImage);
        ~DeviceLocalImageContext();
        VkSampler getSampler() const;
        VkImageView getImageView() const;
    private:
        VkDevice device;
        VkImage image;
        VkDeviceMemory deviceMemory;
        std::optional<ImageViewContext> imageViewContext;
        VkSampler sampler;
    };

    // struct Camera
    // {
    //     float fovYAxis;
    //     glm::mat4 getOrthoMatrix(uint32_t screenWidth, uint32_t screenHeight) const;
    //     Camera(float fovy = 1.0f) : fovYAxis(fovy) {}
    // };

    // struct InstanceData
    // {
    //     glm::mat4 model;
    //     glm::vec4 tint;
    //     static VkVertexInputBindingDescription getBindingDescription(uint32_t binding = 0);
    //     static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions(uint32_t binding = 0, uint32_t location = 0);
    // };
}