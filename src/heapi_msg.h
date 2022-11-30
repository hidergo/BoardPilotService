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
 * @brief Parse set IQS5xx registers message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int heapi_msg_SET_IQS_REGS (struct HEApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse get IQS5xx registers message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int heapi_msg_GET_IQS_REGS (struct HEApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse set keymap message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int heapi_msg_SET_KEYMAP (struct HEApiClient *client, cJSON *json, cJSON *resp);

/**
 * @brief Parse get keymap message
 * 
 * @param client 
 * @param json 
 * @param resp 
 * @return int 
 */
int heapi_msg_GET_KEYMAP (struct HEApiClient *client, cJSON *json, cJSON *resp);

#endif