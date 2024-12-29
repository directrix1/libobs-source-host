#pragma once

#include <cstdint>
#include <obs/media-io/video-io.h>

enum obs_host_run_state {
    OHRS_STOPPED = 0,
    OHRS_START = 1,
    OHRS_RUNNING = 2,
    OHRS_STOP = 3,
};

struct obs_host_state {
    obs_host_run_state run_state;
    uint32_t output_width;
    uint32_t output_height;
    enum video_format output_format;
    uint32_t frame_size;
    uint32_t shm_size;
    uint32_t num_frames;
    uint32_t cur_frame;
};

class ObsSourceHost {
private:
protected:
    bool inited;
    struct obs_host_state* state;
    uint8_t* buffers;
    pid_t child_process;

    void obsRunner();

public:
    ObsSourceHost();
    ~ObsSourceHost();

    bool startCapturing(uint32_t width, uint32_t height, enum video_format output_format, uint32_t num_frames);
    bool isCapturing();
    void* getNewestBuffer();
    void stopCapturing();
};
