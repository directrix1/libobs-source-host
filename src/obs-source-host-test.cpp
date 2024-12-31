#include "libobs-source-host.hpp"
#include <iostream>
#include <obs/media-io/video-io.h>
#include <thread>

using namespace std;

int main()
{
    ObsSourceHost osh;
    cout << "Starting libsobs-source-host..." << endl;
    if (!osh.startCapturing("/usr/lib/obs-plugins/linux-capture.so;/usr/lib/obs-plugins/linux-pipewire.so", "", 1920, 1080, VIDEO_FORMAT_RGBA, 3)) {
        cout << "Failed to start." << endl;
        return 1;
    }
    cout << "Waiting for 10 seconds..." << endl;
    this_thread::sleep_for(chrono::seconds(10));
    cout << "Capturing: " << (osh.isCapturing() ? "yes" : "no") << endl;
    cout << "Stopping..." << endl;
    osh.stopCapturing();
    cout << "Buh bye!" << endl;
    return 0;
}
