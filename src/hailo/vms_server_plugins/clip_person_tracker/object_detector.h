// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// #include <opencv2/dnn.hpp>

#include <nx/sdk/helpers/uuid_helper.h>
#include <nx/sdk/uuid.h>

#include "detection.h"
#include "frame.h"

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

class ObjectDetector
{
public:
    explicit ObjectDetector(std::filesystem::path modelPath);

    void ensureInitialized();
    bool isTerminated() const;
    void terminate();
    DetectionList run(const Frame& frame);

private:
    void loadModel();
    DetectionList runImpl(const Frame& frame);

private:
    bool m_netLoaded = false;
    bool m_terminated = false;
    const std::filesystem::path m_modelPath;
    // std::unique_ptr<cv::dnn::Net> m_net;
    nx::sdk::Uuid m_trackId = nx::sdk::UuidHelper::randomUuid();
};

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
