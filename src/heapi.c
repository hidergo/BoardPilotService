#include "heapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)

#endif
#include <assert.h>
#include "cJSON/cJSON.h"
#include "hedef.h"
#include "heapi_msg.h"
#include "zmk_control.h"

#if defined(_WIN32)
#define UNUSED UNREFERENCED_PARAMETER
#else
#define UNUSED(x) (void)(x)
#endif

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
                send(apiServer.clients[i].sockfd, buffer_out, (int)strlen(buffer_out) + 1, 0);
        }
    }
    else {
        if(client->connected)
            send(client->sockfd, buffer_out, (int)strlen(buffer_out) + 1, 0);
    }

    cJSON_free(buffer_out);
}

int heapi_parse_client_message (struct HEApiClient *client) {
    int err = 0;
    cJSON *json = cJSON_Parse((const char *)client->recvBuffer);

    // Create response object
    // cmd and reqid is checked in heapi_msg_validate, so they shouldn't be null
    cJSON *resp = cJSON_CreateObject();

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
    err = heapi_msg_validate(json);
    if(err) {
        goto clean;
    }

    // Add cmd to response
    cJSON_AddNumberToObject(resp, "cmd", cmd->valueint);
    // Add reqid response
    cJSON_AddNumberToObject(resp, "reqid", cJSON_GetObjectItem(json, "reqid")->valueint);

    switch (cmd->valueint)
    {
        case APICMD_REGISTER:
            err = heapi_msg_AUTH(client, json, resp);
            printf("Client register status: %i\n", err);
            break;
        case APICMD_DEVICES:
            err = heapi_msg_DEVICES(client, json, resp);
            break;
        case APICMD_SET_IQS_REGS:
            err = heapi_msg_SET_IQS_REGS(client, json, resp);
            break;
        case APICMD_GET_IQS_REGS:
            err = heapi_msg_GET_IQS_REGS(client, json, resp);
            break;
        case APICMD_SET_KEYMAP:
            err = heapi_msg_SET_KEYMAP(client, json, resp);
            break;
        case APICMD_GET_KEYMAP:
            err = heapi_msg_GET_KEYMAP(client, json, resp);
            break;
        case APICMD_SET_MOUSE_SENS:
            err = heapi_msg_SET_MOUSE_SENSITIVITY(client, json, resp);
            break;
        case APICMD_GET_MOUSE_SENS:
            err = heapi_msg_GET_MOUSE_SENSITIVITY(client, json, resp);
            break;
        case APICMD_ZMK_CONTROL_WRITE:
            err = heapi_msg_ZMK_CONTROL_WRITE(client, json, resp);
            break;
        case APICMD_ZMK_CONTROL_READ:
            err = heapi_msg_ZMK_CONTROL_READ(client, json, resp);
            break;
        default:
            break;
    }

    // Send response
    heapi_send(client, resp);
    
    clean:

    cJSON_Delete(resp);

    cJSON_Delete(json);
    return err;
}

