#pragma once

// #include "ECS.h"

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
    constexpr std::string toString(VkResult result)
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

    std::vector<char> readFile(const std::string& filename);

    VkResult createVkShaderModule(const VkDevice device, VkShaderModule& shaderModule, const std::vector<char>& code);

    tinygltf::Model loadGLTFModel(const std::string& filename);

    std::vector<glm::vec3> getGLTFModelPositions(const tinygltf::Model& model);

    std::vector<glm::vec3> getGLTFModelNormals(const tinygltf::Model& model);

    std::vector<uint16_t> getGLTFModelIndices(const tinygltf::Model& model);

    VkResult createBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags& usageFlags, const VkMemoryPropertyFlags& propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    VkResult copyBuffer(const VkDevice device, const VkQueue graphicsQueue, const VkCommandPool commandPool, const VkBuffer srcBuffer, VkBuffer dstBuffer, const VkDeviceSize size);

    VkFormat getDepthFormat(const VkPhysicalDevice physicalDevice);

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
        std::vector<VkImageView> swapchainImageViews;
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

    // NOTE: does not need to be rebuilt at runtime
    class PipelineContext
    {
    public:
        template <VertexInput... Ts>
            requires (sizeof...(Ts) > 0)
        static PipelineContext create(VkDevice device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> descriptorSetLayouts)
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

            VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
            vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
            std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
            (
                [&vertexBindingDescriptions, &vertexAttributeDescriptions]() {
                    vertexBindingDescriptions.push_back(Ts::getBindingDescription());
                    auto attributeDescriptions = Ts::getAttributeDescriptions();
                    vertexAttributeDescriptions.insert(vertexAttributeDescriptions.end(), attributeDescriptions.begin(), attributeDescriptions.end());
                }(),
                ...
            );
            if (vertexBindingDescriptions.size() <= 0 || vertexAttributeDescriptions.size() <= 0)
            {
                throw std::runtime_error("Error: failed to get VkVertexInputBindingDescriptions or VkVertexInputAttributeDescription!");
            }

            vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
            vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
            vertexInputCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
            vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

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
            pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
            pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
            pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

            PipelineContext pipelineContext;
            pipelineContext.device = device;

            VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineContext.pipelineLayout));

            VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
            graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            graphicsPipelineCreateInfo.stageCount = 2;
            graphicsPipelineCreateInfo.pStages = shaderStages;
            graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
            graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
            graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
            graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
            graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
            graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
            graphicsPipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
            graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
            graphicsPipelineCreateInfo.layout = pipelineContext.pipelineLayout;
            graphicsPipelineCreateInfo.renderPass = renderPass;
            graphicsPipelineCreateInfo.subpass = 0;
            graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

            VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipelineContext.pipeline));

            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            vkDestroyShaderModule(device, fragShaderModule, nullptr);

            std::cout << "Create PipelineContext\n";
            return pipelineContext;
        }
        ~PipelineContext();
        VkPipelineLayout getPipelineLayout() const;
        VkPipeline getPipeline() const;
    private:
        PipelineContext() = default;
        VkDevice device;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
    };

    // struct Tint
    // {
    //     glm::vec4 color;
    // };

    // struct Quad{};

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