#pragma once

#define NOMINMAX
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

struct SwapChainSupportDetails;
struct QueueFamilyIndices;

class HelloTriangleApp
{
public:
    HelloTriangleApp();

    void Run();

private:
    void InitializeWindow();
    static void FramebufferResizeCallback(GLFWwindow* aWindow, int aWidth, int aHeight);
    void InitializeVulkan();
    void MainLoop();
    void CleanupSwapChain();
    void Cleanup();
    void RecreateSwapChain();
    void CreateInstance();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& aCreateInfo);
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void DrawFrame();
    VkShaderModule CreateShaderModule(const std::vector<char>& aShaderCode);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& someAvailableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& someAvailablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& aCapabilities);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice aDevice);
    static std::vector<char> ReadFile(const std::string& aFilename);

    bool IsDeviceSuitable(VkPhysicalDevice aDevice);
    bool HasDeviceExtensionSupport(VkPhysicalDevice aDevice);
    bool HasValidationLayerSupport();

    QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice aDevice);
    std::vector<const char*> GetRequiredExtensions();

private:
    std::string myResourcesPath;
    GLFWwindow* myGLFWWindow;
    VkInstance myVkInstance;
    VkDebugUtilsMessengerEXT myVkDebugMessenger;
    VkSurfaceKHR myVkSurface;
    VkPhysicalDevice myVkPhysicalDevice;
    VkDevice myVkDevice;
    VkQueue myVkGraphicsQueue;
    VkQueue myVkPresentQueue;
    VkSwapchainKHR myVkSwapChain;
    VkFormat myVkSwapChainImageFormat;
    VkExtent2D myVkSwapChainExtent;
    VkRenderPass myVkRenderPass;
    VkPipelineLayout myVkPipelineLayout;
    VkPipeline myVkGraphicsPipeline;
    VkCommandPool myVkCommandPool;
    std::vector<VkImage> myVkSwapChainImages;
    std::vector<VkImageView> myVkSwapChainImageViews;
    std::vector<VkFramebuffer> myVkSwapChainFramebuffers;
    std::vector<VkCommandBuffer> myVkCommandBuffers;
    std::vector<VkSemaphore> myVkImageAvailableSemaphores;
    std::vector<VkSemaphore> myVkRenderFinishedSemaphores;
    std::vector<VkFence> myVkInFlightFences;
    std::vector<VkFence> myVkImagesInFlight;
    int myCurrentFrameIndex;
    bool myIsFramebufferResized;
};