#if defined(__linux__)
void *heapi_client_listener (void *data) {
#elif defined(_WIN32)
DWORD WINAPI heapi_client_listener (void *data) {
#endif
    struct HEApiClient *client = (struct HEApiClient*)data;
    printf("Client connected\n");
    
    while(client->connected) {
        int len = recv(client->sockfd, client->recvBuffer, MAX_RECV_BUFFER_SIZE, 0);
        if(len == 0) {
            break;
        }
        else if(len < 0) {
            printf("[WARNING] Client recv failed!\n");
            break;
        }
        client->recvBuffer[len] = 0;
        int err = heapi_parse_client_message(client);
        if(err != 0) {
            printf("[WARNING] Message parse error 0x%X\n", err);
            //break;
        }
    }
#if defined(__linux__)
    shutdown(client->sockfd, SHUT_RDWR);
    close(client->sockfd);
#elif defined(_WIN32)
    shutdown(client->sockfd, SD_BOTH);
    closesocket(client->sockfd);
#endif
    printf("Client disconnected\n");
    // CLIENT DISCONNECT
    memset(client, 0, sizeof(struct HEApiClient));
    client->sockfd = (socktype_t)-1;
    apiServer.clientCount--;
    // Exit thread
#if defined(__linux__)
    pthread_exit(NULL);
#elif defined(_WIN32)
    return 0;
#endif
}

#if defined(__linux__)
void *heapi_server_listener (void *data) {
#elif defined(_WIN32)
DWORD WINAPI heapi_server_listener (void *data) {
#endif
    UNUSED(data);

    while(1) {
        while(apiServer.status == APISERVER_STATE_CONNECTED) {
            // Wait for available connection slots...
            while(apiServer.clientCount >= MAX_API_CLIENT_COUNT) {
                #if defined(__linux__)
                sleep(1);
                #elif defined(_WIN32)
                Sleep(1000);
                #endif
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
            printf("Waiting for next connection\n");
            cli->sockfd = accept(apiServer.sockfd, (struct sockaddr *)&cli->addrInfo, &cli_addr_size);
            if((int)cli->sockfd < 0) {
                printf("[ERROR] error accepting a socket\n");
                cli->sockfd = (socktype_t)-1;
                continue;
            }
            printf("Client connected, starting thread..\n");
            cli->connected = 1;
            apiServer.clientCount++;
            #if defined(__linux__)
                if(pthread_create(&cli->thread_client_listener, NULL, heapi_client_listener, cli)) {
                    printf("[ERROR] failed to create client thread\n");
                    exit(1);
                }
            #elif defined(_WIN32)
                apiServer.thread_server_listener = CreateThread(NULL, 0, heapi_client_listener, cli, 0, NULL);
                if(apiServer.thread_server_listener == NULL) {
                    printf("[ERROR] failed to create client thread\n");
                    exit(1);
                }
            #endif
        }
        // In case of server shutting down, wait for 5s and restart
        #if defined(__linux__)
            sleep(5);
        #elif defined(_WIN32)
            Sleep(5000);
        #endif
        printf("Recreate server\n");
        heapi_create_server(0);
    }
}

int heapi_create_server (uint8_t create_thread) {

    // WSA startup for windows
#if defined(_WIN32)
    WSADATA wsa;    
    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
    {
        printf("[ERROR] Could not initialize WSA (%i)", WSAGetLastError());
        return -1;
    }
#endif

    int err = 0;
    apiServer.sockfd = (socktype_t)-1;
    apiServer.port = 24429;
    apiServer.status = APISERVER_STATE_NOT_CONNECTED;
    memset(apiServer.clients, 0, sizeof(struct HEApiClient) * MAX_API_CLIENT_COUNT);
    for(int i = 0; i < MAX_API_CLIENT_COUNT; i++) {
        apiServer.clients[i].sockfd = (socktype_t)-1;
    }
    apiServer.clientCount = 0;
    // TODO: IPPROTO_TCP?
    apiServer.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if((int)apiServer.sockfd < 0) {
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
	if(setsockopt(apiServer.sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&en, sizeof(int)) < 0) {
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
        #if defined(__linux__)
            err = pthread_create(&apiServer.thread_server_listener, NULL, heapi_server_listener, NULL);
        #elif defined(_WIN32)
            apiServer.thread_server_listener = CreateThread(NULL, 0, heapi_server_listener, NULL, 0, NULL);
            if(apiServer.thread_server_listener == NULL) {
                err = -1;
            }
        #endif
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
            #if defined(__linux__)
                shutdown(apiServer.clients[i].sockfd, SHUT_RDWR);
                close(apiServer.clients[i].sockfd);
            #elif defined(_WIN32)
                shutdown(apiServer.clients[i].sockfd, SD_BOTH);
                closesocket(apiServer.clients[i].sockfd);
            #endif
        }
    }
    // Close the server
#if defined(__linux__)
    shutdown(apiServer.sockfd, SHUT_RDWR);
    close(apiServer.sockfd);
    apiServer.sockfd = -1;
#elif defined(_WIN32)
    shutdown(apiServer.sockfd, SD_BOTH);
    closesocket(apiServer.sockfd);
    WSACleanup();
#endif

}