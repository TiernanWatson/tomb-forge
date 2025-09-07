#include <iostream>

#include "Engine/Engine.h"

#if EDITOR_ENABLED
#include "Engine/Editor.h"
#endif

int main()
{
    using namespace TombForge;

    EngineContext context{};

    if (!InitEngine(context))
    {
        std::cerr << "Failed to initialize engine." << std::endl;
        return -1;
    }

#if EDITOR_ENABLED
    Editor editor(context);
#endif

    bool shouldQuit = false;
    while (!shouldQuit)
    {
        shouldQuit = !UpdateEngine(context);

#if EDITOR_ENABLED
        editor.Update();
#endif

        SwapBuffers(context);
        PollEvents(context);
    };

    DestroyEngine(context);

    return 0;
}

