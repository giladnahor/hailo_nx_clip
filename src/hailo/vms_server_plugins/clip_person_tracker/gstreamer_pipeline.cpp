#include <gst/gst.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdlib> 
#include <opencv2/opencv.hpp>

#include <nx/sdk/analytics/helpers/object_metadata_packet.h>
#include <nx/sdk/analytics/helpers/consuming_device_agent.h>
#include <nx/sdk/helpers/uuid_helper.h>
#include <nx/sdk/ptr.h>

#include "exceptions.h"
#include "frame.h"
#include "device_agent.h"

#include "gstreamer_pipeline.hpp"
#include "TextImageMatcher.hpp"
// #include "DetectionManager.h"

// Tappas includes
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo_xtensor.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"

using nx::sdk::Ptr;
using nx::sdk::analytics::IMetadataPacket;
using nx::sdk::analytics::ObjectMetadataPacket;

namespace hailo {
namespace vms_server_plugins {
namespace clip_person_tracker {

GStreamerObjectDetector::GStreamerObjectDetector(std::filesystem::path pluginHomeDir, hailo::vms_server_plugins::clip_person_tracker::DeviceAgent* deviceAgentPtr)
    : deviceAgent(deviceAgentPtr) // Initialize the DeviceAgent pointer
{
    m_pluginHomeDir = pluginHomeDir;
    
    // Initialize GStreamer and create pipeline
    pipeline_thread = std::make_unique<std::thread>(&GStreamerObjectDetector::runPipeline, this);
    try {
        m_textImageMatcher = TextImageMatcher::getInstance("RN50x4", 0.5, 10);
        m_textImageMatcher->load_embeddings(m_pluginHomeDir.string() + "/resources/nx_text_embedding.json");
        // m_DetectionManager = DetectionManager::getInstance();
    } catch (const std::exception& e) {
        NX_PRINT << "An error occurred: " << e.what() << std::endl;
    } catch (...) {
        NX_PRINT << "An unknown error occurred." << std::endl;
    }
    // m_thread_id = std::this_thread::get_id();
}


GStreamerObjectDetector::~GStreamerObjectDetector() {
    if (pipeline_thread && pipeline_thread->joinable()) {
        pipeline_thread->join();
    }
    terminate();
}


void GStreamerObjectDetector::ensureInitialized() {
    // Ensure the pipeline is initialized and running
    if (isTerminated())
    {
        throw ObjectDetectorIsTerminatedError(
            "Object detector initialization error: object detector is terminated.");
    }
    if (m_loaded)
        return;

}

bool GStreamerObjectDetector::isTerminated() const {
    return m_terminated;
}

void GStreamerObjectDetector::terminate() {
    std::cout << "Terminating GStreamer pipeline" << std::endl;
    if (isTerminated())
        return;
    
    NX_PRINT << "Terminating GStreamer pipeline";
    try {
        // Send EOS to the pipeline
        if (this->m_loaded && this->pipeline != nullptr && this->appsrc != nullptr) {
            std::cout << "sending eos" << std::endl;
            try {
                GstFlowReturn ret;
                g_signal_emit_by_name(this->appsrc, "end-of-stream", &ret);
                if (ret != GST_FLOW_OK) {
                    // handle error
                    std::cout << "Error sending EOS to pipeline" << std::endl;
                }
            } 
            catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
            }
        } 
        else {
            std::cout << "GStreamerObjectDetector::terminate(): Pipeline not started yet" << std::endl;
            return;
        }
        // Stop the GStreamer pipeline
        std::cout << "stop pipeline" << std::endl;
        
        // check if pipline is running
        GstState current_state;
        if (gst_element_get_state(this->pipeline, &current_state, NULL, GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_FAILURE) {
            if (current_state == GST_STATE_PLAYING) {
                std::cout << "Stop pipeline: Setting state to NULL" << std::endl;
                GstStateChangeReturn ret = gst_element_set_state(this->pipeline, GST_STATE_NULL);
                if (ret == GST_STATE_CHANGE_FAILURE) {
                    std::cout << "Failed to set pipeline state to NULL" << std::endl;
                }
                std::cout << "Stop pipeline: Setting state to NULL done" << std::endl;
            }
        }

        // if (this->pipeline != nullptr){
        //     std::cout << "stop pipeline setting state to null" << std::endl;
        //     gst_element_set_state(this->pipeline, GST_STATE_NULL);
        //     std::cout << "stop pipeline unrefing pipeline" << std::endl;
        //     // gst_object_unref(this->pipeline);
        // }

        // std::cout << "stop pipeline unrefing appsrc" << std::endl;
        // if (this->appsrc != nullptr){
        //     gst_object_unref(this->appsrc);
        // }
        std::cout << "stop pipeline unrefing clip_matcher_identity" << std::endl;
        
        // if (this->clip_matcher_identity != nullptr){
        //     gst_object_unref(this->clip_matcher_identity);
        // }
        std::cout << "stop pipeline unrefing main_loop" << std::endl;
        if (this->main_loop != nullptr){
            g_main_loop_quit(this->main_loop);
            g_main_loop_unref(this->main_loop);
        }
        std::cout << "stop pipeline done" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "An error occurred while stopping the pipeline: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "An unknown error occurred while stopping the pipeline." << std::endl;
    }
    m_terminated = true;
    
}

DetectionList GStreamerObjectDetector::run(const Frame& frame) {
    if (isTerminated())
        throw ObjectDetectorIsTerminatedError("Detection error: object detector is terminated.");

    try
    {
        pushFrameToPipeline(frame);
    }
    catch (const std::exception& e)
    {
        terminate();
        throw ObjectDetectionError(std::string("Pushing frame to pipeline: Error: ") + e.what());

    }
    return {};
}
gboolean async_bus_callback(GstBus *bus, GstMessage *message, gpointer user_data)
{
  GStreamerObjectDetector *detector = static_cast<GStreamerObjectDetector *>(user_data);
  switch (GST_MESSAGE_TYPE(message))
  {
  case GST_MESSAGE_ERROR:
  {
    // An error occurred in the pipeline
    GError *error = nullptr;
    gchar *debug_info = nullptr;
    gst_message_parse_error(message, &error, &debug_info);
    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(message->src), error->message);
    g_printerr("Debugging info: %s\n", debug_info ? debug_info : "none");
    g_clear_error(&error);
    g_free(debug_info);
    // stop main loop
    // g_main_loop_quit(detector->main_loop);
    break;
  }
  case GST_MESSAGE_EOS:
    // The pipeline has reached the end of the stream
    g_print("End-Of-Stream reached.\n");
    // stop main loop
    // g_main_loop_quit(detector->main_loop);
    break;
  
  // print QOS message
  case GST_MESSAGE_QOS:
  {
    std::cout << "QOS message detected from " << GST_OBJECT_NAME(message->src) << std::endl;
    break;
  }
  default:
    // Print a message for other message types
    // g_print("Received message of type %s\n", GST_MESSAGE_TYPE_NAME(message));
    break;
  }
  // We want to keep receiving messages
  return TRUE;
}

void GStreamerObjectDetector::runPipeline() {
    gst_init(nullptr, nullptr);
    int deviceAgentId = this->deviceAgent->m_DeviceAgentId;
    std::string deviceAgentIdStr = std::to_string(deviceAgentId);
    std::cout << "runPipeline() Device agent ID: " << deviceAgentIdStr << " PID: " << getpid() << ", Thread ID: " << std::this_thread::get_id() << ", this pointer: " << this << std::endl;
    // Run the GStreamer pipeline in a separate thread
    std::string clip_vdevice = "1"; // hailo used for CLIP
    std::string detection_vdevice = "3"; // Hailo used for detection
    bool multi_device = true; // Set to true to use multi hailo chips
    if (multi_device)
    {
        if (deviceAgentId % 2 == 1){
            detection_vdevice = "2";
        }
    }
    else 
    {
        detection_vdevice = "1";
    }
    std::cout << "ID: " << deviceAgentIdStr << " Detection vdevice: " << detection_vdevice << std::endl;

    std::string hef_path = this->m_pluginHomeDir.string() + "/resources/yolov5s_personface.hef";
    std::string clip_hef_path = this->m_pluginHomeDir.string() + "/resources/clip_resnet_50x4.hef";
    
    std::string post_so_path = this->m_pluginHomeDir.string() + "/resources/libyolo_post.so";
    std::string config_path = this->m_pluginHomeDir.string() + "/resources/configs/yolov5_personface.json";
    std::string clip_cropper_so_path = this->m_pluginHomeDir.string() + "/resources/libclip_croppers.so";
    std::string clip_post_so_path = this->m_pluginHomeDir.string() + "/resources/libclip_post.so";
    std::string cpp_aspect_fix_path = this->m_pluginHomeDir.string() + "/resources/libaspect_ratio_fix.so";
    std::string WHOLE_BUFFER_CROP_SO = this->m_pluginHomeDir.string() + "/resources/libwhole_buffer.so";
    std::string pipeline_string = "appsrc name=app_source ! "
    "video/x-raw, width=1280, height=720, format=RGB ! "
    "queue leaky=downstream max-size-buffers=3 max-size-bytes=0 max-size-time=0 name=pre_detection_tee max-size-buffers=12 name=pre_detection_tee ! "
    "hailocropper  name=detection_crop so-path=" + WHOLE_BUFFER_CROP_SO + " function-name=create_crops use-letterbox=true resize-method=inter-area internal-offset=true "
    "hailoaggregator name=agg1 "
    "detection_crop. ! queue leaky=no max-size-buffers=20 max-size-bytes=0 max-size-time=0 silent=true name=detection_bypass_q ! agg1.sink_0 "
    "detection_crop. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 silent=true name=pre_detecion_net ! "
    "video/x-raw, pixel-aspect-ratio=1/1 ! "
    "hailonet hef-path=" + hef_path + " batch-size=8 vdevice-group-id=" + detection_vdevice + " "
    "multi-process-service=false scheduler-timeout-ms=100 scheduler-priority=31 ! "
    "queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 name=pre_detecion_post ! "
    "hailofilter so-path=" +  post_so_path + " qos=false function_name=yolov5_personface_letterbox config-path=" + config_path + " ! "
    "queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! "
    "agg1.sink_1 "
    "agg1. ! "   
    "queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! "
    "hailotracker name=hailo_tracker class-id=1 kalman-dist-thr=0.8 iou-thr=0.9 init-iou-thr=0.7 keep-new-frames=2 keep-tracked-frames=15 keep-lost-frames=2 keep-past-metadata=true qos=false ! "
    "queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! "
    "hailocropper so-path=" + clip_cropper_so_path + " function-name=person_cropper internal-offset=true name=cropper use-letterbox=true no-scaling-bbox=true "
    "hailoaggregator name=agg cropper. ! "
    "queue leaky=no max-size-buffers=20 max-size-bytes=0 max-size-time=0 name=clip_bypass_q ! "
    "agg.sink_0 cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 name=pre_clip_net ! "
    "hailonet hef-path=" + clip_hef_path + " vdevice-group-id=" + clip_vdevice + " multi-process-service=false batch-size=8 scheduler-timeout-ms=1000 ! "
    "queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! "
    "hailofilter name=clip_post so-path=" + clip_post_so_path + " qos=false ! "
    "queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg.sink_1 agg. ! "
    "queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! "
    "identity name=clip_matcher_identity ! "
    "fakesink silent=true name=clip_matcher_sink sync=false async=false qos=false ";
    
    
    NX_PRINT << "Running pipeline: " << pipeline_string;
    // Parse the pipeline string and create the pipeline
    GError* error = nullptr;
    this->pipeline = gst_parse_launch(pipeline_string.c_str(), &error);
    if (error) {
        // throw ObjectDetectorInitializationError("Error creating pipeline: " + std::string(error->message));
        std::cout << "Error creating pipeline ID: " << deviceAgentIdStr << " " << error->message << std::endl;
    }
    std::cout << "Parsing pipeline ID: " << deviceAgentIdStr << " done" << std::endl;
    // connect bus to pipeline
    this->bus = gst_pipeline_get_bus(GST_PIPELINE(this->pipeline));
    gst_bus_add_watch(this->bus, async_bus_callback, this);

    // Get the appsrc element from the pipeline
    this->appsrc = gst_bin_get_by_name(GST_BIN(this->pipeline), "app_source");
    
    //Set appsrc properties
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                    "format", G_TYPE_STRING, "RGB",
                                    "width", G_TYPE_INT, 1280,
                                    "height", G_TYPE_INT, 720,
                                    NULL);
    g_object_set(G_OBJECT(this->appsrc), "caps", caps, NULL);
    gst_caps_unref(caps);

