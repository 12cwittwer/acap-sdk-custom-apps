/**
 * Copyright (C) 2021, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 Modified by Christian Wittwer
 */

/**
 * - send_event.c -
 *
 * This example illustrates how to send a stateful ONVIF event, which is
 * changing the value every 10th second.
 *
 * Error handling has been omitted for the sake of brevity.
 */
#include <axsdk/axevent.h>
#include <glib-object.h>
#include <glib.h>
#include <string.h>
#include <syslog.h>

#include "send_event.h"

static AppData* app_data = NULL;

/**
 * brief Send event.
 *
 * Send the previously declared event.
 *
 * param send_data Application data containing e.g. the event declaration id.
 * return TRUE
 */
gboolean send_event(AppData* send_data) {
    AXEventKeyValueSet* key_value_set = NULL;
    AXEvent* event                    = NULL;

    syslog(LOG_INFO, "Sending event Event Handler: %p with ID: %u and Value: %d", send_data->event_handler, send_data->event_id, send_data->value);

    key_value_set = ax_event_key_value_set_new();

    // Add the variable elements of the event to the set
    if (!ax_event_key_value_set_add_key_value(key_value_set,
                                         "SuccessValue",
                                         "tnsaxis",
                                         &send_data->value,
                                         AX_VALUE_TYPE_INT,
                                         NULL)) {
        syslog(LOG_ERR, "Key value set was not made");
    }

    // Create the event
    // Use ax_event_new2 since ax_event_new is deprecated from 3.2
    event = ax_event_new2(key_value_set, NULL);
    if (!event) {
        syslog(LOG_ERR, "New event failed to create");
    }

    // The key/value set is no longer needed
    ax_event_key_value_set_free(key_value_set);

    // Send the event
    if (!ax_event_handler_send_event(send_data->event_handler, send_data->event_id, event, NULL)) {
        syslog(LOG_ERR, "Failed to send event to handler");
    }

    ax_event_free(event);

    // Returning TRUE keeps the timer going
    return TRUE;
}

/**
 * brief Callback function which is called when event declaration is completed.
 *
 * This callback will be called when the declaration has been registered
 * with the event system. The event declaration can now be used to send
 * events.
 *
 * param declaration Event declaration id.
 * param value Start value of the event.
 */
static void declaration_complete(guint declaration, gint* value) {
    syslog(LOG_INFO, "Declaration complete for: %d", declaration);

    (void)value;
    // app_data->value = *value;

    // Set up a timer to be called every 10th second
    // app_data->timer = g_timeout_add_seconds(10, (GSourceFunc)send_event, app_data);
}

/**
 * brief Setup a declaration of an event.
 *
 * Declare a stateful ONVIF event that looks like this,
 * which is using ONVIF namespace "tns1".
 *
 * Topic: tns1:Monitoring/ProcessorUsage
 * <tt:MessageDescription IsProperty="true">
 *  <tt:Source>
 *   <tt:SimpleItemDescription Name=”Token” Type=”tt:ReferenceToken”/>
 *  </tt:Source>
 *  <tt:Data>
 *   <tt:SimpleItemDescription Name="Value" Type="xs:float"/>
 *  </tt:Data>
 * </tt:MessageDescription>
 *
 * Value = 0 <-- The initial value will be set to 0.0
 *
 * param event_handler Event handler.
 * return declaration id as integer.
 */
static guint setup_declaration(AXEventHandler* event_handler, guint* start_value) {
    AXEventKeyValueSet* key_value_set = NULL;
    guint declaration                 = 0;
    guint token                       = 0;
    GError* error                     = NULL;

    // Create keys, namespaces and nice names for the event
    key_value_set = ax_event_key_value_set_new();

    syslog(LOG_INFO, "Key Value Set Created at %p", key_value_set);

    if(!ax_event_key_value_set_add_key_value(key_value_set, "topic0", "tnsaxis",
                                        "CameraApplicationPlatform", AX_VALUE_TYPE_STRING, NULL)) {
                                            syslog(LOG_ERR, "Failed to add a key value set");
                                        }
    if(!ax_event_key_value_set_add_key_value(key_value_set, "topic1", "tnsaxis", "BarcodeScanned",
                                        AX_VALUE_TYPE_STRING, NULL)) {
                                            syslog(LOG_ERR, "Failed to add a key value set");
                                        }
    if (!ax_event_key_value_set_add_key_value(key_value_set, "Token", "tnsaxis", &token,
                                        AX_VALUE_TYPE_INT, NULL)) {
                                            syslog(LOG_ERR, "Failed to add a key value set");
                                        }
    if (!ax_event_key_value_set_add_key_value(key_value_set, "SuccessValue", "tnsaxis", start_value,
                                        AX_VALUE_TYPE_INT, NULL)) {
                                            syslog(LOG_ERR, "Failed to add a key value set");
                                        }
    if (!ax_event_key_value_set_mark_as_source(key_value_set, "Token", "tnsaxis", NULL)) {
        syslog(LOG_ERR, "Failed to mark as source");
    }
    if (!ax_event_key_value_set_mark_as_user_defined(key_value_set,
                                                    "Token",
                                                    "tnsaxis",
                                                    "wstype:tt:ReferenceToken",
                                                    NULL)) {
        syslog(LOG_ERR, "Failed to mark as user defined");
    }
    if (!ax_event_key_value_set_mark_as_data(key_value_set, "SuccessValue", "tnsaxis", NULL)) {
        syslog(LOG_ERR, "failed to mark as data");
    }
    if (!ax_event_key_value_set_mark_as_user_defined(key_value_set,
                                                    "SuccessValue",
                                                    "tnsaxis",
                                                    "wstype:xs:float",
                                                    NULL)) {
        syslog(LOG_ERR, "Failed to mark as user defined");
    }

    // Declare event
    if (!ax_event_handler_declare(event_handler,
                                  key_value_set,
                                  FALSE,  // Indicate a property state event
                                  &declaration,
                                  (AXDeclarationCompleteCallback)declaration_complete,
                                  start_value,
                                  &error)) {
        syslog(LOG_WARNING, "Could not declare: %s", error->message);
        g_error_free(error);
    }

    // The key/value set is no longer needed
    ax_event_key_value_set_free(key_value_set);
    return declaration;
}

void event_cleanup() {
    // Cleanup event handler
    syslog(LOG_INFO, "Cleaning Up Event");
    ax_event_handler_undeclare(app_data->event_handler, app_data->event_id, NULL);
    ax_event_handler_free(app_data->event_handler);
    free(app_data);
}

/**
 * brief Main function which sends an event.
 */
AppData* create_event(void) {
    guint start_value  = 0;

    app_data = calloc(1, sizeof(AppData));
    if (!app_data) {
        syslog(LOG_ERR, "Failed to allocate AppData");
        return NULL;
    }

    // Event handler
    app_data->event_handler = ax_event_handler_new();
    if (!app_data->event_handler) {
        syslog(LOG_ERR, "New Event Handler was not Created");
        event_cleanup();
        return NULL;
    }
    syslog(LOG_INFO, "Event handler created at: %p", app_data->event_handler);
    app_data->event_id = setup_declaration(app_data->event_handler, &start_value);
    if (!app_data->event_id) {
        syslog(LOG_ERR, "New Event Failed to Create");
        event_cleanup();
        return NULL;
    }
    syslog(LOG_INFO, "Event ID %d Saved to App Data", app_data->event_id);

    return app_data;
}
