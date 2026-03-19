#include "GateServerDrogon.h"
#include <iostream>
#include <memory>
#include <csignal>
#include <drogon/drogon.h>

int main(int argc, char* argv[])
{
    // Set up signal handling for graceful shutdown
    signal(SIGINT, [](int) {
        std::cout << "Received SIGINT, shutting down..." << std::endl;
        drogon::app().quit();
    });
    signal(SIGTERM, [](int) {
        std::cout << "Received SIGTERM, shutting down..." << std::endl;
        drogon::app().quit();
    });

    // Load configuration from config.ini
    GateServerDrogon::GetInstance()->Initialize();

    // Run the Drogon application
    drogon::app().run();

    return 0;
}
