#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <axsdk/axevent.h>
#include <glib-object.h>
#include <glib.h>
#include <syslog.h>
#include <stdlib.h>

// Define a struct to store application data
typedef struct {
    AXEventHandler* event_handler;
    guint event_id;
    gdouble value;
} AppData;

// Function declarations

/**
 * @brief Create an event declaration and initialize event handling.
 *
 * @return int Returns 1 on success, 0 on failure.
 */
AppData* create_event(void);

/**
 * @brief Send an event using the provided application data.
 *
 * @param send_data Pointer to application data containing event details.
 * @return gboolean TRUE to keep the timer running, FALSE otherwise.
 */
gboolean send_event(AppData* send_data);

/**
 * @brief Perform cleanup operations for the event handling system.
 */
void event_cleanup(void);

#ifdef __cplusplus
}
#endif