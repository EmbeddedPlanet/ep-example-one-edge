/**
 * Embedded Planet Telit OneEdge Example
 * oneedge.h
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
#ifndef ONE_EDGE_H
#define ONE_EDGE_H

#include "mbed.h"

#define ONEEDGE_MAX_FULL_FILE_PATH_LENGTH   128
#define MAX_TEMP_LENGTH                     10
#define LWM2MSET_AT_TIMEOUT                 5 * 1000 // 5 seconds
#define LWM2MSET_FLOAT_TYPE                 1
#define TEMPERATURE_OBJECT_ID               3303
#define SENSOR_VALUE_RESOURCE_ID            5700

/** 
 * Retrieves static pointer to the contents of the temperature object's (3303) XML description
 * 
 *  @return             pointer to file contents
 */
const char *get_object_3303()
{
    static char object_3303[] = {
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<LWM2M  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openmobilealliance.org/tech/profiles/LWM2M.xsd\">"
            "<Object ObjectType=\"MODefinition\">"
                "<Name>Temperature</Name>"
                "<Description1>Description: This IPSO object should be used with a temperature sensor to report a temperature measurement.  It also provides resources for minimum/maximum measured values and the minimum/maximum range that can be measured by the temperature sensor. An example measurement unit is degrees Celsius (ucum:Cel).</Description1>"
                "<ObjectID>3303</ObjectID>"
                "<ObjectURN>urn:oma:lwm2m:ext:3303</ObjectURN>"
                "<MultipleInstances>Multiple</MultipleInstances>"
                "<Mandatory>Optional</Mandatory>"
                "<Resources>"
                "<Item ID=\"5700\">"
                    "<Name>Sensor Value</Name>"
                    "<Operations>R</Operations>"
                    "<MultipleInstances>Single</MultipleInstances>"
                    "<Mandatory>Mandatory</Mandatory>"
                    "<Type>Float</Type>"
                    "<RangeEnumeration>"
                    "</RangeEnumeration>"
                    "<Units>Defined by \"Units\" resource.</Units>"
                    "<Description>Last or Current Measured Value from the Sensor</Description>"
                "</Item>"
                "<Item ID=\"5601\">"
                    "<Name>Min Measured Value</Name>"
                    "<Operations>R</Operations>"
                    "<MultipleInstances>Single</MultipleInstances>"
                    "<Mandatory>Optional</Mandatory>"
                    "<Type>Float</Type>"
                    "<RangeEnumeration>"
                    "</RangeEnumeration>"
                    "<Units>Defined by \"Units\" resource.</Units>"
                    "<Description>The minimum value measured by the sensor since power ON or reset</Description>"
                "</Item>"
                "<Item ID=\"5602\">"
                    "<Name>Max Measured Value</Name>"
                    "<Operations>R</Operations>"
                    "<MultipleInstances>Single</MultipleInstances>"
                    "<Mandatory>Optional</Mandatory>"
                    "<Type>Float</Type>"
                    "<RangeEnumeration>"
                    "</RangeEnumeration>"
                    "<Units>Defined by \"Units\" resource.</Units>"
                    "<Description>The maximum value measured by the sensor since power ON or reset</Description>"
                "</Item>"
                "<Item ID=\"5603\">"
                    "<Name>Min Range Value</Name>"
                    "<Operations>R</Operations>"
                    "<MultipleInstances>Single</MultipleInstances>"
                    "<Mandatory>Optional</Mandatory>"
                    "<Type>Float</Type>"
                    "<RangeEnumeration>"
                    "</RangeEnumeration>"
                    "<Units>Defined by \"Units\" resource.</Units>"
                    "<Description>The minimum value that can be measured by the sensor</Description>"
                "</Item>"
                "<Item ID=\"5604\">"
                    "<Name>Max Range Value</Name>"
                    "<Operations>R</Operations>"
                    "<MultipleInstances>Single</MultipleInstances>"
                    "<Mandatory>Optional</Mandatory>"
                    "<Type>Float</Type>"
                    "<RangeEnumeration>"
                    "</RangeEnumeration>"
                    "<Units>Defined by \"Units\" resource.</Units>"
                    "<Description>The maximum value that can be measured by the sensor</Description>"
                "</Item>"
                "<Item ID=\"5701\">"
                    "<Name>Sensor Units</Name>"
                    "<Operations>R</Operations>"
                    "<MultipleInstances>Single</MultipleInstances>"
                    "<Mandatory>Optional</Mandatory>"
                    "<Type>String</Type>"
                    "<RangeEnumeration>"
                    "</RangeEnumeration>"
                    "<Units>"
                    "</Units>"
                    "<Description>Measurement Units Definition e.g. \"Cel\" for Temperature in Celsius.</Description>"
                "</Item>"
                "<Item ID=\"5605\">"
                    "<Name>Reset Min and Max Measured Values</Name>"
                    "<Operations>E</Operations>"
                    "<MultipleInstances>Single</MultipleInstances>"
                    "<Mandatory>Optional</Mandatory>"
                    "<Type>String</Type>"
                    "<RangeEnumeration>"
                    "</RangeEnumeration>"
                    "<Units>"
                    "</Units>"
                    "<Description>Reset the Min and Max Measured Values to Current Value</Description>"
                "</Item>"
                "</Resources>"
                "<Description2>"
                "</Description2>"
            "</Object>"
        "</LWM2M>\r\n"
    };
    return object_3303;
};

#endif // ONE_EDGE_H