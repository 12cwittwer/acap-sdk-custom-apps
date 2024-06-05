#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <syslog.h>
#include <stdint.h>
#include <sqlite3.h>
#include <time.h>
#include <inttypes.h>
#include <axsdk/axparameter.h>
#include <glib.h>

#define LOCAL_PATH "/var/spool/storage/areas/SD_DISK/speedmonitor/"
#define APP_NAME "httpsUpload"

// Function prototypes
static void upload_recent_entries(const char *db_path, const char *endpoint, const char *auth, const int days);
static int extract_recent_entries(const char *db_path, char **json_data, const int days);
__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void panic(const char *format, ...);

__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void panic(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

static int extract_recent_entries(const char *db_path, char **json_data, const int days) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        syslog(LOG_ERR, "Cannot open database: %s", sqlite3_errmsg(db));
        return rc;
    }

    // Calculate the timestamp for variable days ago
    time_t now = time(NULL);
    time_t calc_time_ago = now - (days * 24 * 60 * 60);
    calc_time_ago *= 1000000; // Converts to microseconds for comparison
    syslog(LOG_INFO, "Calculated Timestamp: %ld", (long)calc_time_ago);
    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT internal_id, track_id, profile_id, profile_trigger_id, classification, start_timestamp, duration, min_speed, max_speed, avg_speed, enter_speed, exit_speed, enter_bearing, exit_bearing, flags FROM track WHERE start_timestamp >= %ld ORDER BY start_timestamp DESC", calc_time_ago);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        syslog(LOG_ERR, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return rc;
    }

    // Allocate initial memory for JSON data
    size_t json_size = 4096;
    *json_data = malloc(json_size);
    if (*json_data == NULL) {
        syslog(LOG_ERR, "Failed to allocate memory for JSON data");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    // Start the JSON array
    strcpy(*json_data, "{\"entries\":[");
    size_t offset = strlen(*json_data);

    int first_entry = 1;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (!first_entry) {
            strcat(*json_data, ",");
            offset += 1;
        }
        first_entry = 0;

        int internal_id = sqlite3_column_int(stmt, 0);
        int track_id = sqlite3_column_int(stmt, 1);
        int profile_id = sqlite3_column_int(stmt, 2);
        int profile_trigger_id = sqlite3_column_int(stmt, 3);
        int classification = sqlite3_column_int(stmt, 4);
        int64_t start_timestamp = sqlite3_column_int64(stmt, 5);
        int duration = sqlite3_column_int(stmt, 6);
        int min_speed = sqlite3_column_int(stmt, 7);
        int max_speed = sqlite3_column_int(stmt, 8);
        int avg_speed = sqlite3_column_int(stmt, 9);
        int enter_speed = sqlite3_column_int(stmt, 10);
        int exit_speed = sqlite3_column_int(stmt, 11);
        int enter_bearing = sqlite3_column_int(stmt, 12);
        int exit_bearing = sqlite3_column_int(stmt, 13);
        int flags = sqlite3_column_int(stmt, 14);

        size_t entry_size = snprintf(NULL, 0, "{\"internal_id\":%d,\"track_id\":%d,\"profile_id\":%d,\"profile_trigger_id\":%d,\"classification\":%d,\"start_timestamp\":%" PRId64 ",\"duration\":%d,\"min_speed\":%d,\"max_speed\":%d,\"avg_speed\":%d,\"enter_speed\":%d,\"exit_speed\":%d,\"enter_bearing\":%d,\"exit_bearing\":%d,\"flags\":%d}", internal_id, track_id, profile_id, profile_trigger_id, classification, start_timestamp, duration, min_speed, max_speed, avg_speed, enter_speed, exit_speed, enter_bearing, exit_bearing, flags) + 1;
        if (offset + entry_size > json_size) {
            json_size *= 2;
            *json_data = realloc(*json_data, json_size);
            if (*json_data == NULL) {
                syslog(LOG_ERR, "Failed to reallocate memory for JSON data");
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return 1;
            }
        }

        offset += snprintf(*json_data + offset, json_size - offset, "{\"internal_id\":%d,\"track_id\":%d,\"profile_id\":%d,\"profile_trigger_id\":%d,\"classification\":%d,\"start_timestamp\":%" PRId64 ",\"duration\":%d,\"min_speed\":%d,\"max_speed\":%d,\"avg_speed\":%d,\"enter_speed\":%d,\"exit_speed\":%d,\"enter_bearing\":%d,\"exit_bearing\":%d,\"flags\":%d}", internal_id, track_id, profile_id, profile_trigger_id, classification, start_timestamp, duration, min_speed, max_speed, avg_speed, enter_speed, exit_speed, enter_bearing, exit_bearing, flags);
    }

    // End the JSON array
    strcat(*json_data, "]}");

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (rc != SQLITE_DONE) {
        syslog(LOG_ERR, "Failed to read data: %s", sqlite3_errmsg(db));
        return rc;
    }

    return 0;
}

