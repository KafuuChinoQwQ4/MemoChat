#include "AppPortRegistry.h"

void AppPortRegistry::bindFeaturePorts()
{
    bindAuthFeaturePorts();
    bindContactFeaturePorts();
    bindGroupFeaturePorts();
    bindProfileFeaturePorts();
}
