// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "device_agent.h"

#include <chrono>
#include <exception>

#include <nx/sdk/analytics/helpers/event_metadata.h>
#include <nx/sdk/analytics/helpers/event_metadata_packet.h>
#include <nx/sdk/analytics/helpers/object_metadata.h>
#include <nx/sdk/analytics/helpers/object_metadata_packet.h>
#include <nx/sdk/helpers/string.h>

#include "detection.h"
#include "exceptions.h"
#include "frame.h"

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

using namespace std::string_literals;

/**
 * @param deviceInfo Various information about the related device, such as its id, vendor, model,
 *     etc.
 */
DeviceAgent::DeviceAgent(
    const nx::sdk::IDeviceInfo* deviceInfo,
    std::filesystem::path pluginHomeDir,
    int DeviceAgentId)
    : ConsumingDeviceAgent(deviceInfo, /*enableOutput*/ true)
{
    
    std::cout << "DeviceAgentId: " << DeviceAgentId << std::endl;
    // Create m_objectDetector
    m_pluginHomeDir = pluginHomeDir;
    m_DeviceAgentId = DeviceAgentId;
    m_objectDetector = std::make_unique<GStreamerObjectDetector>(pluginHomeDir, this);
}

DeviceAgent::~DeviceAgent()
{
    try
    {
        m_objectDetector->terminate();
        m_terminated = true;
    }
    catch (const std::exception& e)
    {
        std::cout << "DeviceAgent::~DeviceAgent() Exception caught: " << e.what() << std::endl;
    }
    catch (...)
    {    
        std::cout << "DeviceAgent::~DeviceAgent() Unknown exception caught" << std::endl;
    }
    // wait 5 seconds before returning
    std::cout << "DeviceAgent::~DeviceAgent() waiting 5 seconds before returning" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "DeviceAgent::~DeviceAgent() returning" << std::endl;
}

/**
 *  @return JSON with the particular structure. Note that it is possible to fill in the values
 * that are not known at compile time, but should not depend on the DeviceAgent settings.
 */
std::string DeviceAgent::manifestString() const
{
    // Tell the Server that the plugin can generate the events and objects of certain types.
    // Id values are strings and should be unique. Format of ids:
    // `{vendor_id}.{plugin_id}.{event_type_id/object_type_id}`.
    //
    // See the plugin manifest for the explanation of vendor_id and plugin_id.
    return /*suppress newline*/ 1 + R"json(
{
    "eventTypes": [
        {
            "id": ")json" + kNewTrackEventType + R"json(",
            "name": "New track started"
        }
    ],
    "supportedTypes": [
        {
            "objectTypeId": ")json" + kPersonObjectType + R"json("
        },
        {
            "objectTypeId": ")json" + kCatObjectType + R"json("
        },
        {
            "objectTypeId": ")json" + kDogObjectType + R"json("
        }
    ]
}
)json";
}

/**
 * Called when the Server sends a new uncompressed frame from a camera.
 */
bool DeviceAgent::pushUncompressedVideoFrame(const IUncompressedVideoFrame* videoFrame)
{
    m_terminated = m_terminated || m_objectDetector->isTerminated();
    if (m_terminated)
    {
        if (!m_terminatedPrevious)
        {
            pushPluginDiagnosticEvent(
                IPluginDiagnosticEvent::Level::error,
                "Plugin is in broken state.",
                "Disable the plugin.");
            m_terminatedPrevious = true;
        }
        return true;
    }

    m_lastVideoFrameTimestampUs = videoFrame->timestampUs();

    // Detecting objects only on every `kDetectionFramePeriod` frame.
    if (m_frameIndex % kDetectionFramePeriod == 0)
    {
        const MetadataPacketList metadataPackets = processFrame(videoFrame);
        for (const Ptr<IMetadataPacket>& metadataPacket: metadataPackets)
        {
            metadataPacket->addRef();
            pushMetadataPacket(metadataPacket.get());
        }
    }

    ++m_frameIndex;
    return true;
}

