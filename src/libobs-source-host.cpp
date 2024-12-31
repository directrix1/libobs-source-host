#include "libobs-source-host.hpp"
#include <iostream>
#include <obs/media-io/video-io.h>
#include <obs/obs-frontend-api.h>
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
            state->run_state = OHRS_RUNNING;

        } else {
            // TODO: capture frames, maybe just use a semafore and wait
        }
    }
    stopObs();
    state->run_state = OHRS_STOPPED;
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
        cout << "Video successfully inited." << endl;
        break;
    DEFAULT:
        cout << "Video failed to init: " << res << endl;
        state->run_state = OHRS_STOP;
        return false;
        break;
    }
    struct obs_audio_info oai = {
        .samples_per_sec = 48000,
        .speakers = SPEAKERS_STEREO,
    };
    if (obs_reset_audio(&oai)) {
        cout << "Audio successfully inited." << endl;
    } else {
        cout << "Audio failed to init." << endl;
        state->run_state = OHRS_STOP;
        return false;
    }
    return true;
}

bool ObsSourceHost::loadModule(const char* moduleName)
{
    obs_module_t* cur_module;
    int code = obs_open_module(&cur_module, moduleName, NULL);
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

bool ObsSourceHost::loadModules()
{
    size_t curPos = 0, endPos = 0;
    cout << "Loading modules..." << endl;
    string moduleName;
    if (moduleList.size() > 0) {
        while ((endPos = moduleList.find(";", curPos)) != string::npos) {
            moduleName = moduleList.substr(curPos, endPos - curPos);
            loadModule(moduleName.c_str());
            endPos = curPos = endPos + 1;
        }
        // Don't forget the last or only one'
        loadModule(moduleList.c_str() + curPos);
    }
    obs_post_load_modules();
    cout << "Loading modules finished." << endl;
    return true;
}

void ObsSourceHost::stopObs()
{
    obs_shutdown();
}

ObsSourceHost::ObsSourceHost()
{
    inited = false;
}

ObsSourceHost::~ObsSourceHost()
{
}

bool ObsSourceHost::startCapturing(const string module_list, const string source_config, uint32_t width, uint32_t height, enum video_format format, uint32_t num_frame_buffers)
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
        munmap(state, shm_size);
        exit(0);
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
    return NULL;
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
