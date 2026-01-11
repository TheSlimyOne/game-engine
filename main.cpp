// main.cpp

#include <iostream>
#include <thread>
#include <memory>

#include <GLFW/glfw3.h>

#include <entry/App.h>
#include <entry/Editor.h>
#include <core/World.h>
#include <core/Types.h>

int main() {
    auto world = std::make_shared<World>();

    Editor editor;
    App app(world);

    editor.init();
    app.init();

    while (!editor.should_close() && !app.should_close()) {
        glfwPollEvents();// Poll events from glfw, one call polls all windows

        editor.tick();
        app.tick();
    }

    return 0;
}