void DeviceAgent::doSetNeededMetadataTypes(
    nx::sdk::Result<void>* outValue,
    const nx::sdk::analytics::IMetadataTypes* /*neededMetadataTypes*/)
{
    if (m_terminated)
        return;

    try
    {
        m_objectDetector->ensureInitialized();
    }
    catch (const ObjectDetectorInitializationError& e)
    {
        *outValue = {ErrorCode::otherError, new String(e.what())};
        m_terminated = true;
    }
    catch (const ObjectDetectorIsTerminatedError& /*e*/)
    {
        m_terminated = true;
    }
};

// wrapper function to allow accessing protected pushMetadataPacket function
void DeviceAgent::pushMetadataPacketWrapper(MetadataPacketList metadataPackets)
{
    try {
        for (const Ptr<IMetadataPacket>& metadataPacket: metadataPackets)
        {
            metadataPacket->addRef();
            pushMetadataPacket(metadataPacket.get());
        }
    } catch (const std::exception& e) {
        // NX_PRINT << "Exception caught: " << e.what() << std::endl;
        NX_PRINT << "Exception caught: ";
    } catch (...) {
        NX_PRINT << "Unknown exception caught" << std::endl;
    }
}

//-------------------------------------------------------------------------------------------------
// private

Ptr<IMetadataPacket> DeviceAgent::generateEventMetadataPacket()
{
    // Generate event every kTrackFrameCount'th frame.
    if (m_frameIndex % kTrackFrameCount != 0)
        return nullptr;

    // EventMetadataPacket contains arbitrary number of EventMetadata.
    const auto eventMetadataPacket = makePtr<EventMetadataPacket>();
    // Bind event metadata packet to the last video frame using a timestamp.
    eventMetadataPacket->setTimestampUs(m_lastVideoFrameTimestampUs);
    // Zero duration means that the event is not sustained, but momental.
    eventMetadataPacket->setDurationUs(0);

    // EventMetadata contains an information about event.
    const auto eventMetadata = makePtr<EventMetadata>();
    // Set all required fields.
    eventMetadata->setTypeId(kNewTrackEventType);
    eventMetadata->setIsActive(true);
    eventMetadata->setCaption("New sample plugin track started");
    eventMetadata->setDescription("New track #" + std::to_string(m_trackIndex) + " started");

    eventMetadataPacket->addItem(eventMetadata.get());

    // Generate index and track id for the next track.
    ++m_trackIndex;
    m_trackId = nx::sdk::UuidHelper::randomUuid();

    return eventMetadataPacket;
}

Ptr<ObjectMetadataPacket> DeviceAgent::detectionsToObjectMetadataPacket(
    const DetectionList& detections,
    int64_t timestampUs)
{
    if (detections.empty())
        return nullptr;

    const auto objectMetadataPacket = makePtr<ObjectMetadataPacket>();

    for (const std::shared_ptr<Detection>& detection: detections)
    {
        const auto objectMetadata = makePtr<ObjectMetadata>();

        objectMetadata->setBoundingBox(detection->boundingBox);
        objectMetadata->setConfidence(detection->confidence);
        objectMetadata->setTrackId(detection->trackId);

        // Convert class label to object metadata type id.
        if (detection->classLabel == "person")
            objectMetadata->setTypeId(kPersonObjectType);
        else if (detection->classLabel == "cat")
            objectMetadata->setTypeId(kCatObjectType);
        else if (detection->classLabel == "dog")
            objectMetadata->setTypeId(kDogObjectType);
        else
            objectMetadata->setTypeId(kPersonObjectType); //default to person
        objectMetadataPacket->addItem(objectMetadata.get());
        // Add clip label and confidence
        if (detection->ClipConfidence > 0.0){
            objectMetadata->addAttribute(makePtr<Attribute>(detection->ClipLabel, std::to_string(detection->ClipConfidence)));
            objectMetadata->addAttribute(makePtr<Attribute>("match", "true"));
        }
    }
    objectMetadataPacket->setTimestampUs(timestampUs);

    return objectMetadataPacket;
}

