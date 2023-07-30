#ifndef __BPAPI_MSG_H
#define __BPAPI_MSG_H

#include "cJSON/cJSON.h"
#include "bpapi.h"

/**
 * @brief Checks message format
 * 
 * @param json 
 * @return int 
 */
int bpapi_msg_validate (cJSON *json);

/**
 * @brief Parse authentication message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int bpapi_msg_AUTH (struct BPApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse get devices message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int bpapi_msg_DEVICES (struct BPApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse write field to the device message
 * 
 * @param client
 * @param json
 * @param resp
 * @return int
*/
int bpapi_msg_ZMK_CONTROL_WRITE (struct BPApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse read field from the device message
 * 
 * @param client
 * @param json
 * @param resp
 * @return int
*/
int bpapi_msg_ZMK_CONTROL_READ (struct BPApiClient *client, cJSON *json, cJSON *resp);

#endif