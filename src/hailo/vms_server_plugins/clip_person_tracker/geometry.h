// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <opencv2/core/core.hpp>

#include <nx/kit/debug.h>
#include <nx/sdk/analytics/rect.h>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

inline cv::Rect nxRectToCvRect(nx::sdk::analytics::Rect rect, int width, int height)
{
    if (!NX_KIT_ASSERT(width > 0) || !NX_KIT_ASSERT(height > 0))
        return {};

    return cv::Rect(
        (int) (rect.x * width),
        (int) (rect.y * height),
        (int) (rect.width * width),
        (int) (rect.height * height)
    ) & cv::Rect(0, 0, width, height); //< Ensure that the result is inside the frame rect.
}

inline nx::sdk::analytics::Rect cvRectToNxRect(cv::Rect rect, int width, int height)
{
    if (!NX_KIT_ASSERT(width > 0) || !NX_KIT_ASSERT(height > 0))
        return {};

    return nx::sdk::analytics::Rect(
        (float) rect.x / width,
        (float) rect.y / height,
        (float) rect.width / width,
        (float) rect.height / height);
}

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
