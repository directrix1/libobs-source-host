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
            int res;

            obs_startup("en_US", NULL, NULL);
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
                break;
            }
            if (state->run_state == OHRS_START) {
                state->run_state = OHRS_RUNNING;
            }

            // TODO: init source and setup frame capture
        } else {
            // TODO: capture frames, maybe just use a semafore and wait
        }
    }
    obs_shutdown();
    state->run_state = OHRS_STOPPED;
}

ObsSourceHost::ObsSourceHost()
{
    inited = false;
}

ObsSourceHost::~ObsSourceHost()
{
}

bool ObsSourceHost::startCapturing(uint32_t width, uint32_t height, enum video_format format, uint32_t num_frames)
{
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
    uint32_t shm_size = 4096 + num_frames * frame_size;

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
        .num_frames = num_frames,
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
    if (state->run_state == OHRS_STOPPED) {
        return;
    }
    state->run_state = OHRS_STOP;
    while (state->run_state != OHRS_STOPPED) { };
    waitpid(child_process, NULL, 0);
    munmap(state, state->shm_size);
    inited = false;
}
