#include "obs-source-host.hpp"
#include <csignal>
#include <glib.h>
#include <obs/media-io/video-io.h>
#include <obs/obs-data.h>
#include <obs/obs-frontend-api.h>
#include <obs/obs-properties.h>
#include <obs/obs.h>
#include <sys/mman.h>
#include <sys/wait.h>

using namespace std;

void ObsSourceHost::obsRunner()
{
    while (state->run_state == OHRS_STOPPED) { }
    while (state->run_state != OHRS_STOP) {
        if (state->run_state == OHRS_START) {
            if (!initObs()) {
                state->run_state = OHRS_STOP;
                continue;
            }
            if (!loadModules()) {
                state->run_state = OHRS_STOP;
                continue;
            }
            size_t idx;
            char* item_type;
            idx = 0;
            while (obs_enum_source_types(idx++, (const char**)&item_type)) {
                blog(LOG_DEBUG, "Available source type: %s\n", item_type);
            }
            idx = 0;
            while (obs_enum_input_types(idx++, (const char**)&item_type)) {
                blog(LOG_DEBUG, "Available input type: %s\n", item_type);
            }
            idx = 0;
            while (obs_enum_filter_types(idx++, (const char**)&item_type)) {
                blog(LOG_DEBUG, "Available filter type: %s\n", item_type);
            }
            idx = 0;
            while (obs_enum_transition_types(idx++, (const char**)&item_type)) {
                blog(LOG_DEBUG, "Available transition type: %s\n", item_type);
            }
            idx = 0;
            while (obs_enum_output_types(idx++, (const char**)&item_type)) {
                blog(LOG_DEBUG, "Available output type: %s\n", item_type);
            }
            idx = 0;
            while (obs_enum_encoder_types(idx++, (const char**)&item_type)) {
                blog(LOG_DEBUG, "Available encoder type: %s\n", item_type);
            }
            idx = 0;
            while (obs_enum_service_types(idx++, (const char**)&item_type)) {
                blog(LOG_DEBUG, "Available service type: %s\n", item_type);
            }
            if (!createScene()) {
                state->run_state = OHRS_STOP;
                continue;
            }

            state->run_state = OHRS_RUNNING;
        } else {
            g_main_context_iteration(NULL, false);
        }
    }
    stopObs();
}

bool ObsSourceHost::initObs()
{
    int res;

    obs_startup("en-US", NULL, NULL);
    struct obs_video_info ovi = {
        .graphics_module = "libobs-opengl",
        .fps_num = 60,
        .fps_den = 1,
        .base_width = state->output_width,
        .base_height = state->output_height,
        .output_width = state->output_width,
        .output_height = state->output_height,
        .output_format = state->output_format,
        .adapter = 0,
        .gpu_conversion = true,
        .colorspace = VIDEO_CS_DEFAULT,
        .range = VIDEO_RANGE_DEFAULT,
        .scale_type = OBS_SCALE_DISABLE,
    };
    switch (res = obs_reset_video(&ovi)) {
    case OBS_VIDEO_SUCCESS:
        blog(LOG_DEBUG, "Video successfully inited.\n");
        break;
    default:
        blog(LOG_DEBUG, "Video failed to init: %d\n", res);
        state->run_state = OHRS_STOP;
        return false;
        break;
    }
    struct obs_audio_info oai = {
        .samples_per_sec = 48000,
        .speakers = SPEAKERS_STEREO,
    };
    if (obs_reset_audio(&oai)) {
        blog(LOG_DEBUG, "Audio successfully inited.\n");
    } else {
        blog(LOG_DEBUG, "Audio failed to init.\n");
        state->run_state = OHRS_STOP;
        return false;
    }
    return true;
}

bool ObsSourceHost::loadModule(const char* moduleName, const char* moduleData)
{
    obs_module_t* cur_module;
    int code = obs_open_module(&cur_module, moduleName, moduleData);
    switch (code) {
    case MODULE_MISSING_EXPORTS:
        blog(LOG_DEBUG, "Failed to load module file '%s', not an OBS plugin", moduleName);
        break;
    case MODULE_FILE_NOT_FOUND:
        blog(LOG_DEBUG, "Failed to load module file '%s', file not found", moduleName);
        break;
    case MODULE_ERROR:
        blog(LOG_DEBUG, "Failed to load module file '%s'", moduleName);
        break;
    case MODULE_INCOMPATIBLE_VER:
        blog(LOG_DEBUG, "Failed to load module file '%s', incompatible version", moduleName);
        break;
    case MODULE_HARDCODED_SKIP:
        blog(LOG_DEBUG, "Hardcoded skip module file '%s'", moduleName);
        break;
    }
    if (!obs_init_module(cur_module)) {
        blog(LOG_DEBUG, "Module '%s', failed to init", moduleName);
        return false;
    }
    return true;
}