    // get the clip_matcher_identity element from the pipeline
    this->clip_matcher_identity = gst_bin_get_by_name(GST_BIN(this->pipeline), "clip_matcher_identity");
    // Connect to the "handoff" signal emitted by the identity element
    g_signal_connect(this->clip_matcher_identity, "handoff", G_CALLBACK(this->on_handoff_clip), this);

    // Set the pipeline state to PLAYING
    std::cout << "Running pipeline ID: " << deviceAgentIdStr << " setting pipeline to playing" << std::endl;
    gst_element_set_state(this->pipeline, GST_STATE_PLAYING);
    // check if the pipeline is running
    if (gst_element_get_state(this->pipeline, nullptr, nullptr, GST_SECOND) == GST_STATE_CHANGE_FAILURE) {
        NX_PRINT << "Error running pipeline ID: " << deviceAgentIdStr << std::endl;
        std::cout << "Error running pipeline ID: " << deviceAgentIdStr << std::endl;
        // throw ObjectDetectorInitializationError("Error running pipeline");
    }
    std::cout << "Running pipeline ID: " << deviceAgentIdStr << " done" << std::endl;
    // Run the main loop this is blocking will run until the main loop is stopped
    this->main_loop = g_main_loop_new(nullptr, FALSE);
    this->m_loaded = true;
    g_main_loop_run(this->main_loop);
    // Free resources
    this->terminate();

}


// Helper function to get xtensor from HailoMatrixPtr
static xt::xarray<float> get_xtensor(HailoMatrixPtr matrix)
{
    // Adapt a HailoTensorPtr to an xarray (quantized)
    xt::xarray<float> xtensor = xt::adapt(matrix->get_data().data(), matrix->size(), xt::no_ownership(), matrix->shape());
    // remove (squeeze) xtensor first dim
    xtensor = xt::squeeze(xtensor, 0);
    return xtensor;
}
// This function is called when the identity element emits the "handoff" signal
void GStreamerObjectDetector::on_handoff_clip(GstElement* object, GstBuffer* buffer, gpointer data) {
    GStreamerObjectDetector* detector = static_cast<GStreamerObjectDetector*>(data);
    
    // if terminated, return
    if (detector->isTerminated())
    {
        return;
    }

    // Get Hailo ROI from buffer
    HailoROIPtr roi;
    roi = get_hailo_main_roi(buffer, false);
    if (roi == nullptr)
    {
        return;
    }
    // define 2D array for image embedding
    xt::xarray<double> image_embedding;
    
    // vector to hold detections
    std::vector<HailoDetectionPtr> detections_ptrs;
    
    // vector to hold used detections
    std::vector<HailoDetectionPtr> used_detections;
    
    // Get detections from roi
    detections_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &detection : detections_ptrs)
    {
        auto matrix_objs = detection->get_objects_typed(HAILO_MATRIX);
        if (matrix_objs.size() > 0)
        {
            HailoMatrixPtr matrix_ptr = std::dynamic_pointer_cast<HailoMatrix>(matrix_objs[0]);
            xt::xarray<float> embeddings = get_xtensor(matrix_ptr);
            // if image_embedding is empty or 0-dimensional, initialize it with embeddings
            if (image_embedding.size() == 0 || image_embedding.dimension() == 0)
            {
                image_embedding = embeddings;
            }
            else 
            {
                // if image_embedding is not empty and not 0-dimensional, concatenate it with embeddings
                image_embedding = xt::concatenate(xt::xtuple(image_embedding, embeddings), 0);
            }
            used_detections.push_back(detection);
        }
    }
    
