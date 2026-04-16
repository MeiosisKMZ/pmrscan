#include "pmrscan_app.h"
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_gpio.h>
#include <gui/elements.h>
#include <furi_hal_speaker.h>
#include <subghz/devices/devices.h>

#define TAG "PMRScanApp"

#define PMRSCAN_DEFAULT_RSSI        (-100.0f)
#define PMRSCAN_DEFAULT_SENSITIVITY (-85.0f)

#define PMRSCAN_VOLUME_QUIET        0.0f
#define PMRSCAN_VOLUME_LOUD         1.0f

#define SUBGHZ_DEVICE_NAME			"cc1101_int"

const uint32_t freqs[] = {
    0,
    446006250,
    446018750,
    446031250,
    446043750,
    446056250,
    446068750,
    446081250,
    446093750,
    446106250,
    446118750,
    446131250,
    446143750,
    446156250,
    446168750,
    446181250,
    446193750,
};
const int num_channels = sizeof(freqs) / sizeof(freqs[0]);
const int min_channel = 1;
const int max_channel = num_channels - 1;

static void speaker_mute(PMRScanApp* app) {
    if(app->speaker_acquired && furi_hal_speaker_is_mine()) {
        subghz_devices_set_async_mirror_pin(app->radio_device, NULL);
        furi_hal_speaker_set_volume(PMRSCAN_VOLUME_QUIET);
    }
}

static void speaker_loud(PMRScanApp* app) {
    if(app->speaker_acquired && furi_hal_speaker_is_mine()) {
        subghz_devices_set_async_mirror_pin(app->radio_device, &gpio_speaker);
        furi_hal_speaker_set_volume(PMRSCAN_VOLUME_LOUD);
    }
}

static void pmrscan_draw_callback(Canvas* canvas, void* context) {
    furi_assert(canvas);
    furi_assert(context);
    PMRScanApp* app = (PMRScanApp*)context;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, "PMRScan");

    canvas_set_font(canvas, FontSecondary);
	#define BUFLEN 32
    char buf[BUFLEN + 1] = "";
    snprintf(buf, BUFLEN, "< Chan %ld (%.4f) >", app->freq_channel, (double)app->frequency / 1000000);
    canvas_draw_str_aligned(canvas, 64, 18, AlignCenter, AlignTop, buf);

    snprintf(buf, BUFLEN, "RSSI: %.2f", (double)app->rssi);
    canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignTop, buf);

    snprintf(buf, BUFLEN, "^ Sens: %.2f V", (double)app->sensitivity);
    canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignTop, buf);

    canvas_draw_str_aligned(canvas, 64, 54, AlignCenter, AlignTop, app->scanning ? "Scanning  ()" : "Locked  ()");
}

static void pmrscan_input_callback(InputEvent* input_event, void* context) {
    furi_assert(context);
    FuriMessageQueue* event_queue = context;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
    FURI_LOG_D(TAG, "Exit pmrscan_input_callback");
}

static void pmrscan_rx_callback(const void* data, size_t size, void* context) {
    UNUSED(data);
    UNUSED(context);
    UNUSED(size);
}

static void pmrscan_update_rssi(PMRScanApp* app) {
    furi_assert(app);
    if(app->radio_device) {
        app->rssi = subghz_devices_get_rssi(app->radio_device);
    } else {
        FURI_LOG_E(TAG, "Radio device is NULL");
        app->rssi = PMRSCAN_DEFAULT_RSSI;
    }
}

static bool pmrscan_init_subghz(PMRScanApp* app) {
    furi_assert(app);
    subghz_devices_init();

    const SubGhzDevice* device = subghz_devices_get_by_name(SUBGHZ_DEVICE_NAME);
    if(!device) {
        FURI_LOG_E(TAG, "Failed to get SubGhzDevice");
        return false;
    }
    FURI_LOG_I(TAG, "SubGhzDevice obtained: %s", subghz_devices_get_name(device));

    app->radio_device = device;

    subghz_devices_begin(device);
    subghz_devices_reset(device);
    if(!subghz_devices_is_frequency_valid(device, app->frequency)) {
        FURI_LOG_E(TAG, "Invalid frequency: %lu", app->frequency);
        return false;
    }
    subghz_devices_load_preset(device, FuriHalSubGhzPreset2FSKDev238Async, NULL);
    subghz_devices_set_frequency(device, app->frequency);
    subghz_devices_start_async_rx(device, pmrscan_rx_callback, app);
    if(furi_hal_speaker_acquire(30)) {
        app->speaker_acquired = true;
        speaker_mute(app);
    } else {
        app->speaker_acquired = false;
        FURI_LOG_E(TAG, "Failed to acquire speaker");
    }
    return true;
}

static uint32_t pmrscan_next_channel(PMRScanApp* app) {
    uint32_t channel = app->freq_channel;

    if(app->scan_dir == ScanDirDown)
        channel -= 1;
    else
        channel += 1;

    if(channel < min_channel)
        channel = max_channel;
    else if(channel > max_channel)
        channel = min_channel;

    return channel;
}