void ObsSourceHost::findModule(ObsSourceHost* param, const struct obs_module_info2* info)
{

    size_t curPos = 0, endPos = 0;
    string moduleList = param->moduleList;
    string moduleName;
    string huntModuleName = string(info->name);
    if (moduleList.size() > 0) {
        while ((endPos = moduleList.find(";", curPos)) != string::npos) {
            moduleName = moduleList.substr(curPos, endPos - curPos);
            if (moduleName == huntModuleName) {
                param->loadModule(info->bin_path, info->data_path);
                return;
            }
            endPos = curPos = endPos + 1;
        }
        // Don't forget the last or only one'
        moduleName = moduleList.substr(curPos, moduleList.length() - curPos);
        if (moduleName == huntModuleName) {
            param->loadModule(info->bin_path, info->data_path);
            return;
        }
    }
}

bool ObsSourceHost::loadModules()
{
    blog(LOG_INFO, "Loading modules...\n");
    obs_find_modules2((obs_find_module_callback2_t)&ObsSourceHost::findModule, (void*)this);
    blog(LOG_INFO, "Done finding modules.\n");
    obs_post_load_modules();
    blog(LOG_INFO, "Loading modules finished.\n");
    return true;
}

void ObsSourceHost::capturedFrameCallback(ObsSourceHost* param, struct video_data* frame)
{
    uint32_t nextFrameNum = (param->state->cur_frame + 1) % (param->state->num_frames);
    memcpy(param->buffers + (nextFrameNum * (param->state->frame_size)), (frame->data)[0], param->state->frame_size);
    param->state->cur_frame = nextFrameNum;
}

bool ObsSourceHost::createScene()
{
    const char* sourceName = sourceConfig.c_str();
    sourceSettings = obs_get_source_defaults(sourceName);
    blog(LOG_DEBUG, "Source %s defaults: %s\n", sourceName, obs_data_get_json(sourceSettings));
    if ((source = obs_source_create(sourceName, "thesource", NULL, NULL)) == NULL) {
        blog(LOG_DEBUG, "Could not create %s source.\n", sourceName);
        return false;
    }
    obs_set_output_source(0, source);
    obs_add_raw_video_callback(NULL, (void (*)(void*, struct video_data*)) & ObsSourceHost::capturedFrameCallback, (void*)this);
    sourceSettings = obs_source_get_settings(source);
    sourceProps = obs_source_properties(source);
    obs_property_t* prop = obs_properties_first(sourceProps);
    do {
        const char* propName = obs_property_name(prop);
        blog(LOG_INFO, "Property: %s\n", propName);
    } while (obs_property_next(&prop));
    obs_properties_destroy(sourceProps);
    return true;
}

void ObsSourceHost::stopObs()
{
    obs_set_output_source(0, NULL);
    obs_source_release(source);
    obs_shutdown();
    g_main_context_iteration(NULL, false);
    state->run_state = OHRS_STOPPED;
    munmap(state, state->shm_size);
    exit(0);
}

ObsSourceHost::ObsSourceHost()
{
    inited = false;
}

ObsSourceHost::~ObsSourceHost()
{
}

bool ObsSourceHost::startCapturing(const char* module_list, const char* source_config, uint32_t width, uint32_t height, enum video_format format, uint32_t num_frame_buffers)
{
    moduleList = module_list;
    sourceConfig = source_config;

    uint32_t frame_size = width * height;
    switch (format) {
    case VIDEO_FORMAT_RGBA:
    case VIDEO_FORMAT_BGRA:
        frame_size *= 4;
        break;
    default:
        return false;
        break;
    }
    uint32_t shm_size = 4096 + num_frame_buffers * frame_size;

    state = (struct obs_host_state*)mmap(NULL,
        shm_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1, 0);
    buffers = (uint8_t*)state;
    buffers += 4096;

    *state = {
        .run_state = OHRS_START,
        .output_width = width,
        .output_height = height,
        .output_format = format,
        .frame_size = frame_size,
        .shm_size = shm_size,
        .num_frames = num_frame_buffers,
        .cur_frame = 0,
    };
    child_process = fork();
    switch (child_process) {
    case -1:
        munmap(state, shm_size);
        return false;
    case 0:
        obsRunner();
        return true;
    default:
        inited = true;
        return true;
    }
}

bool ObsSourceHost::isCapturing()
{
    if (inited) {
        return state->run_state == OHRS_RUNNING;
    }
    return false;
}

void* ObsSourceHost::getNewestBuffer()
{
    return buffers + (state->frame_size * state->cur_frame);
}

void ObsSourceHost::stopCapturing()
{
    if (inited) {
        switch (state->run_state) {
        OHRS_STOPPED:
        OHRS_STOP:
            break;
        default:
            state->run_state = OHRS_STOP;
        }
        while (state->run_state != OHRS_STOPPED) { };
        waitpid(child_process, NULL, 0);
        munmap(state, state->shm_size);
        inited = false;
    }
}
