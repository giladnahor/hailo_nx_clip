// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "hailo_clip_plugin_ini.h"

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {


Ini& ini()
{
    static Ini ini;
    return ini;
}

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo