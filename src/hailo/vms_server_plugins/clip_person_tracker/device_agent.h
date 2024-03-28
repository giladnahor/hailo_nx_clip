// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <filesystem>

#include <nx/sdk/analytics/helpers/object_metadata_packet.h>
#include <nx/sdk/analytics/helpers/consuming_device_agent.h>
#include <nx/sdk/helpers/uuid_helper.h>
#include <nx/sdk/ptr.h>

#include "engine.h"
#include "gstreamer_pipeline.hpp"

// Tappas includes
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "gst_hailo_meta.hpp"

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

class DeviceAgent: public nx::sdk::analytics::ConsumingDeviceAgent
{
public:
    using MetadataPacketList = std::vector<nx::sdk::Ptr<nx::sdk::analytics::IMetadataPacket>>;
public:
    DeviceAgent(
        const nx::sdk::IDeviceInfo* deviceInfo,
        std::filesystem::path pluginHomeDir,
        int DeviceAgentId);
    virtual ~DeviceAgent() override;
    int m_DeviceAgentId; // Device Agent ID

protected:
    virtual std::string manifestString() const override;

    virtual bool pushUncompressedVideoFrame(
        const nx::sdk::analytics::IUncompressedVideoFrame* videoFrame) override;

    virtual void doSetNeededMetadataTypes(
        nx::sdk::Result<void>* outValue,
        const nx::sdk::analytics::IMetadataTypes* neededMetadataTypes) override;

public:
    nx::sdk::Ptr<nx::sdk::analytics::IMetadataPacket> generateEventMetadataPacket();

    nx::sdk::Ptr<nx::sdk::analytics::ObjectMetadataPacket> detectionsToObjectMetadataPacket(
        const DetectionList& detections,
        int64_t timestampUs);

    void pushMetadataPacketWrapper(MetadataPacketList metadataPackets);

    // Used for checking whether the frame size changed, and for reinitializing the tracker.
    int64_t m_lastVideoFrameTimestampUs = 0;
    std::unique_ptr<GStreamerObjectDetector> m_objectDetector;
    std::filesystem::path m_pluginHomeDir;
private:
    MetadataPacketList processFrame(
        const nx::sdk::analytics::IUncompressedVideoFrame* videoFrame);

private:
    bool m_FirstSetting = true;
    const std::string kPersonObjectType = "nx.base.Person";
    const std::string kCatObjectType = "nx.base.Cat";
    const std::string kDogObjectType = "nx.base.Dog";
    const std::string kNewTrackEventType = "nx.sample.newTrack";

    /** Length of the the track (in frames). The value was chosen arbitrarily. */
    static constexpr int kTrackFrameCount = 256;

    /** Should work on modern PCs. */
    static constexpr int kDetectionFramePeriod = 2;

private:
    bool m_terminated = false;
    bool m_terminatedPrevious = false;
    nx::sdk::Uuid m_trackId = nx::sdk::UuidHelper::randomUuid();
    int m_frameIndex = 0; /**< Used for generating the detection in the right place. */
    int m_trackIndex = 0; /**< Used in the description of the events. */

//settings
public:
    static const std::string kTimeShiftSetting;
private:
    mutable std::mutex m_mutex;
    int m_timestampShiftMs = 0;
protected:
    virtual nx::sdk::Result<const nx::sdk::ISettingsResponse*> settingsReceived() override;

};

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
