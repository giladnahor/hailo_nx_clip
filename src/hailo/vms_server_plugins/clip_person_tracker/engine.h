// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <filesystem>

#include <nx/sdk/analytics/helpers/plugin.h>
#include <nx/sdk/analytics/helpers/engine.h>
#include <nx/sdk/analytics/i_uncompressed_video_frame.h>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

class Engine: public nx::sdk::analytics::Engine
{
public:
    explicit Engine(std::filesystem::path pluginHomeDir);

    virtual ~Engine() override;

protected:
    virtual std::string manifestString() const override;

protected:
    virtual void doObtainDeviceAgent(
        nx::sdk::Result<nx::sdk::analytics::IDeviceAgent*>* outResult,
        const nx::sdk::IDeviceInfo* deviceInfo) override;

private:
    std::filesystem::path m_pluginHomeDir;
    // Add counter to allow to instantiate every device agent with a unique ID
    static int m_DeviceManagerCounter;

};

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