    // if image_embedding is empty, return
    if (image_embedding.size() == 0 || image_embedding.dimension() == 0)
    {
        return;
    }
    
    std::vector<Match> matches = detector->m_textImageMatcher->match(image_embedding);
    for (auto &match : matches)
    {
        auto detection = used_detections[match.row_idx];
        auto old_classifications = hailo_common::get_hailo_classifications(detection);
        for (auto old_classification : old_classifications)
        {
            if (old_classification->get_classification_type() == "clip")
            detection->remove_object(old_classification);
        }
        // if (match.negative || !match.passed_threshold)
        // {
        //     continue;
        // }
        HailoClassificationPtr classification = std::make_shared<HailoClassification>(std::string("clip"), match.text, match.similarity);
        detection->add_object(classification);
    }
    
    // NX detections
    DetectionList nx_detections;

    // get buffer dts
    GstClockTime dts = GST_BUFFER_DTS(buffer);
    //convert dts to microseconds
    uint64_t timestampUs = dts / 1000;
    
    //print detections
    for (HailoDetectionPtr &detection : used_detections)
    {
        if (detection->get_label() != "person")
            continue;
        // get BBOX
        HailoBBox bbox = detection->get_bbox();
        //get track id
        std::vector<HailoUniqueIDPtr> track_id = hailo_common::get_hailo_track_id(detection);
        int id = 9999;
        if (track_id.size() == 1) {
            id = track_id[0]->get_id();
            // NX_PRINT << "Handoff Probe track_id: " << id;
        } 
        else {
            std::cout << " ID not found" << std::endl;
        }
        

        std::vector<HailoClassificationPtr> classifications = hailo_common::get_hailo_classifications(detection);
        std::string clip_text = "";
        float clip_confidence = 0.0;
        bool prompt_upadte = detector->m_textImageMatcher->get_prompt_update();
        for (auto classification : classifications)
        {
            if (classification->get_classification_type() == "clip")
            {
                if (prompt_upadte)
                {
                    detection->remove_object(classification);
                }
                else
                {
                clip_text = classification->get_label();
                clip_confidence = classification->get_confidence();
                // std::cout << clip_text << " " << clip_confidence << std::endl;
                }
            }
        }
        // convert hailo detection to nx detection
        const std::shared_ptr<Detection> nx_detection = std::make_shared<Detection>(Detection{
            /*boundingBox*/ nx::sdk::analytics::Rect(bbox.xmin(), bbox.ymin(), bbox.width(), bbox.height()),
            /*classLabel*/ detection->get_label(),
            /*confidence*/ detection->get_confidence(),
            /*m_trackId*/ nx::sdk::Uuid(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,id),
            /*ClipLabel*/ clip_text,
            /*ClipConfidence*/ clip_confidence
        });
        nx_detections.push_back(nx_detection);
    }

