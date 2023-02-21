#include "HelloTriangleApp.h"

#include <iostream>

int main()
{
    HelloTriangleApp app;

    try
    {
        app.Run();
    }
    catch (const std::exception& anException)
    {
        std::cerr << anException.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
