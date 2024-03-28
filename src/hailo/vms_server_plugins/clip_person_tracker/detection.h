// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <memory>
#include <vector>
#include <string>

#include <nx/sdk/analytics/rect.h>
#include <nx/sdk/uuid.h>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

// Class labels for the MobileNet SSD model (VOC dataset).
extern const std::vector<std::string> kClasses;
extern const std::vector<std::string> kClassesToDetect;

/**
 * Stores information about detection (one box per frame).
 */
struct Detection
{
    const nx::sdk::analytics::Rect boundingBox;
    const std::string classLabel;
    const float confidence;
    const nx::sdk::Uuid trackId;
    const std::string ClipLabel;
    const float ClipConfidence;
};

using DetectionList = std::vector<std::shared_ptr<Detection>>;

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
