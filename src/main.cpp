#include "AppHost.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    voidlayer::AppHost app;
    return app.Run(instance, showCommand);
}
