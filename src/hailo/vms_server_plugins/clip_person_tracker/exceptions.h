// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <stdexcept>

#include <opencv2/core/core.hpp>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

class Error: public std::runtime_error { using std::runtime_error::runtime_error; };

class ObjectDetectorError: public Error { using Error::Error; };

class ObjectDetectorInitializationError: public ObjectDetectorError
    { using ObjectDetectorError::ObjectDetectorError; };

class ObjectDetectorIsTerminatedError: public ObjectDetectorError
    { using ObjectDetectorError::ObjectDetectorError; };

class ObjectDetectionError: public ObjectDetectorError
    { using ObjectDetectorError::ObjectDetectorError; };

inline std::string cvExceptionToStdString(const cv::Exception& e)
{
    return "OpenCV error: " + e.err + " (error code: " + std::to_string(e.code) + ")";
}

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