static void upload_recent_entries(const char *db_path, const char *endpoint, const char *auth, const int days) {
    char *json_data = NULL;

    if (extract_recent_entries(db_path, &json_data, days) != 0) {
        syslog(LOG_ERR, "Failed to extract recent entries");
        return;
    }

    CURL *curl;
    CURLcode res;
    char error_buffer[CURL_ERROR_SIZE];

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        char auth_header[256];
        if ((size_t)snprintf(auth_header, sizeof(auth_header), "PARKSPLUS_AUTH: %.240s", auth) >= sizeof(auth_header)) {
            syslog(LOG_ERR, "Auth header truncated");
            free(json_data);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return;
        }
        headers = curl_slist_append(headers, auth_header);

        curl_easy_setopt(curl, CURLOPT_URL, endpoint);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);       // Limit the number of redirects to follow

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            syslog(LOG_ERR, "curl_easy_perform() failed: %s", error_buffer);
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            syslog(LOG_INFO, "HTTP response code: %ld", http_code);
            if (http_code == 200) {
                syslog(LOG_INFO, "Data uploaded successfully");
            } else {
                syslog(LOG_ERR, "Data upload failed, server response code: %ld", http_code);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        syslog(LOG_ERR, "Failed to initialize CURL");
    }

    free(json_data);
    curl_global_cleanup();
}

int main(void) {
    GError *error = NULL;

    // Open syslog
    openlog(APP_NAME, LOG_PID | LOG_CONS, LOG_USER);

    syslog(LOG_INFO, "Starting FTP Upload App");

    // Load configuration parameters
    char endpoint[256];
    char auth[256];
    int interval;
    int days;

    AXParameter *handle = ax_parameter_new(APP_NAME, &error);

    if (handle) {
        gchar *param_value = NULL;

        if (ax_parameter_get(handle, "ENDPOINT", &param_value, &error)) {
            strncpy(endpoint, param_value, sizeof(endpoint) - 1);
            endpoint[sizeof(endpoint) - 1] = '\0';
            g_free(param_value);
            syslog(LOG_INFO, "Successfully Retrieved ENDPOINT");
        } else {
            panic("Failed to get ENDPOINT: %s", error->message);
        }

        if (ax_parameter_get(handle, "AUTH", &param_value, &error)) {
            strncpy(auth, param_value, sizeof(auth) - 1);
            auth[sizeof(auth) - 1] = '\0';
            g_free(param_value);
            syslog(LOG_INFO, "Successfully Retrieved AUTH");
        } else {
            panic("Failed to get AUTH: %s", error->message);
        }

        if (ax_parameter_get(handle, "INTERVAL", &param_value, &error)) {
            interval = atoi(param_value);
            g_free(param_value);
            syslog(LOG_INFO, "Successfully Retrieved INTERVAL");
        } else {
            panic("Failed to get INTERVAL: %s", error->message);
        }

        if (ax_parameter_get(handle, "DAYS", &param_value, &error)) {
            days = atoi(param_value);
            g_free(param_value);
            syslog(LOG_INFO, "Successfully Retrieved DAYS");
        } else {
            panic("Failed to get Days: %s", error->message);
        }

        ax_parameter_free(handle);
    } else {
        panic("Failed to create AXParameter: %s", error->message);
    }

    syslog(LOG_INFO, "Entering upload loop");

    while (1) {
        syslog(LOG_DEBUG, "Starting data upload cycle");
        const char *db_path = LOCAL_PATH "statistics.db";
        upload_recent_entries(db_path, endpoint, auth, days);
        syslog(LOG_DEBUG, "Sleeping for %d seconds", interval);
        sleep(interval);
    }

    syslog(LOG_INFO, "Stopping FTP Upload App");

    // Close syslog
    closelog();

    return 0;
}
