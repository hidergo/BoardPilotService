#ifndef __HEAPI_MSG_H
#define __HEAPI_MSG_H

#include "cJSON/cJSON.h"
#include "heapi.h"

/**
 * @brief Checks message format
 * 
 * @param json 
 * @return int 
 */
int heapi_msg_validate (cJSON *json);

/**
 * @brief Parse authentication message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int heapi_msg_AUTH (struct HEApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse get devices message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int heapi_msg_DEVICES (struct HEApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse write field to the device message
 * 
 * @param client
 * @param json
 * @param resp
 * @return int
*/
int heapi_msg_ZMK_CONTROL_WRITE (struct HEApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse read field from the device message
 * 
 * @param client
 * @param json
 * @param resp
 * @return int
*/
int heapi_msg_ZMK_CONTROL_READ (struct HEApiClient *client, cJSON *json, cJSON *resp);

#endif