#include "app/Application.h"
#include "app/CrashHandler.h"
#include <fmod.hpp>

int main() {
    CrashHandler::Install();
    Application app;
    app.Run();
    return 0;
}
