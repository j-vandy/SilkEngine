#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
// #include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <array>
#include <functional>

#include "ECS.h"

namespace silk
{

    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct Renderable{};

    struct Camera
    {
        float fovy;
        float near;
        float far;
    };

    struct Transform
    {
        glm::vec3 position;
        glm::vec4 rotation;
        glm::vec3 scale;
    };

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    };
   
    class Engine
    {
    public:
        std::vector<std::function<void(float)>> updateCallbacks;
        Engine(int width, int height, const char* applicationName);
        ~Engine();
        void loadScene(Scene& scene);
        void run();
        void shouldResizeFramebuffer();
        void memcpyUBO(const UniformBufferObject& ubo);
        const VkExtent2D& getSwapchainExtent();
    private:
        const bool ENABLE_VALIDATION_LAYERS = true;
        const int MAX_FRAMES_IN_FLIGHT = 2;
        const char* ENGINE_NAME = "Silk Engine";
        const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        bool framebufferResized = true;
        bool sceneLoaded = false;
        uint32_t currentFrame = 0;
        GLFWwindow* window;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        uint32_t graphicsQueueFamilyIndex, presentQueueFamilyIndex;
        VkSwapchainKHR swapchain;
        VkExtent2D swapchainExtent;
        std::vector<VkImageView> swapchainImageViews;
        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        std::vector<VkFramebuffer> swapchainFramebuffers;
        VkCommandPool commandPool;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        uint32_t indexCount;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        std::vector<void*> uniformBuffersMapped;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        VkResult createSwapchain(VkFormat& swapchainImageFormat);
        VkResult createImageViews(const VkFormat swapchainImageFormat);
        VkResult createShaderModule(VkShaderModule& shaderModule, const std::vector<char>& code);
        VkResult createFramebuffers();
        VkResult createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        VkResult copyBuffer(const VkBuffer srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size);
        VkResult recreateSwapchain();
        void cleanupSwapchain();
        void cleanupScene();
    };
}