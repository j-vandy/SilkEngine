#include "Engine.h"
// #include "Transform.h"
// #include <glm/gtc/matrix_transform.hpp>

// const float FOV_Y = 2.0f;
// const float CAMERA_SPEED = 1.0f;

// silk::Engine engine(WIDTH, HEIGHT, APPLICATION_NAME);
// silk::Scene scene;
// silk::Entity cam = scene.createEntity(silk::Camera{FOV_Y}, silk::Transform{});
// glm::vec2 dirInput(0.0f);

// const glm::vec4 BLACK(0.0f);
// const glm::vec4 RED(1.0f, 0.0f, 0.0f, 1.0f);
// const glm::vec4 GREEN(0.0f, 1.0f, 0.0f, 1.0f);
// const glm::vec4 BLUE(0.0f, 0.0f, 1.0f, 1.0f);
// const glm::vec4 WHITE(1.0f, 1.0f, 1.0f, 1.0f);

// glm::vec4 tint = WHITE;
// void keyCallback(GLFWwindow* window __attribute__((unused)), int key, int scancode __attribute__((unused)), int action, int mods __attribute__((unused)))
// {
//     if (action == GLFW_REPEAT)
//     {
//         return;
//     }

//     // reverse input if action is released
//     float isPress = action == GLFW_PRESS ? 1.0f : -1.0f;

//     switch (key)
//     {
//         case GLFW_KEY_W:
//             dirInput.y += 1 * isPress;
//             break;
//         case GLFW_KEY_A:
//             dirInput.x -= 1 * isPress;
//             break;
//         case GLFW_KEY_S:
//             dirInput.y -= 1 * isPress;
//             break;
//         case GLFW_KEY_D:
//             dirInput.x += 1 * isPress;
//             break;
//         case GLFW_KEY_0:
//             tint = BLACK;
//             break;
//         case GLFW_KEY_1:
//             tint = RED;
//             break;
//         case GLFW_KEY_2:
//             tint = GREEN;
//             break;
//         case GLFW_KEY_3:
//             tint = BLUE;
//             break;
//         case GLFW_KEY_4:
//             tint = WHITE;
//             break;
//     }
// }

// float mouseButton1IsDown = false;
// float press_x, press_y;
// silk::Entity currQuad;
// void mouseButtonCallback(GLFWwindow* window __attribute__((unused)), int button, int action, int mods __attribute__((unused)))
// {
//     if (button == GLFW_MOUSE_BUTTON_1)
//     {
//         if (action == GLFW_PRESS)
//         {
//             mouseButton1IsDown = true;
//             engine.getCursorWorldSpace(scene, cam, &press_x, &press_y);
//             currQuad = scene.createEntity(silk::Quad{}, silk::Transform{}, silk::Tint{tint});
//         }
//         else if (action == GLFW_RELEASE)
//         {
//             mouseButton1IsDown = false;
//         }
//     }
// }

// void onUpdate(float deltaTime)
// {
//     // update quad position
//     if (mouseButton1IsDown)
//     {
//         silk::Transform& quadTransform = scene.getComponent<silk::Transform>(currQuad);

//         float x,y;
//         engine.getCursorWorldSpace(scene, cam, &x, &y);

//         glm::vec2 pressToCurr = glm::vec2((x - press_x), (y - press_y));
//         quadTransform.setScale(pressToCurr);
//         quadTransform.setPosition((x + press_x)/2.0f, (y + press_y)/2.0f);
//     }

//     // update camera pos + ubo
//     silk::Transform& camTransform = scene.getComponent<silk::Transform>(cam);
//     glm::vec2 dist(0.0f);
//     if (glm::length(dirInput) != 0.0f)
//     {
//         dist = glm::normalize(dirInput) * CAMERA_SPEED * deltaTime;
//     }
//     camTransform.setPosition(camTransform.getPosition() + dist);

//     silk::CameraUBO ubo{};
//     ubo.view = glm::inverse(camTransform.getMatrix());

