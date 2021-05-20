/**
 * Embedded Planet Telit OneEdge Example
 *
 * Built with ARM Mbed-OS
 *
 * Copyright (c) 2021 Embedded Planet, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "mbed.h"

#include "mbed_trace.h"
#include "CellularDevice.h"
#include "one_edge.h"
#include "Si7021.h"

#define TRACE_GROUP   "MAIN"
#define ONEEDGE_CLIENT_STATE_MAX_LENGTH 20

// Global pointers
CellularDevice *dev;
ATHandler *at_handler;
I2C i2c(PIN_NAME_SDA, PIN_NAME_SCL);
Si7021 si7021(i2c);
DigitalOut sensor_power_enable(PIN_NAME_SENSOR_POWER_ENABLE);

/**
 * OneEdge LWM2M client enabling status
 */
enum ClientEnablingStatus {
    ClientDisabled = 0, // The client is disabled
    ClientEnabled = 1   // The client is enabled
};

/**
 * OneEdge LWM2M client internal status
 */
enum ClientInternalStatus {
    Disabled,       // The client is disabled
    Waiting,        // Waiting for the user's ACK
    Active,         // After the ACK, the session is currently active
    Idle,           // There is not an active session currently
    Deregistering,  // The client is deregistering
    Unknown         // Current status unknown
};

/**
 * OneEdge LWM2M client status
 */
struct ClientStatus {
    ClientEnablingStatus enabled_status;
    ClientInternalStatus internal_status;
};

ClientStatus get_client_status()
{
    ClientStatus current_status {
        .enabled_status = ClientDisabled,
        .internal_status = Unknown
    };

    at_handler->lock();

    at_handler->cmd_start_stop("#LWM2MSTAT", "");
    at_handler->resp_start("#LWM2MGETSTAT:");

    int current_enabled_status = -1;
    char current_internal_status[ONEEDGE_CLIENT_STATE_MAX_LENGTH];

    current_enabled_status = at_handler->read_int();
    at_handler->read_string(current_internal_status, sizeof(current_internal_status));

    at_handler->resp_stop();
    at_handler->unlock();

    // Populate client status struct
    switch (current_enabled_status) {
        default:
        case 0:
            current_status.enabled_status = ClientDisabled;
            tr_debug("LWM2M client enabling status: Disabled");
            break;
        case 1:
            current_status.enabled_status = ClientEnabled;
            tr_debug("LWM2M client enabling status: Enabled");
            break;
    }

    if (strstr(current_internal_status, "DIS") != NULL) {
        current_status.internal_status = Disabled;
        tr_debug("LWM2M client internal status: Disabled");
    } else if (strstr(current_internal_status, "WAIT") != NULL) {
        current_status.internal_status = Waiting;
        tr_debug("LWM2M client internal status: Waiting");
    } else if (strstr(current_internal_status, "ACTIVE") != NULL) {
        current_status.internal_status = Active;
        tr_debug("LWM2M client internal status: Active");
    } else if (strstr(current_internal_status, "IDLE") != NULL) {
        current_status.internal_status = Idle;
        tr_debug("LWM2M client internal status: Idle");
    } else if (strstr(current_internal_status, "DEREG") != NULL) {
        current_status.internal_status = Deregistering;
        tr_debug("LWM2M client internal status: Deregistering");
    } else {
        current_status.internal_status = Unknown;
        tr_debug("LWM2M client internal status: Unknown");
    }

    return current_status;
}

void set_battery_level(int battery_level)
{
    tr_info("Setting the battery level resource to %d", battery_level);
    at_handler->lock();
    at_handler->at_cmd_discard("#LWM2MSET", "=", "%d%d%d%d%d%d", 0, 3, 0, 9, 0, battery_level);
    at_handler->unlock();
}

bool file_exists(char *target_file)
{
    at_handler->lock();

    at_handler->cmd_start_stop("#M2MLIST", "=/XML");
    at_handler->resp_start("#M2MLIST:");

    while (at_handler->info_resp()) {
        char m2mlist_entry[ONEEDGE_MAX_FULL_FILE_PATH_LENGTH];
        at_handler->read_string(m2mlist_entry, sizeof(m2mlist_entry));
        if (strstr(m2mlist_entry, target_file) != NULL) {
            at_handler->resp_stop();
            at_handler->unlock();
            return true;
        }
    }

    at_handler->resp_stop();
    at_handler->unlock();
    return false; 
}

bool enable_temperature_object()
{
    // Check if the object description file already exists on the modem
    if (file_exists("object_3303.xml")) {
        tr_debug("'object_3303.xml' file found!");
        return true;
    }

    at_handler->lock();

    int write_size = 0;

    // Write the file to the modem
    at_handler->cmd_start_stop("#M2MWRITE", "=", "%s%d", "/XML/object_3303.xml", strlen(get_object_3303()));
    at_handler->resp_start(">>>", true);

    if (at_handler->get_last_error() != NSAPI_ERROR_OK) {
        tr_warn("Unable to send file");
        at_handler->unlock();
        return false;
    }

    write_size = at_handler->write_bytes((uint8_t *)get_object_3303(), strlen(get_object_3303()));
    if (write_size < strlen(get_object_3303())) {
        tr_warn("Unable to send full object_3303.xml file");
        at_handler->unlock();
        return false;
    }
    at_handler->resp_start("\r\nOK", true);
    at_handler->resp_stop();

    if (at_handler->get_last_error() != NSAPI_ERROR_OK) {
        tr_warn("Error sending object_3303.xml file");
        at_handler->unlock();
        return false;
    }

    tr_debug("object_3303.xml file sent");

    // Now that the file has been sent, we need to trigger a module reboot
    at_handler->at_cmd_discard("#REBOOT", "");
    at_handler->unlock();

    // Wait for the module to reboot
    ThisThread::sleep_for(10s);

    // Reset the MCU
    tr_info("Resetting to have the new settings take effect");
    NVIC_SystemReset();

    return true;
}

