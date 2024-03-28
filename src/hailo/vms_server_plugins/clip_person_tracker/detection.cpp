// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "detection.h"

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

// Class labels for the MobileNet SSD model (VOC dataset).
const std::vector<std::string> kClasses{
    "background", "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat",
    "chair", "cow", "dining table", "dog", "horse", "motorbike", "person", "potted plant",
    "sheep", "sofa", "train", "tv monitor"
};
const std::vector<std::string> kClassesToDetect{"cat", "dog", "person"};

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