    try {
        const auto& objectMetadataPacket =
            detector->deviceAgent->detectionsToObjectMetadataPacket(nx_detections, timestampUs);
        hailo::vms_server_plugins::clip_person_tracker::DeviceAgent::MetadataPacketList metadataPackets;
        
        if (objectMetadataPacket)
        {
            metadataPackets.push_back(objectMetadataPacket);
        }
        // Push metadata packets to the DeviceAgent
        detector->deviceAgent->pushMetadataPacketWrapper(metadataPackets);
        
    }
    catch (const std::exception& e) {
        NX_PRINT << "Exception caught: " << e.what() << '\n';
    }
    catch (...) {
        NX_PRINT << "Unknown exception caught\n";
    }
    
    return ;
}
    
void GStreamerObjectDetector::pushFrameToPipeline(const Frame& frame) {
    
    //check if pipeline is already running if not return
    if (!this->m_loaded) {
        return;
    }
    
    // Push frame data to the appsrc element in the GStreamer pipeline  
    const cv::Mat image = frame.cvMat;
    if (frame.width != 1280 || frame.height != 720) {
        // throw ObjectDetectionError("Frame size is not 1280x720");
        NX_PRINT << "Frame size is not 1280x720 width: " << frame.width << " height: " << frame.height << std::endl;
        std::cout << "Frame size is not 1280x720 width: " << frame.width << " height: " << frame.height << std::endl;
        return;
    }
    
    // convert cv::Mat to GstBuffer
    auto timestampUs = frame.timestampUs;
    GstClockTime timestampNs = (GstClockTime)(timestampUs * 1000); // convert to nanoseconds
    GstBuffer* buffer = gst_buffer_new_wrapped_full(
        (GstMemoryFlags)GST_MEMORY_FLAG_READONLY,
        (gpointer)image.data,
        image.total() * image.elemSize(),
        0,
        image.total() * image.elemSize(),
        nullptr,
        nullptr);
    
    // set buffer timestamp will be used later in the on_handoff function
    buffer->pts = timestampNs;
    buffer->dts = timestampNs;
    buffer->duration = GST_CLOCK_TIME_NONE;
    
    GstFlowReturn ret;
    g_signal_emit_by_name(this->appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
    if (ret != GST_FLOW_OK) {
        // throw std::runtime_error("Error pushing buffer to pipeline");
        std::cout << "Error pushing buffer to pipeline" << std::endl;
    }
    return;
}

} // namespace clip_person_tracker
} // namespace vms_server_plugins
} // namespace hailo
