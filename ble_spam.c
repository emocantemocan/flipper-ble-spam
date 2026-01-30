/*
 * BLE Spam Custom - Flipper Zero Application
 * Спам BLE устройствами (AirPods, Samsung, Windows и т.д.)
 */

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>
#include <notification/notification_messages.h>

#define TAG "BLESpam"

typedef enum {
    DeviceTypeAll,
    DeviceTypeAirPods,
    DeviceTypeAirPodsMax,
    DeviceTypeSamsung,
    DeviceTypeWindows,
    DeviceTypeCount
} DeviceType;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Widget* widget;
    NotificationApp* notifications;
    DeviceType selected_device;
    bool is_spamming;
    FuriThread* spam_thread;
} BLESpamApp;

typedef enum {
    BLESpamViewSubmenu,
    BLESpamViewWidget,
} BLESpamView;

static const uint8_t airpods_packet[] = {
    0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x01, 0x02, 0x20, 0x75,
    0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t airpods_max_packet[] = {
    0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x01, 0x0E, 0x20, 0x75,
    0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t samsung_packet[] = {
    0x1B, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01,
    0xFF, 0x00, 0x00, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t windows_packet[] = {
    0x1E, 0xFF, 0x06, 0x00, 0x01, 0x09, 0x20, 0x00, 0x03, 0x00,
    0x01, 0x80, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char* device_names[] = {
    "ВСЕ УСТРОЙСТВА",
    "AirPods",
    "AirPods Max",
    "Samsung Buds",
    "Windows Swift Pair"
};

static int32_t spam_thread_func(void* context) {
    BLESpamApp* app = context;
    FURI_LOG_I(TAG, "Starting BLE spam for device type %d", app->selected_device);
    
    uint8_t packet_index = 0;
    const uint8_t* packets[] = {airpods_packet, airpods_max_packet, samsung_packet, windows_packet};
    const size_t packet_sizes[] = {sizeof(airpods_packet), sizeof(airpods_max_packet), sizeof(samsung_packet), sizeof(windows_packet)};
    
    while(app->is_spamming) {
        if(app->selected_device == DeviceTypeAll) {
            // Спам всеми устройствами по очереди
            const uint8_t* packet = packets[packet_index];
            size_t packet_size = packet_sizes[packet_index];
            
            // Здесь отправка BLE пакета
            // furi_hal_bt_tx(packet, packet_size);
            
            notification_message(app->notifications, &sequence_blink_blue_10);
            
            packet_index = (packet_index + 1) % 4;
            furi_delay_ms(500); // 2 пакета в секунду
        } else {
            // Спам одним устройством
            const uint8_t* packet = NULL;
            size_t packet_size = 0;
            
            switch(app->selected_device) {
                case DeviceTypeAirPods:
                    packet = airpods_packet;
                    packet_size = sizeof(airpods_packet);
                    break;
                case DeviceTypeAirPodsMax:
                    packet = airpods_max_packet;
                    packet_size = sizeof(airpods_max_packet);
                    break;
                case DeviceTypeSamsung:
                    packet = samsung_packet;
                    packet_size = sizeof(samsung_packet);
                    break;
                case DeviceTypeWindows:
                    packet = windows_packet;
                    packet_size = sizeof(windows_packet);
                    break;
                default:
                    break;
            }
            
            if(packet) {
                // Здесь отправка BLE пакета
                // furi_hal_bt_tx(packet, packet_size);
                notification_message(app->notifications, &sequence_blink_blue_10);
            }
            
            furi_delay_ms(500); // 2 пакета в секунду
        }
    }
    
    FURI_LOG_I(TAG, "BLE spam stopped");
    return 0;
}

static void submenu_callback(void* context, uint32_t index) {
    BLESpamApp* app = context;
    app->selected_device = index;
    view_dispatcher_switch_to_view(app->view_dispatcher, BLESpamViewWidget);
    
    app->is_spamming = true;
    app->spam_thread = furi_thread_alloc();
    furi_thread_set_name(app->spam_thread, "BLESpamThread");
    furi_thread_set_stack_size(app->spam_thread, 2048);
    furi_thread_set_context(app->spam_thread, app);
    furi_thread_set_callback(app->spam_thread, spam_thread_func);
    furi_thread_start(app->spam_thread);
    
    widget_reset(app->widget);
    FuriString* text = furi_string_alloc();
    furi_string_printf(text, "Спам активен:\n%s\n\nНажмите Back\nдля остановки", device_names[index]);
    widget_add_text_scroll_element(app->widget, 0, 0, 128, 64, furi_string_get_cstr(text));
    furi_string_free(text);
}

static bool back_event_callback(void* context) {
    BLESpamApp* app = context;
    if(app->is_spamming) {
        app->is_spamming = false;
        if(app->spam_thread) {
            furi_thread_join(app->spam_thread);
            furi_thread_free(app->spam_thread);
            app->spam_thread = NULL;
        }
    }
    return true;
}

static BLESpamApp* ble_spam_app_alloc() {
    BLESpamApp* app = malloc(sizeof(BLESpamApp));
    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    
    app->submenu = submenu_alloc();
    submenu_add_item(app->submenu, "ВСЕ УСТРОЙСТВА", DeviceTypeAll, submenu_callback, app);
    submenu_add_item(app->submenu, "AirPods", DeviceTypeAirPods, submenu_callback, app);
    submenu_add_item(app->submenu, "AirPods Max", DeviceTypeAirPodsMax, submenu_callback, app);
    submenu_add_item(app->submenu, "Samsung Buds", DeviceTypeSamsung, submenu_callback, app);
    submenu_add_item(app->submenu, "Windows Swift", DeviceTypeWindows, submenu_callback, app);
    view_dispatcher_add_view(app->view_dispatcher, BLESpamViewSubmenu, submenu_get_view(app->submenu));
    
    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BLESpamViewWidget, widget_get_view(app->widget));
    
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, back_event_callback);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BLESpamViewSubmenu);
    
    app->is_spamming = false;
    app->spam_thread = NULL;
    return app;
}

static void ble_spam_app_free(BLESpamApp* app) {
    if(app->is_spamming) {
        app->is_spamming = false;
        if(app->spam_thread) {
            furi_thread_join(app->spam_thread);
            furi_thread_free(app->spam_thread);
        }
    }
    
    view_dispatcher_remove_view(app->view_dispatcher, BLESpamViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, BLESpamViewWidget);
    submenu_free(app->submenu);
    widget_free(app->widget);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t ble_spam_app(void* p) {
    UNUSED(p);
    BLESpamApp* app = ble_spam_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    ble_spam_app_free(app);
    return 0;
}
