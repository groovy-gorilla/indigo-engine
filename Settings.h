#ifndef SETTINGS_H
#define SETTINGS_H

enum DISPLAY_MODE {FULLSCREEN, WINDOWED};


struct Settings {
    uint32_t WIDTH = 640;
    uint32_t HEIGHT = 480;
    DISPLAY_MODE DISPLAY_MODE = FULLSCREEN;
    bool ASPECT_RATIO = true;
    const int MAX_FRAMES_IN_FLIGHT = 2;
    VkSampleCountFlagBits MSAA_SAMPLES = VK_SAMPLE_COUNT_1_BIT;
    VkFilter FILTER = VK_FILTER_LINEAR;
};

extern Settings SETTINGS;

// MSAA samples
// filter (nearest / linear)

#endif //SETTINGS_H
