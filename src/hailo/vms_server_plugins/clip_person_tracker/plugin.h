// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/plugin.h>
#include <nx/sdk/analytics/i_engine.h>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

class Plugin: public nx::sdk::analytics::Plugin
{
protected:
    virtual nx::sdk::Result<nx::sdk::analytics::IEngine*> doObtainEngine() override;
    virtual std::string manifestString() const override;
};

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