DeviceAgent::MetadataPacketList DeviceAgent::processFrame(
    const IUncompressedVideoFrame* videoFrame)
{
    const Frame frame(videoFrame, m_frameIndex);

    try
    {
        DetectionList detections = m_objectDetector->run(frame);
    }
    catch (const ObjectDetectionError& e)
    {
        pushPluginDiagnosticEvent(
            IPluginDiagnosticEvent::Level::error,
            "Object detection error.",
            e.what());
        m_terminated = true;
    }
    // Detections are pushed from the GstreamerObjectDetector
    return {};
}

//Settings
const std::string DeviceAgent::kTimeShiftSetting = "timestampShiftMs";
nx::sdk::Result<const nx::sdk::ISettingsResponse*> DeviceAgent::settingsReceived()
{
    if (m_FirstSetting || m_terminated)
    {
        m_FirstSetting = false;
        return nullptr;
    }
    const std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> textSettings(5);
    std::vector<bool> textSettings_negative(5);
    std::string textSettingsString = "";
    double detectionThreshold = 0.9;
    bool debug = false;
    const std::map<std::string, std::string>& settings = currentSettings();
    for (const auto& entry: settings)
    {
        const std::string& key = entry.first;
        const std::string& value = entry.second;
        if (key == kTimeShiftSetting){
            m_timestampShiftMs = std::stoi(value);
            std::cout << "m_timestampShiftMs: " << m_timestampShiftMs << std::endl;
        }

        for (int i = 0; i < 5; ++i) {
            std::string key_textSetting = "textSetting" + std::to_string(i);
            // std::string key_textSetting_negative = "textSetting" + std::to_string(i) + "_negative";

            if (key == key_textSetting) {
                textSettings[i] = value;
                std::cout << key_textSetting << ": " << value << std::endl;
                if (value != "")
                    textSettingsString += "\"" + value + "\" ";
            }

            // if (key == key_textSetting_negative) {
            //     textSettings_negative[i] = (value == "true");
            //     std::cout << key_textSetting_negative << ": " << value << std::endl;
            // }
        }
        if (key == "Threshold") {     
            detectionThreshold = std::stod(value);
            std::cout << "Detection Thr: " <<  detectionThreshold << std::endl;
        }
        if (key == "debug_mode") {     
            if (value == "true")
                debug = true;    
            std::cout << "debug mode: " <<  value << std::endl;
        }
        
    }
    // run text embedding
    
    std::string command = "text_image_matcher --texts-list " + textSettingsString + " --output " + m_pluginHomeDir.string() + "/resources/nx_text_embedding.json";

    std::thread t([command, detectionThreshold, debug, this] {
        this->m_objectDetector->m_textImageMatcher->set_prompt_update(true);
        std::cout << "run text embedding....." << std::endl;
        std::cout << "command: " << command << std::endl;
        auto ret = std::system(command.c_str());
        std::cout << "ret: " << ret << std::endl;
        if (this->m_objectDetector && this->m_objectDetector->m_textImageMatcher) {
            this->m_objectDetector->m_textImageMatcher->load_embeddings(m_pluginHomeDir.string() + "/resources/nx_text_embedding.json");
            this->m_objectDetector->m_textImageMatcher->set_threshold(detectionThreshold);
            this->m_objectDetector->m_textImageMatcher->set_debug(debug);
            this->m_objectDetector->m_textImageMatcher->set_prompt_update(false);
        } else {
            std::cerr << "Error: m_objectDetector or m_textImageMatcher is null." << std::endl;
        }
    std::cout << "text embedding finished" << std::endl;
    });

    t.detach();

    std::cout << "keep running....." << std::endl;
    return nullptr;
}

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