bool create_temperature_object_instance(int instance = 0)
{
    at_handler->lock();
    
    // Read the resource first to see if it already exists
    at_handler->at_cmd_discard("#LWM2MR", "=", "%d%d%d%d%d",
                0,          // Telit instance
                3303,       // Temperature object
                instance,   // Object instance
                5700,       // Current value resource ID
                0);         // Resource instance ID
    if (at_handler->get_last_error() == NSAPI_ERROR_OK) {
        // Resource already exists
        at_handler->unlock();
        return true;
    }

    at_handler->clear_error();
    at_handler->flush();
    at_handler->at_cmd_discard("#LWM2MNEWINST", "=", "%d%d%d", 0, 3303, instance);

    return at_handler->unlock_return_error() == NSAPI_ERROR_OK;
}

void set_temperature(float temperature)
{
    char temperature_string[MAX_TEMP_LENGTH];

    snprintf(temperature_string, MAX_TEMP_LENGTH, "%0.2f", temperature);
    tr_info("Setting the temperature resource to %0.2f", temperature);
    at_handler->lock();

    at_handler->set_at_timeout(LWM2MSET_AT_TIMEOUT);
    at_handler->cmd_start("AT#LWM2MSET=");
    at_handler->write_int(LWM2MSET_FLOAT_TYPE);             // Float type
    at_handler->write_int(TEMPERATURE_OBJECT_ID);           // Temperature object ID
    at_handler->write_int(0);                               // Object instance
    at_handler->write_int(SENSOR_VALUE_RESOURCE_ID);        // Resource ID
    at_handler->write_int(0);                               // Resource instance (0)
    at_handler->write_string(temperature_string, false);    // New value
    at_handler->cmd_stop_read_resp();
    at_handler->restore_at_timeout();

    at_handler->unlock();
}

int main()
{
    // Initialize mbed trace
    mbed_trace_init();

#if !MBED_CONF_CELLULAR_DEBUG_AT
    mbed_trace_exclude_filters_set("CELL");
#endif

    tr_info("************************************************");
    tr_info("* Embedded Planet Telit OneEdge Example v0.2.0 *");
    tr_info("************************************************");

    // Make sure we're running on a compatible EP target
#ifndef TARGET_EP_AGORA
    tr_error("This example must be run from a compatible EP target!");
    while (1) {
        ThisThread::sleep_for(10ms);
    }
#endif

    // Get pointers to the device and AT handler
    dev = CellularDevice::get_target_default_instance();
    at_handler = dev->get_at_handler();

    // Check if the device is ready
    tr_info("Bringing the cell module online");
    if (dev->is_ready() != NSAPI_ERROR_OK) {
        dev->hard_power_on();
        ThisThread::sleep_for(250ms);
        dev->soft_power_on();
        ThisThread::sleep_for(10s);
        dev->init();
    }

    nsapi_error_t err = NSAPI_ERROR_OK;

    // Enable the Telit OneEdge LWM2M client
    at_handler->lock();
    err = at_handler->at_cmd_discard("#LWM2MENA", "=1");
    if (err != NSAPI_ERROR_OK) {
        tr_warn("Unable to enable the Telit OneEdge LWM2M client");
    }
    at_handler->unlock();

    // Setup the temperature object instance
    if (!enable_temperature_object()) {
        tr_error("Unable to enable temperature object!");
        while (1);
    }

    // Create an instance of the temperature object
    if (!create_temperature_object_instance()) {
        tr_error("Unable to create an instance of the temperature object!");
        while (1);
    }

    // Wait until the client is registered
    bool registered = false;
    while (!registered) {
        // Get the current status of the client
        ClientStatus current_status = get_client_status();

        switch (current_status.internal_status) {
            case Disabled:
                // Try to re-enable the client
                at_handler->lock();
                err = at_handler->at_cmd_discard("#LWM2MENA", "=1");
                if (err != NSAPI_ERROR_OK) {
                    tr_warn("Unable to enable the Telit OneEdge LWM2M client");
                }
                at_handler->unlock();
                break;
            case Waiting:
                // Client is in the waiting state
                break;
            case Active:
                // Client is active, so we're done
                registered = true;
                break;
            case Idle:
                // Client is idle, so we're done
                registered = true;
                break;
            case Unknown:
                break;
            default:
            case Deregistering:
                break;
        }

        // Sleep for 5s
        ThisThread::sleep_for(5000);
    }

    tr_info("Client registered");

    // Enable power to the sensors
    tr_info("Enabling power to the sensors");
    sensor_power_enable = 1;
    ThisThread::sleep_for(500ms);

    // Make sure the SI7021 is online
    bool si7021_online = false;
    if (si7021.check()) {
        tr_info("SI7021 online");
        si7021_online = true;
    } else {
        tr_error("SI7021 offline!");
        si7021_online = false;
    }

    int battery_level = 100;

    while (1) {
        // Update the battery level resource
        set_battery_level(battery_level);

        // Decrement the battery level for the next loop
        battery_level--;
        if (battery_level < 0) {
            battery_level = 100;
        }

        // Handle reading the temperature
        if (si7021_online) {
            // Perform a measurement
            si7021.measure();

            // Update the temperature resource
            set_temperature(si7021.get_temperature() / 1000.0);
        }

        // Sleep for 30 seconds
        ThisThread::sleep_for(30s);
    }
}