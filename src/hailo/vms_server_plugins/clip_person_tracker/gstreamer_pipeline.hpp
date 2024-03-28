#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

#include "TextImageMatcher.hpp"
// #include "DetectionManager.h"

#include "exceptions.h"
#include "frame.h"
#include "detection.h"

#include <gst/gst.h>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {
class DeviceAgent; // Forward declaration of DeviceAgent

class GStreamerObjectDetector {
public:
    explicit GStreamerObjectDetector(std::filesystem::path pluginHomeDir, hailo::vms_server_plugins::clip_person_tracker::DeviceAgent* deviceAgentPtr);
    ~GStreamerObjectDetector();
    void ensureInitialized();
    bool isTerminated() const;
    void terminate();
    void set_debug(bool debug);
    DetectionList run(const Frame& frame);
    hailo::vms_server_plugins::clip_person_tracker::DeviceAgent* deviceAgent; // Pointer to DeviceAgent
    TextImageMatcher* m_textImageMatcher; // Pointer to TextImageMatcher
    // DetectionManager* m_DetectionManager; // Pointer to DetectionManager
    int m_thread_id; // Thread ID
    std::atomic<bool> m_debug;
private:
    void runPipeline();
    void pushFrameToPipeline(const Frame& frame);
    static void on_handoff_clip(GstElement* object, GstBuffer* buffer, gpointer data);
    std::unique_ptr<std::thread> pipeline_thread;
    std::atomic<bool> m_terminated{false};
    std::atomic<bool> m_loaded{false};
    // const std::filesystem::path m_modelPath;
    std::filesystem::path m_pluginHomeDir;
    std::mutex pipeline_mutex;
    GstElement* pipeline;
    GstElement* appsrc;
    GstElement* clip_matcher_identity;
    GMainLoop* main_loop;
    GstBus *bus;
};  

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
