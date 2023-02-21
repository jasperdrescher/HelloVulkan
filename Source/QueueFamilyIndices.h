#pragma once

#include <optional>

struct QueueFamilyIndices
{
    bool IsComplete() { return myGraphicsFamily.has_value() && myPresentFamily.has_value(); }

    std::optional<uint32_t> myGraphicsFamily;
    std::optional<uint32_t> myPresentFamily;
};
