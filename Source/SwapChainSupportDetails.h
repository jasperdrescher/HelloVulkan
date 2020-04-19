#pragma once

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR myCapabilities;
    std::vector<VkSurfaceFormatKHR> myFormats;
    std::vector<VkPresentModeKHR> myPresentModes;
};
