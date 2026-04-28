#include "R18PluginProtocol.h"

#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

template <typename T>
T LoadSymbol(
#ifdef _WIN32
    HMODULE lib,
#else
    void* lib,
#endif
    const char* name)
{
#ifdef _WIN32
    return reinterpret_cast<T>(GetProcAddress(lib, name));
#else
    return reinterpret_cast<T>(dlsym(lib, name));
#endif
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "usage: R18PluginHost <plugin-library>\n";
        return 2;
    }

#ifdef _WIN32
    HMODULE lib = LoadLibraryA(argv[1]);
#else
    void* lib = dlopen(argv[1], RTLD_NOW);
#endif
    if (!lib) {
        std::cerr << "failed to load plugin\n";
        return 3;
    }

    auto abi = LoadSymbol<R18PluginAbiVersionFn>(lib, "r18_plugin_abi_version");
    auto call = LoadSymbol<R18PluginCallFn>(lib, "r18_plugin_call");
    auto free_result = LoadSymbol<R18PluginFreeFn>(lib, "r18_plugin_free");
    auto shutdown = LoadSymbol<R18PluginShutdownFn>(lib, "r18_plugin_shutdown");
    if (!abi || !call || !free_result || abi() != MEMOCHAT_R18_PLUGIN_ABI_VERSION) {
        std::cerr << "invalid r18 plugin ABI\n";
        return 4;
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }
        const char* response = call(line.c_str());
        if (response) {
            std::cout << response << std::endl;
            free_result(response);
        } else {
            std::cout << "{\"error\":500,\"message\":\"plugin returned null\"}" << std::endl;
        }
    }

    if (shutdown) {
        shutdown();
    }

#ifdef _WIN32
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
    return 0;
}
