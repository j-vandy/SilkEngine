#include "silk/Engine.h"

#include <iostream>

// TODO
// - 2D paint
//      - Get previous & current frame's mouse position
//      - Draw capsule from previous to curr mouse pos (7/2)
//      - Control diameter with scroll
//      - Swap color with numbers
//      - add a clear button to reset
//      - Improve API: minimize code duplication
// - flatland RC or holographic RC (7/13)
// - Improve API
// - screen space RC, Input System, "Phox" Engine, ...

bool framebufferResized = false;
void framebufferResizeCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
    framebufferResized = true;
}

int main()
{
    const uint32_t WIDTH = 960;
    const uint32_t HEIGHT = 960;
    const char* APPLICATION_NAME = "Paint";
    silk::WindowContext windowContext(WIDTH, HEIGHT, APPLICATION_NAME);
    glfwSetFramebufferSizeCallback(windowContext.getWindow(), framebufferResizeCallback);

    silk::DeviceContextCreateInfo deviceContextCreateInfo{};
    deviceContextCreateInfo.applicationName = APPLICATION_NAME;

    silk::DeviceContext deviceContext(windowContext.getWindow(), deviceContextCreateInfo);

    silk::RenderPassContext renderPassContext(deviceContext);

    silk::SwapchainContext swapchainContext(windowContext.getWindow(), deviceContext, renderPassContext.getRenderPass());

    silk::DescriptorSetLayoutContext descriptorSetLayoutContext(deviceContext.getDevice(), {});

    struct BrushPC
    {
        glm::vec2 position;
        
        static VkPushConstantRange getPushConstantRange()
        {
            return VkPushConstantRange
            {
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                static_cast<uint32_t>(sizeof(BrushPC))
            };
        }
    } brushPC;

    silk::PipelineContextCreateInfo pipelineContextCreateInfo{};
    pipelineContextCreateInfo.descriptorSetLayouts = { descriptorSetLayoutContext.getDescriptorSetLayout() };
    pipelineContextCreateInfo.pushConstantRanges = { BrushPC::getPushConstantRange() };

    silk::PipelineContext pipelineContext(deviceContext.getDevice(), renderPassContext.getRenderPass(), pipelineContextCreateInfo);

    silk::CommandPoolContext commandPoolContext(deviceContext);

    const int MAX_FRAMES_IN_FLIGHT = 2;
    silk::DescriptorPoolContext descriptorPoolContext(deviceContext.getDevice(), {}, MAX_FRAMES_IN_FLIGHT);

    std::vector<VkCommandBuffer> commandBuffers(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(silk::allocateCommandBuffers(deviceContext.getDevice(), commandPoolContext.getCommandPool(), commandBuffers));

    // create synchronization objects
    std::vector<silk::SemaphoreContext> imageAvailableSemaphores;
    std::vector<silk::FenceContext> inFlightFences;
    imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        imageAvailableSemaphores.emplace_back(deviceContext.getDevice());
        inFlightFences.emplace_back(deviceContext.getDevice());
    }

    std::vector<silk::SemaphoreContext> renderFinishedSemaphores;
    const size_t swapchainImageCount = swapchainContext.getSwapchainImageCount();
    renderFinishedSemaphores.reserve(swapchainImageCount);
    for (size_t i = 0; i < swapchainImageCount; i++)
    {
        renderFinishedSemaphores.emplace_back(deviceContext.getDevice());
    }

    // draw frame
    {
        VkDevice device = deviceContext.getDevice();

        uint32_t currentFrame = 0;
        while(!glfwWindowShouldClose(windowContext.getWindow()))
        {
            glfwPollEvents();

            // update ModelPC
            double xpos, ypos;
            glfwGetCursorPos(windowContext.getWindow(), &xpos, &ypos);
            brushPC.position = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));

            // draw frame
            {
                VkFence fence = inFlightFences[currentFrame].getFence();
                vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

                uint32_t imageIndex;
                VkResult result = vkAcquireNextImageKHR(device, swapchainContext.getSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame].getSemaphore(), VK_NULL_HANDLE, &imageIndex);

                if (result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    swapchainContext.recreate(windowContext.getWindow(), deviceContext, renderPassContext.getRenderPass());
                    continue;
                }
                else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                {
                    throw std::runtime_error("Error: failed to aquire next swapchain image!");
                }

                vkResetFences(device, 1, &fence);

                vkResetCommandBuffer(commandBuffers[currentFrame], 0);

                // record command buffer
                VkCommandBufferBeginInfo commandBufferBeginInfo{};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                VK_CHECK(vkBeginCommandBuffer(commandBuffers[currentFrame], &commandBufferBeginInfo));

                VkRenderPassBeginInfo renderPassBeginInfo{};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = renderPassContext.getRenderPass();
                renderPassBeginInfo.framebuffer = swapchainContext.getFramebuffers()[imageIndex];
                renderPassBeginInfo.renderArea.offset = {0, 0};
                renderPassBeginInfo.renderArea.extent = swapchainContext.getExtent();

                std::vector<VkClearValue> clearValues(2);
                clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
                clearValues[1].depthStencil = { 1.0f, 0 };

                renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                renderPassBeginInfo.pClearValues = clearValues.data();

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

                    vkCmdPushConstants(commandBuffers[currentFrame], pipelineContext.getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BrushPC), &brushPC);

                    vkCmdDraw(commandBuffers[currentFrame], 6, 1, 0, 0);

                vkCmdEndRenderPass(commandBuffers[currentFrame]);

                VK_CHECK(vkEndCommandBuffer(commandBuffers[currentFrame]));

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

                VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame].getSemaphore() };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

                VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex].getSemaphore() };
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                VK_CHECK(vkQueueSubmit(deviceContext.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame].getFence()));

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
                    swapchainContext.recreate(windowContext.getWindow(), deviceContext, renderPassContext.getRenderPass());
                }
                else if (result != VK_SUCCESS)
                {
                    throw std::runtime_error("Error: failed to present swapchain image!");
                }

                currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        }
    }

    return EXIT_SUCCESS;
}