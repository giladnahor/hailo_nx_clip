// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/kit/ini_config.h>
#include <nx/sdk/analytics/helpers/pixel_format.h>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {


struct Ini: public nx::kit::IniConfig
{
    Ini(): IniConfig("hailo_clip_plugin.ini") { reload(); }

    NX_INI_FLAG(0, enableOutput, "");
};

Ini& ini();

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo