#include "heapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "cJSON/cJSON.h"

struct HEApiServer apiServer;

void heapi_send (struct HEApiClient *client, cJSON *json) {
    char *buffer_out;
    buffer_out = cJSON_PrintUnformatted(json);
    if(buffer_out == NULL) {

        return;
    }
    if(client == NULL) {
        for(int i = 0; i < MAX_API_CLIENT_COUNT; i++) {
            if(apiServer.clients[i].connected)
                send(apiServer.clients[i].sockfd, buffer_out, strlen(buffer_out), 0);
        }
    }
    else {
        if(client->connected)
            send(client->sockfd, buffer_out, strlen(buffer_out), 0);
    }

    free(buffer_out);
}

int heapi_validate_msg (cJSON *json) {
    return 0;
}

// Authentication message
int heapi_msg_AUTH (struct HEApiClient *client, cJSON *json) {
    int err = 0;
    err = strcmp((const char *)cJSON_GetObjectItem(json, "key")->valuestring, HEAPI_KEY);

    // RESPONSE
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "cmd", APICMD_REGISTER);
    cJSON_AddNumberToObject(resp, "reqid", cJSON_GetObjectItem(json, "reqid")->valueint);
    cJSON_AddBoolToObject(resp, "status", err == 0);
    heapi_send(client, resp);
    cJSON_Delete(resp);

    return err;
}

int heapi_msg_DEVICES (struct HEApiClient *client, cJSON *json) {
    int err = 0;

    // RESPONSE
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "cmd", APICMD_REGISTER);
    cJSON_AddNumberToObject(resp, "reqid", cJSON_GetObjectItem(json, "reqid")->valueint);
    cJSON *resp_devices = cJSON_AddArrayToObject(resp, "devices");

    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) 
            cJSON_AddItemToArray(resp_devices, hedev_to_json(device_list[i]));
    }
    
    heapi_send(client, resp);
    cJSON_Delete(resp);

    return err;
}


int heapi_parse_client_message (struct HEApiClient *client) {
    int err = 0;
    cJSON *json = cJSON_Parse((const char *)client->recvBuffer);
    if(json == NULL) {
        err = 1;
        goto clean;
    }
    if(!cJSON_HasObjectItem(json, "cmd")) {
        err = 1;
        goto clean;
    }

    cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
    if(!client->authenticated && cmd->valueint != APICMD_REGISTER) {
        printf("[ERROR] Client not authenticated\n");
        return 1;
    }
    // Check for incorrect format
    err = heapi_validate_msg(json);
    if(err) {
        goto clean;
    }
    switch (cmd->valueint)
    {
        case APICMD_REGISTER:
            err = heapi_msg_AUTH(client, json);
            break;
        case APICMD_DEVICES:
            err = heapi_msg_DEVICES(client, json);
            break;
        default:
            break;
    }

    clean:

    cJSON_Delete(json);
    return err;
}

void *heapi_client_listener (void *data) {
    struct HEApiClient *client = (struct HEApiClient*)data;
    printf("Client connected\n");
    
    while(client->connected) {
        int len = recv(client->sockfd, client->recvBuffer, MAX_RECV_BUFFER_SIZE, 0);
        if(len == 0) {
            printf("Client disconnected\n");
            break;
        }
        else if(len < 0) {
            printf("[WARNING] Client recv failed!\n");
            break;
        }
        client->recvBuffer[len] = 0;
        int err = heapi_parse_client_message(client);
        if(err > 0) {
            printf("[WARNING] Message parse error 0x%X\n", err);
            break;
        }
    }
    // CLIENT DISCONNECT
    memset(client, 0, sizeof(struct HEApiClient));
    client->sockfd = -1;
    apiServer.clientCount--;
    // Exit thread
    pthread_exit(NULL);
}

void *heapi_server_listener (void *data) {
    while(1) {
        while(apiServer.status == APISERVER_STATE_CONNECTED) {
            // Wait for available connection slots...
            while(apiServer.clientCount >= MAX_API_CLIENT_COUNT) {
                sleep(1);
            }

            struct HEApiClient *cli = NULL;

            // Get first available slot for the client
            for(int i = 0; i < MAX_API_CLIENT_COUNT; i++) {
                if(!apiServer.clients[i].connected) {
                    cli = &apiServer.clients[i];
                    break;
                }
            }
            // Should not happen
            assert(cli != NULL);

            socklen_t cli_addr_size = sizeof(struct sockaddr_in);
            cli->sockfd = accept(apiServer.sockfd, (struct sockaddr *)&cli->addrInfo, &cli_addr_size);
            if(cli->sockfd < 0) {
                printf("[ERROR] error accepting a socket\n");
                cli->sockfd = -1;
                continue;
            }
            printf("Client connected, starting thread..\n");
            cli->connected = 1;
            if(pthread_create(&cli->thread_client_listener, NULL, heapi_client_listener, cli)) {
                printf("[ERROR] failed to create client thread\n");
                exit(1);
            }
        }
        // In case of server shutting down, wait for 5s and restart
        sleep(5);
        heapi_create_server(0);
    }
}

int heapi_create_server (uint8_t create_thread) {
    int err = 0;
    apiServer.sockfd = -1;
    apiServer.port = 24429;
    apiServer.status = APISERVER_STATE_NOT_CONNECTED;
    memset(apiServer.clients, 0, sizeof(struct HEApiClient) * MAX_API_CLIENT_COUNT);
    for(int i = 0; i < MAX_API_CLIENT_COUNT; i++) {
        apiServer.clients[i].sockfd = -1;
    }
    apiServer.clientCount = 0;

    apiServer.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(apiServer.sockfd < 0) {
		// ERROR
		printf("[ERROR] Failed to open socket\n");
		return -1;
	}
	/*
	struct timeval timeout;      
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    if (setsockopt (MSNet::sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		// WARN
		printf("[WARN] Failed to set socket option SO_RCVTIMEO\n");
	}
	*/
	
	// Reuse address
	int en = 1;
	if(setsockopt(apiServer.sockfd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(int)) < 0) {
		// WARN
		printf("[WARN] Failed to set socket option SO_REUSEADDR\n");
	}
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(apiServer.port);
	
	if(bind(apiServer.sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		// ERROR
		printf("[ERROR] Failed to bind socket\n");
		return -1;
	}
	
	listen(apiServer.sockfd, 4);
	apiServer.status = APISERVER_STATE_CONNECTED;
    if(create_thread) {
        err = pthread_create(&apiServer.thread_server_listener, NULL, heapi_server_listener, NULL);
    }
    return err;
}

// Triggers an event and transmits to clients
void heapi_trigger_event (enum ApiEventType event, struct HEDev *device) {
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddNumberToObject(msg, "cmd", (const double)APICMD_EVENT);
    cJSON_AddNumberToObject(msg, "type", (const double)event);

    switch(event) {
        case APIEVENT_DEVICE_CONNECTED:
        case APIEVENT_DEVICE_DISCONNECTED:
            cJSON_AddItemToObject(msg, "device", hedev_to_json(device));
            break;
        default:
            break;
    }

    heapi_send(NULL, msg);

    cJSON_Delete(msg);
}

void heapi_cleanup () {
    // Close connections
    for(int i = 0; i < MAX_API_CLIENT_COUNT; i++) {
        if(apiServer.clients[i].sockfd >= 0 && apiServer.clients[i].connected) {
            apiServer.clients[i].connected = 0;
            shutdown(apiServer.clients[i].sockfd, SHUT_RDWR);
            close(apiServer.clients[i].sockfd);
        }
    }
    // Close the server
    shutdown(apiServer.sockfd, SHUT_RDWR);
    close(apiServer.sockfd);
    apiServer.sockfd = -1;
}