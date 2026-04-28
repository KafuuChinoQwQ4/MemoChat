#pragma once

#define MEMOCHAT_R18_PLUGIN_ABI_VERSION 1

extern "C" {
using R18PluginAbiVersionFn = int (*)();
using R18PluginCallFn = const char* (*)(const char* request_json);
using R18PluginFreeFn = void (*)(const char* response_json);
using R18PluginShutdownFn = void (*)();
}
