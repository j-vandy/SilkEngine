#include "Engine.h"

#include <iostream>
#include <fstream>
#include <format>
#include <set>
#include <optional>
#include <algorithm>

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

// TODO
// Only create a function or possibly a data structure whenever it makes sense (i.e., code duplication or reconstruction)
// COMMON CASES OF RECONSTRUCTION:
// - Cleanup the deviceContext.getDevice()
// - load model
// - Stanford Rabbit Viewer
// - Check for depricated code
// - Creating different kinds of buffers
//      - Setting buffer/shader uniform values
// - 2D paint (ImGui support)
// - flatland RC
// - 3D/screen space RC
// - Phox Engine

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
        // TODO
        // glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    silk::DeviceContextCreateInfo deviceContextCreateInfo{};
    deviceContextCreateInfo.applicationName = APPLICATION_NAME;

    silk::DeviceContext deviceContext(window, deviceContextCreateInfo);

    // create VkRenderPass
    VkSurfaceFormatKHR surfaceFormat = silk::getPhysicalDeviceSurfaceFormat(deviceContext.getPhysicalDevice(), deviceContext.getSurface());
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

        silk::validateVkResult(vkCreateRenderPass(deviceContext.getDevice(), &renderPassCreateInfo, nullptr, &renderPass), "Error: failed to create VkRenderPass!");
    }

    // create SwapchainContext
    silk::SwapchainContext swapchainContext(window, deviceContext, renderPass);

    // needed for any kind of uniform data (textures, buffers, etc.)
    // create VkDescriptorSetLayout
    // VkDescriptorSetLayout descriptorSetLayout;
    // {
    //     VkDescriptorSetLayoutBinding uboLayoutBinding{};
    //     uboLayoutBinding.binding = 0;
    //     uboLayoutBinding.descriptorCount = 1;
    //     uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //     uboLayoutBinding.pImmutableSamplers = nullptr;
    //     uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //     VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    //     descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //     descriptorSetLayoutCreateInfo.bindingCount = 1;
    //     descriptorSetLayoutCreateInfo.pBindings = &uboLayoutBinding;

    //     silk::validateVkResult(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout), "Error: failed to create VkDescriptorSetLayout!");
    // }

    struct Vertex
    {
        glm::vec3 position;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
        {
            return {
                { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position) }
            };
        }
    };

    silk::PipelineContext pipelineContext = silk::PipelineContext::create<Vertex>(deviceContext.getDevice(), renderPass);

    // create VkCommandPool
    VkCommandPool commandPool;
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = deviceContext.getGraphicsQueueFamilyIndex();

        silk::validateVkResult(vkCreateCommandPool(deviceContext.getDevice(), &commandPoolCreateInfo, nullptr, &commandPool),"Error: failed to create VkCommandPool!");
    }

    const std::vector<Vertex> TRI_VERTICES = {
        {{-0.5f, 0.5f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}},
        {{0.0f, -0.5f, 0.0f}}
    };

    // TODO
    // - make it easy to create different types of buffers
    // create (vertex) VkBuffer
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    {
        VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();
        VkDevice device = deviceContext.getDevice();
        VkDeviceSize vertexBufferSize = sizeof(TRI_VERTICES[0]) * TRI_VERTICES.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        silk::validateVkResult(createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory), "Error: failed to create vertex staging buffer!");

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, vertexBufferSize, 0, &data);
            memcpy(data, TRI_VERTICES.data(), static_cast<size_t>(vertexBufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        silk::validateVkResult(createBuffer(physicalDevice, device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory), "Error: failed to create vertex buffer!");

        silk::validateVkResult(copyBuffer(device, deviceContext.getGraphicsQueue(), commandPool, stagingBuffer, vertexBuffer, vertexBufferSize), "Error: failed to copy buffer!");

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    // create (instance) VkBuffer
    const int MAX_FRAMES_IN_FLIGHT = 2;
    // uint32_t maxInstances = 100;
    // std::vector<uint32_t> instanceCounts;
    // std::vector<VkBuffer> instanceBuffers;
    // std::vector<VkDeviceMemory> instanceBuffersMemory;
    // std::vector<void*> instanceBuffersMapped;
    // {
    //     instanceCounts.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    //     instanceBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    //     VkDeviceSize instanceBufferSize = sizeof(silk::InstanceData) * maxInstances;
    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         silk::validateVkResult(createBuffer(physicalDevice, device, instanceBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instanceBuffers[i], instanceBuffersMemory[i]), "Error: failed to create buffer!");
    //         silk::validateVkResult(vkMapMemory(device, instanceBuffersMemory[i], 0, instanceBufferSize, 0, &instanceBuffersMapped[i]), "Error: failed to map memory!");
    //     }
    // }

    const std::vector<uint16_t> TRI_INDICES = {
        0, 1, 2
    };

    // create (index) VkBuffer
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    {
        VkPhysicalDevice physicalDevice = deviceContext.getPhysicalDevice();
        VkDevice device = deviceContext.getDevice();
        VkDeviceSize indexBufferSize = sizeof(TRI_INDICES[0]) * TRI_INDICES.size(); 

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        silk::validateVkResult(createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory), "Error: failed to create index staging buffer!");

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, indexBufferSize, 0, &data);
            memcpy(data, TRI_INDICES.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        silk::validateVkResult(createBuffer(physicalDevice, device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory), "Error: failed to create index buffer!");
        silk::validateVkResult(copyBuffer(device, deviceContext.getGraphicsQueue(), commandPool, stagingBuffer, indexBuffer, indexBufferSize), "Error: failed to copy buffer!");

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    // create (uniform) VkBuffer
    // std::vector<VkBuffer> uniformBuffers;
    // std::vector<VkDeviceMemory> uniformBuffersMemory;
    // std::vector<void*> uniformBuffersMapped;
    // {
    //     uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //     uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    //     uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    //     VkDeviceSize bufferSize = sizeof(silk::CameraUBO);
    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         silk::validateVkResult(createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]), "Error: failed to create uniform buffers!");
    //         silk::validateVkResult(vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]), "Error: failed to map uniform buffers memory!");
    //     }
    // }

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

        silk::validateVkResult(vkCreateDescriptorPool(deviceContext.getDevice(), &descriptorPoolCreateInfo, nullptr, &descriptorPool), "Error: failed to create VkDescriptorPool!");
    }

    // TODO
    // - how do we link descriptor sets with buffers
    // create VkDescriptorSets
    // std::vector<VkDescriptorSet> descriptorSets;
    // {
    //     std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    //     VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    //     descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    //     descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    //     descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    //     descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

    //     descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    //     silk::validateVkResult(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets.data()), "Error: failed to create VkDescriptorSets!");

    //     for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    //     {
    //         VkDescriptorBufferInfo descriptorBufferInfo{};
    //         // descriptorBufferInfo.buffer = uniformBuffers[i];
    //         // descriptorBufferInfo.offset = 0;
    //         // descriptorBufferInfo.range = sizeof(silk::CameraUBO);

    //         VkWriteDescriptorSet writeDescriptorSet{};
    //         writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //         writeDescriptorSet.dstSet = descriptorSets[i];
    //         writeDescriptorSet.dstBinding = 0;
    //         writeDescriptorSet.dstArrayElement = 0;
    //         writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //         writeDescriptorSet.descriptorCount = 1;
    //         writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    //         vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    //     }
    // }

    // allocate VkCommandBuffer
    std::vector<VkCommandBuffer> commandBuffers;
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        silk::validateVkResult(vkAllocateCommandBuffers(deviceContext.getDevice(), &commandBufferAllocateInfo, commandBuffers.data()), "Error: failed to allocate VkCommandBuffer!");
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

        VkDevice device = deviceContext.getDevice();
        for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
        {
            
            silk::validateVkResult(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]), "Error: failed to create VkSemaphore!");
            silk::validateVkResult(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]), "Error: failed to create VkSemaphore!");
            silk::validateVkResult(vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]), "Error: failed to create VkFence!");
        }
    }

    // run
    uint32_t indexCount = TRI_INDICES.size();
    {
        VkDevice device = deviceContext.getDevice();

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
                VkResult result = vkAcquireNextImageKHR(device, swapchainContext.getSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    swapchainContext.recreate(window, deviceContext, renderPass);
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
                renderPassBeginInfo.framebuffer = swapchainContext.getFramebuffers()[imageIndex];
                renderPassBeginInfo.renderArea.offset = {0, 0};
                renderPassBeginInfo.renderArea.extent = swapchainContext.getExtent();

                VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
                renderPassBeginInfo.clearValueCount = 1;
                renderPassBeginInfo.pClearValues = &clearColor;

                vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.getPipeline());

                    VkViewport viewport{};
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = static_cast<float>(swapchainContext.getExtent().width);
                    viewport.height = static_cast<float>(swapchainContext.getExtent().height);
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;
                    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

                    VkRect2D scissor{};
                    scissor.offset = {0, 0};
                    scissor.extent = swapchainContext.getExtent();
                    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

                    VkBuffer vertexBuffers[] = { vertexBuffer };
                    VkDeviceSize offsets[] = { 0, 0 };
                    vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

                    vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

                    // vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

                    vkCmdDrawIndexed(commandBuffers[currentFrame], indexCount, 1, 0, 0, 0);

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

                silk::validateVkResult(vkQueueSubmit(deviceContext.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]), "Error: failed to submit queue!");

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;

                VkSwapchainKHR swapchains[] = { swapchainContext.getSwapchain() };
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = swapchains;
                presentInfo.pImageIndices = &imageIndex;

                result = vkQueuePresentKHR(deviceContext.getPresentQueue(), &presentInfo);
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
                {
                    framebufferResized = false;
                    swapchainContext.recreate(window, deviceContext, renderPass);
                }
                else if (result != VK_SUCCESS)
                {
                    throw std::runtime_error("Error: failed to present swapchain image!");
                }

                currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        }
    }

    VkDevice device = deviceContext.getDevice();
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
    // for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    // {
    //     vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    //     vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    // }

    // destroy (index) VkBuffer
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);

    // destroy (instance) VkBuffer
    // for (size_t i = 0; i < static_cast<size_t>(MAX_FRAMES_IN_FLIGHT); i++)
    // {
    //     vkFreeMemory(device, instanceBuffersMemory[i], nullptr);
    //     vkDestroyBuffer(device, instanceBuffers[i], nullptr);
    // }

    // destroy (vertex) VkBuffer
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);

    // destroy VkCommandPool
    vkDestroyCommandPool(device, commandPool, nullptr);

    // destroy VkDescriptorSetLayout
    // vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    // destroy renderPass
    vkDestroyRenderPass(device, renderPass, nullptr);

    // destroy glfw window
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}