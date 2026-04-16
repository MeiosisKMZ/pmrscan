#pragma once

#include <gui/gui.h>
#include <gui/view_port.h>
#include <subghz/devices/devices.h>

typedef enum {
    ScanDirDown,
    ScanDirUp,
} ScanDir;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
    bool running;
	uint32_t freq_channel;
    uint32_t frequency;
    float rssi;
    float sensitivity;
    bool scanning;
    ScanDir scan_dir;
    const SubGhzDevice* radio_device;
    bool speaker_acquired;
} PMRScanApp;

PMRScanApp* pmrscan_app_alloc(void);
void pmrscan_app_free(PMRScanApp* app);
int32_t pmrscan_app(void* p);