static void pmrscan_process_scanning(PMRScanApp* app) {
    furi_assert(app);
    pmrscan_update_rssi(app);
    bool signal_detected = (app->rssi > app->sensitivity);

    if(signal_detected) {
        if(app->scanning) {
            app->scanning = false;
        }
    } else {
        if(!app->scanning) {
            app->scanning = true;
        }
    }

    if(app->scanning) {
        speaker_mute(app);
    } else {
        speaker_loud(app);
    }

    uint32_t new_channel = pmrscan_next_channel(app);
    uint32_t new_frequency = freqs[new_channel];

    if(!subghz_devices_is_frequency_valid(app->radio_device, new_frequency)) {
        new_channel = min_channel;
        new_frequency = freqs[new_channel];
    }

    subghz_devices_flush_rx(app->radio_device);
    subghz_devices_stop_async_rx(app->radio_device);

    subghz_devices_idle(app->radio_device);
    app->freq_channel = new_channel;
    app->frequency = new_frequency;
    subghz_devices_set_frequency(app->radio_device, app->frequency);

    subghz_devices_start_async_rx(app->radio_device, pmrscan_rx_callback, app);
}

PMRScanApp* pmrscan_app_alloc() {
    PMRScanApp* app = malloc(sizeof(PMRScanApp));
    if(!app) {
        FURI_LOG_E(TAG, "Failed to allocate PMRScanApp");
        return NULL;
    }

    app->view_port = view_port_alloc();

    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    app->running = true;
    app->freq_channel = min_channel;
    app->frequency = freqs[app->freq_channel];
    app->rssi = PMRSCAN_DEFAULT_RSSI;
    app->sensitivity = PMRSCAN_DEFAULT_SENSITIVITY;
    app->scanning = true;
    app->scan_dir = ScanDirUp;
    app->speaker_acquired = false;
    app->radio_device = NULL;

    view_port_draw_callback_set(app->view_port, pmrscan_draw_callback, app);

    view_port_input_callback_set(app->view_port, pmrscan_input_callback, app->event_queue);

    app->gui = furi_record_open(RECORD_GUI);

    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    return app;
}

void pmrscan_app_free(PMRScanApp* app) {
    furi_assert(app);
    if(app->speaker_acquired && furi_hal_speaker_is_mine()) {
        subghz_devices_set_async_mirror_pin(app->radio_device, NULL);
        furi_hal_speaker_set_volume(PMRSCAN_VOLUME_LOUD);
        furi_hal_speaker_release();
        app->speaker_acquired = false;
    }

    if(app->radio_device) {
        subghz_devices_flush_rx(app->radio_device);
        subghz_devices_stop_async_rx(app->radio_device);
        subghz_devices_idle(app->radio_device);
        subghz_devices_sleep(app->radio_device);
        subghz_devices_end(app->radio_device);
    }

    subghz_devices_deinit();
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);

    furi_message_queue_free(app->event_queue);

    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t pmrscan_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Enter pmrscan_app");

    PMRScanApp* app = pmrscan_app_alloc();
    if(!app) {
        FURI_LOG_E(TAG, "Failed to allocate app");
        return 1;
    }

    if(!pmrscan_init_subghz(app)) {
        FURI_LOG_E(TAG, "Failed to initialize SubGHz");
        pmrscan_app_free(app);
        return 255;
    }

    InputEvent event;
    while(app->running) {
        if(app->scanning) {
            speaker_mute(app);
            pmrscan_process_scanning(app);
        } else {
            speaker_loud(app);
            pmrscan_update_rssi(app);
        }

        if(furi_message_queue_get(app->event_queue, &event, 10) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                if(event.key == InputKeyOk) {
                    app->scanning = !app->scanning;
                    FURI_LOG_I(TAG, "Toggled scanning: %d", app->scanning);
                } else if(event.key == InputKeyUp) {
                    app->sensitivity += 1.0f;
                    FURI_LOG_I(TAG, "Increased sensitivity: %f", (double)app->sensitivity);
                } else if(event.key == InputKeyDown) {
                    app->sensitivity -= 1.0f;
                    FURI_LOG_I(TAG, "Decreased sensitivity: %f", (double)app->sensitivity);
                } else if(event.key == InputKeyLeft) {
                    app->scan_dir = ScanDirDown;
                    FURI_LOG_I(TAG, "Scan direction set to down");
                } else if(event.key == InputKeyRight) {
                    app->scan_dir = ScanDirUp;
                    FURI_LOG_I(TAG, "Scan direction set to up");
                } else if(event.key == InputKeyBack) {
                    app->running = false;
                    FURI_LOG_I(TAG, "Exiting app");
                }
            }
        }

        view_port_update(app->view_port);
        furi_delay_ms(500);
    }

    pmrscan_app_free(app);
    return 0;
}
