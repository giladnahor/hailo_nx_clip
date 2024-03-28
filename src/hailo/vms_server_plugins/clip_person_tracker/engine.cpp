// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "engine.h"

#include "device_agent.h"
#include "hailo_clip_plugin_ini.h"

#include <nx/kit/json.h>

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

int Engine::m_DeviceManagerCounter = 0;

Engine::Engine(std::filesystem::path pluginHomeDir):
    // Call the DeviceAgent helper class constructor telling it to verbosely report to stderr.
    nx::sdk::analytics::Engine(ini().enableOutput),
    m_pluginHomeDir(pluginHomeDir)
{
}

Engine::~Engine()
{
}

/**
 * Called when the Server opens a video-connection to the camera if the plugin is enabled for this
 * camera.
 *
 * @param outResult The pointer to the structure which needs to be filled with the resulting value
 *     or the error information.
 * @param deviceInfo Contains various information about the related device such as its id, vendor,
 *     model, etc.
 */
void Engine::doObtainDeviceAgent(Result<IDeviceAgent*>* outResult, const IDeviceInfo* deviceInfo)
{
    std::cout << "m_DeviceManagerCounter: " << m_DeviceManagerCounter << std::endl;
    *outResult = new DeviceAgent(deviceInfo, m_pluginHomeDir, m_DeviceManagerCounter);
    m_DeviceManagerCounter++;
}

/**
 *  @return JSON with the particular structure. Note that it is possible to fill in the values
 * that are not known at compile time, but should not depend on the Engine settings.
 */
std::string Engine::manifestString() const
{
    using namespace nx::kit;
    Json::array generationSettings;
    
    generationSettings.push_back(Json::object{ {"type", "Separator"} });
    
    //banner
    Json::object banner ={
                    {"type", "Banner"},
                    {"icon", "info"},
                    {"text", "Edit text box below to set searched person's attributes"}
                };
    generationSettings.push_back(std::move(banner));

    // check if the json file exists
    std::string jsonPath = m_pluginHomeDir.string() + "resources/nx_text_embedding.json";
    
    std::vector<std::string> defaultValues = {"man with a striped shirt", "man with blue jeans", "man with red hat", "man", "woman"};
    
    
    Json::object textSetting = {
        {"type", "TextField"},
        {"name", "textSetting" + std::to_string(0)},
        {"caption", "A photo of a "},
        {"description", "describe the person you want to find"},
        {"defaultValue", defaultValues[0]}
    };
    generationSettings.push_back(std::move(textSetting));
    
    Json::object banner2 ={
                    {"type", "Banner"},
                    {"icon", "info"},
                    {"text", "Edit text boxes below to set negative prompts for the person's attributes"}
                };
    generationSettings.push_back(std::move(banner2));

    for (int i = 1; i < 5; ++i) {
        Json::object textSetting = {
            {"type", "TextField"},
            {"name", "textSetting" + std::to_string(i)},
            {"caption", "A photo of a "},
            {"description", "describe the person you want to find"},
            {"defaultValue", defaultValues[i]}
        };
        generationSettings.push_back(std::move(textSetting));

    }
    generationSettings.push_back(Json::object{ {"type", "Separator"} });

    Json::object Threshold = {
        {"type", "DoubleSpinBox"},
        {"name", "Threshold"},
        {"caption", "Detection threshold: "},
        {"defaultValue", "0.9"},
        {"minValue", "0.0"},
        {"maxValue", "1.0"},
    };
    generationSettings.push_back(std::move(Threshold));

    Json::object debug_mode = {
        {"type", "CheckBox"},
        {"caption", "Debug mode"},
        {"name", "debug_mode"},
        {"description", "Enable to get all classifications"},
        {"defaultValue", false}
    };
    generationSettings.push_back(std::move(debug_mode));
    
    Json::object settingsModel = {
        {"type", "Settings"},
        {"items", generationSettings}
    };
    Json::object engineManifest = {
        {"capabilities", "needUncompressedVideoFrames_rgb"},
        {"deviceAgentSettingsModel", settingsModel}
    };
    return Json(engineManifest).dump();
    
//     // Ask the Server to supply uncompressed video frames in RGB format
//     return /*suppress newline*/ 1 + R"json(
// {
//     "capabilities": "needUncompressedVideoFrames_rgb"
// }
// )json";
}

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
