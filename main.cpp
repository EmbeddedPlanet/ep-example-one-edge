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

#define TRACE_GROUP   "MAIN"
#define ONEEDGE_CLIENT_STATE_MAX_LENGTH 20

// Global pointers
CellularDevice *dev;
ATHandler *at_handler;

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

int main()
{
    // Initialize mbed trace
    mbed_trace_init();

#if !MBED_CONF_CELLULAR_DEBUG_AT
    mbed_trace_exclude_filters_set("CELL");
#endif

    tr_info("************************************************");
    tr_info("* Embedded Planet Telit OneEdge Example v0.1.0 *");
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

    int battery_level = 100;

    while (1) {
        // Update the battery level resource
        set_battery_level(battery_level);

        // Decrement the battery level for the next loop
        battery_level--;
        if (battery_level < 0) {
            battery_level = 100;
        }

        // Sleep for 30 seconds
        ThisThread::sleep_for(30s);
    }
}