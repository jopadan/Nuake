#pragma once
#include <src/Core/Maths.h>
#include <string>

struct LaunchSettings
{
    int32_t monitor = 1;
    Nuake::Vector2 resolution = { 1100, 630 };
    std::string windowTitle = "Nuake Editor ";
    std::string projectPath;
};
