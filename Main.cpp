#include <iostream>

#include "Engine.h"

int main()
{
    std::cout << "TombForge v0.0.1" << std::endl;

    TombForge::Engine engine;

    while (engine.Update()) {}

    std::cout << "TombForge terminated" << std::endl;

    return 0;
}

