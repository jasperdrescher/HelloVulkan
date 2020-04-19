#pragma once

#include <cstdint>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> myGraphicsFamily;
    std::optional<uint32_t> myPresentFamily;

    bool IsComplete()
    {
        return myGraphicsFamily.has_value() && myPresentFamily.has_value();
    }
};