//     const VkExtent2D& swapchainExtent = engine.getSwapchainExtent();
//     ubo.proj = scene.getComponent<silk::Camera>(cam).getOrthoMatrix(swapchainExtent.width, swapchainExtent.height);
//     ubo.proj[1][1] *= -1;

//     engine.memcpyCameraUBO(ubo);

//     // load instance info
//     std::vector<silk::InstanceData> instanceDatas;
//     for(silk::Entity e : scene.query<silk::Quad, silk::Transform>())
//     {
//         silk::InstanceData i;
//         i.model = scene.getComponent<silk::Transform>(e).getMatrix();
//         i.tint = scene.getComponent<silk::Tint>(e).color;
//         instanceDatas.push_back(i);
//     }
//     engine.updateInstanceBuffer(instanceDatas);
// }

#include <iostream>
#include <fstream>
#include <format>
#include <set>
#include <optional>
#include <algorithm>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType __attribute__((unused)), const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData __attribute__((unused)))
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

static std::vector<char> readFile(const std::string& filename)
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

VkResult createBuffer(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
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

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    std::optional<uint32_t> memoryTypeIndex;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
        {
            memoryTypeIndex = i;
        }
    }

    if (!memoryTypeIndex.has_value())
    {
        return VK_ERROR_UNKNOWN;
    }

    memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex.value();

    result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &bufferMemory);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    return vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkResult copyBuffer(const VkDevice& device, const VkQueue& graphicsQueue, const VkCommandPool& commandPool, const VkBuffer srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size)
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

bool framebufferResized = false;
void framebufferResizeCallback(GLFWwindow* window __attribute__((unused)), int width __attribute__((unused)), int height __attribute__((unused)))
{
    framebufferResized = true;
}

void recreateSwapchainContext(GLFWwindow* window, const VkPhysicalDevice& physicalDevice, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex, const VkSurfaceKHR& surface, const VkDevice& device, const VkRenderPass& renderPass, std::unique_ptr<silk::SwapchainContext>& swapchainContext)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    swapchainContext.reset();
    swapchainContext = std::make_unique<silk::SwapchainContext>(window, physicalDevice, graphicsQueueFamilyIndex, presentQueueFamilyIndex, surface, device, renderPass);
}

// TODO
// Only create a function or possibly a data structure whenever it makes sense (i.e., code duplication or reconstruction)
// COMMON CASES OF RECONSTRUCTION:
// - Creating different kinds of buffers
//      - Setting buffer/shader uniform values
// - Stanford Rabbit Viewer
// - 2D paint (ImGui support)
// - Check for depricated code
// - flatland RC
// - 3D/screen space RC

int main()
{
    // create glfw window
    const uint32_t WIDTH = 960;
    const uint32_t HEIGHT = 960;
    const char* APPLICATION_NAME = "Demo App";
    GLFWwindow* window;
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, APPLICATION_NAME, nullptr, nullptr);
        // glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    // create VkInstance
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
    const bool ENABLE_VALIDATION_LAYERS = true;
    const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
    VkInstance instance;
    {
        // check validation layer support
        if (ENABLE_VALIDATION_LAYERS)
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            std::set<std::string> requiredValidationLayers(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
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
        applicationInfo.pApplicationName = APPLICATION_NAME;
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
        if (ENABLE_VALIDATION_LAYERS)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        if (ENABLE_VALIDATION_LAYERS)
        {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

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

        silk::validateVkResult(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Error: failed to create VkInstance!");
    }

    // create VkDebugUtilsMessengerEXT
    VkDebugUtilsMessengerEXT debugMessenger;
    if (ENABLE_VALIDATION_LAYERS)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            silk::validateVkResult(func(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger), "Error: failed to create VkDebugUtilsMessengerEXT!");
        }
    }

    // create VkSurfaceKHR
    VkSurfaceKHR surface;
    silk::validateVkResult(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Error: failed to create VkSurfaceKHR!");

    // pick VkPhysicalDevice
    const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
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

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
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

            std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
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
    VkDevice device;
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
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
        deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

        if (ENABLE_VALIDATION_LAYERS)
        {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }
        else
        {
            deviceCreateInfo.enabledLayerCount = 0;
        }

        silk::validateVkResult(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device), "Error: failed to create VkDevice!");
    }

    // create queues
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    {
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
    }

    // create VkRenderPass
    VkSurfaceFormatKHR surfaceFormat = silk::getPhysicalDeviceSurfaceFormat(physicalDevice, surface);
    VkRenderPass renderPass;
    {
        VkAttachmentDescription colorAttachmentDescription{};
        colorAttachmentDescription.format = surfaceFormat.format;
        colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentReference;

        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;

        silk::validateVkResult(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass), "Error: failed to create VkRenderPass!");
    }

    // create SwapchainContext
    std::unique_ptr<silk::SwapchainContext> swapchainContext = std::make_unique<silk::SwapchainContext>(window, physicalDevice, graphicsQueueFamilyIndex, presentQueueFamilyIndex, surface, device, renderPass);

    // create VkDescriptorSetLayout
    VkDescriptorSetLayout descriptorSetLayout;
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = 1;
        descriptorSetLayoutCreateInfo.pBindings = &uboLayoutBinding;

        silk::validateVkResult(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout), "Error: failed to create VkDescriptorSetLayout!");
    }

    // TODO
    // - make it easy to change VkVertexInputBindingDescription (input rate & stride)
    // - Cull mode support: rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    // create (graphics) VkPipeline
    {
        // relative to binary dir
        std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
        std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule;
        silk::validateVkResult(createVkShaderModule(device, vertShaderModule, vertShaderCode), "Error: failed to create fragment VkShaderModule!");

        VkShaderModule fragShaderModule;
        silk::validateVkResult(createVkShaderModule(device, fragShaderModule, fragShaderCode), "Error: failed to create fragment VkShaderModule!");

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

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
        vertexBindingDescriptions.push_back(silk::Vertex::getBindingDescription());

        uint32_t instanceBinding = 1;
        uint32_t instanceLocation = 1;
        vertexBindingDescriptions.push_back(silk::InstanceData::getBindingDescription(instanceBinding));

        std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
        for (auto vertexAttributeDescription : silk::Vertex::getAttributeDescriptions())
        {
            vertexAttributeDescriptions.push_back(vertexAttributeDescription);
        }
        for (auto instanceAttributeDescription : silk::InstanceData::getAttributeDescriptions(instanceBinding, instanceLocation))
        {
            vertexAttributeDescriptions.push_back(instanceAttributeDescription);
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

        VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo{};
        rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationCreateInfo.lineWidth = 1.0f;
        rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE;
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
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

        silk::validateVkResult(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout), "Error: failed to create VkPipelineLayout!");

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.stageCount = 2;
        graphicsPipelineCreateInfo.pStages = shaderStages;
        graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &viewportCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        graphicsPipelineCreateInfo.layout = pipelineLayout;
        graphicsPipelineCreateInfo.renderPass = renderPass;
        graphicsPipelineCreateInfo.subpass = 0;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

        silk::validateVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline), "Error: failed to create VkPipeline!");

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
    }

    // create VkCommandPool
    VkCommandPool commandPool;
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

        silk::validateVkResult(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool),"Error: failed to create VkCommandPool!");
    }

    // TODO
    // - make it easy to create different types of buffers
    // create (vertex) VkBuffer
    const std::vector<silk::Vertex> QUAD_VERTICES = {
        {{-0.5f, -0.5f}},
        {{0.5f, -0.5f}},
        {{0.5f, 0.5f}},
        {{-0.5f, 0.5f}}
    };
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    {
        VkDeviceSize vertexBufferSize = sizeof(QUAD_VERTICES[0]) * QUAD_VERTICES.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        silk::validateVkResult(createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory), "Error: failed to create vertex staging buffer!");

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
            memcpy(data, QUAD_VERTICES.data(), static_cast<size_t>(vertexBufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        silk::validateVkResult(createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory), "Error: failed to create vertex buffer!");

        silk::validateVkResult(copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, vertexBuffer, vertexBufferSize), "Error: failed to copy buffer!");

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    // create (instance) VkBuffer
    uint32_t maxInstances = 100;
    const int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<uint32_t> instanceCounts;
    std::vector<VkBuffer> instanceBuffers;
    std::vector<VkDeviceMemory> instanceBuffersMemory;
    std::vector<void*> instanceBuffersMapped;
    {
        instanceCounts.resize(MAX_FRAMES_IN_FLIGHT);
        instanceBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        instanceBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        instanceBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        VkDeviceSize instanceBufferSize = sizeof(silk::InstanceData) * maxInstances;
        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            silk::validateVkResult(createBuffer(physicalDevice, device, instanceBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instanceBuffers[i], instanceBuffersMemory[i]), "Error: failed to create buffer!");
            silk::validateVkResult(vkMapMemory(device, instanceBuffersMemory[i], 0, instanceBufferSize, 0, &instanceBuffersMapped[i]), "Error: failed to map memory!");
        }
    }

    // create (index) VkBuffer
    const std::vector<uint16_t> QUAD_INDICES = {
        0, 1, 2, 2, 3, 0
    };
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    {
        VkDeviceSize indexBufferSize = sizeof(QUAD_INDICES[0]) * QUAD_INDICES.size(); 

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        silk::validateVkResult(createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory), "Error: failed to create index staging buffer!");

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
            memcpy(data, QUAD_INDICES.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        silk::validateVkResult(createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory), "Error: failed to create index buffer!");
        silk::validateVkResult(copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, indexBuffer, indexBufferSize), "Error: failed to copy buffer!");

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    // create (uniform) VkBuffer
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    {
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        VkDeviceSize bufferSize = sizeof(silk::CameraUBO);
        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            silk::validateVkResult(createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]), "Error: failed to create uniform buffers!");
            silk::validateVkResult(vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]), "Error: failed to map uniform buffers memory!");
        }
    }

    // create VkDescriptorPool
    VkDescriptorPool descriptorPool;
    {
        VkDescriptorPoolSize descriptorPoolSize{};
        descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.poolSizeCount = 1;
        descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
        descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        silk::validateVkResult(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool), "Error: failed to create VkDescriptorPool!");
    }

    // TODO
    // - how do we link descriptor sets with buffers
    // create VkDescriptorSets
    std::vector<VkDescriptorSet> descriptorSets;
    {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        silk::validateVkResult(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets.data()), "Error: failed to create VkDescriptorSets!");

        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            VkDescriptorBufferInfo descriptorBufferInfo{};
            descriptorBufferInfo.buffer = uniformBuffers[i];
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(silk::CameraUBO);

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = descriptorSets[i];
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

            vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
        }
    }

    // allocate VkCommandBuffer
    std::vector<VkCommandBuffer> commandBuffers;
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        silk::validateVkResult(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data()), "Error: failed to allocate VkCommandBuffer!");
    }

    // create synchronization objects
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            
            silk::validateVkResult(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]), "Error: failed to create VkSemaphore!");
            silk::validateVkResult(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]), "Error: failed to create VkSemaphore!");
            silk::validateVkResult(vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]), "Error: failed to create VkFence!");
        }
    }

    // run
    uint32_t indexCount = QUAD_INDICES.size();
    {
        // auto previousTime = std::chrono::high_resolution_clock::now();
        uint32_t currentFrame = 0;
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // auto currentTime = std::chrono::high_resolution_clock::now();
            // float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
            // previousTime = currentTime;
            // for (std::function<void(float)> fn : updateCallbacks)
            // {
            //     fn(deltaTime);
            // }

            // draw frame
            {
                vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;
                VkResult result = vkAcquireNextImageKHR(device, swapchainContext->getSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    recreateSwapchainContext(window, physicalDevice, graphicsQueueFamilyIndex, presentQueueFamilyIndex, surface, device, renderPass, swapchainContext);
                    continue;
                }
                else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                {
                    throw std::runtime_error("Error: failed to aquire next swapchain image!");
                }

                vkResetFences(device, 1, &inFlightFences[currentFrame]);

                vkResetCommandBuffer(commandBuffers[currentFrame], 0);
                
                // record command buffer
                VkCommandBufferBeginInfo commandBufferBeginInfo{};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                silk::validateVkResult(vkBeginCommandBuffer(commandBuffers[currentFrame], &commandBufferBeginInfo), "Error: failed to begin command buffer!");

                VkRenderPassBeginInfo renderPassBeginInfo{};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = renderPass;
                renderPassBeginInfo.framebuffer = swapchainContext->getFramebuffers()[imageIndex];
                renderPassBeginInfo.renderArea.offset = {0, 0};
                renderPassBeginInfo.renderArea.extent = swapchainContext->getExtent();

                VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassBeginInfo.clearValueCount = 1;
                renderPassBeginInfo.pClearValues = &clearColor;

                vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                    VkViewport viewport{};
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = static_cast<float>(swapchainContext->getExtent().width);
                    viewport.height = static_cast<float>(swapchainContext->getExtent().height);
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

                    VkRect2D scissor{};
                    scissor.offset = {0, 0};
                    scissor.extent = swapchainContext->getExtent();
                    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

                    VkBuffer vertexBuffers[] = { vertexBuffer, instanceBuffers[currentFrame] };
                    VkDeviceSize offsets[] = { 0, 0 };
                    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 2, vertexBuffers, offsets);

                    vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

                    vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

                    vkCmdDrawIndexed(commandBuffers[currentFrame], indexCount, instanceCounts[currentFrame], 0, 0, 0);

                vkCmdEndRenderPass(commandBuffers[currentFrame]);

                silk::validateVkResult(vkEndCommandBuffer(commandBuffers[currentFrame]), "Error: failed to begin command buffer!");

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

                VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                silk::validateVkResult(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]), "Error: failed to submit queue!");

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;

                VkSwapchainKHR swapchains[] = { swapchainContext->getSwapchain() };
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapchains;
                presentInfo.pImageIndices = &imageIndex;

                result = vkQueuePresentKHR(presentQueue, &presentInfo);
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
                {
                    framebufferResized = false;
                    recreateSwapchainContext(window, physicalDevice, graphicsQueueFamilyIndex, presentQueueFamilyIndex, surface, device, renderPass, swapchainContext);
                }
                else if (result != VK_SUCCESS)
                {
                    throw std::runtime_error("Error: failed to present swapchain image!");
                }

                currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        }
    }

    vkDeviceWaitIdle(device);

    // destroy synchronization objects
    for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    {
        vkDestroyFence(device, inFlightFences[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    }

    // destroy VkDescriptorPool
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    // destroy (uniform) VkBuffer
    for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    {
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    }

    // destroy (index) VkBuffer
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);

    // destroy (instance) VkBuffer
    for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    {
        vkFreeMemory(device, instanceBuffersMemory[i], nullptr);
        vkDestroyBuffer(device, instanceBuffers[i], nullptr);
    }

    // destroy (vertex) VkBuffer
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);

    // destroy VkCommandPool
    vkDestroyCommandPool(device, commandPool, nullptr);

    // destroy VkPipeline
    vkDestroyPipeline(device, pipeline, nullptr);

    // destroy VkPipelineLayout
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    // destroy VkDescriptorSetLayout
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    // destroy SwapchainContext
    // TODO
    swapchainContext.reset();

    // destroy renderPass
    vkDestroyRenderPass(device, renderPass, nullptr);

    // destroy VkDevice
    vkDestroyDevice(device, nullptr);

    // destroy VkSurfaceKHR
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // destroy VkDebugUtilsMessengerEXT
    if (ENABLE_VALIDATION_LAYERS)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, nullptr);
        }
    }

    // destroy VkInstance
    vkDestroyInstance(instance, nullptr);

    // destroy glfw window
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}