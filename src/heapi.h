#ifndef __HEAPI_H
#define __HEAPI_H
#include <stdint.h>
#if defined(_WIN32)
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#include <conio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include "hedev.h"

#define HEAPI_KEY "p*kG462jhJBY166EZLKxf9Du"

#define MAX_API_CLIENT_COUNT    8
#define MAX_SUBSCRIPTION_COUNT  8
#define MAX_RECV_BUFFER_SIZE    4096

#define ERR_API_AUTH            0x10

enum ApiServerState {
    APISERVER_STATE_NOT_CONNECTED,
    APISERVER_STATE_CONNECTED
};

enum ApiClientType {
    APICLIENT_TYPE_UNKNOWN,
    APICLIENT_TYPE_HIDERGOUI,
    APICLIENT_TYPE_CUSTOM
};

enum ApiEventType {
    APIEVENT_NONE,
    APIEVENT_DEVICE_CONNECTED,
    APIEVENT_DEVICE_DISCONNECTED
};

// API Commands
enum ApiCommand {
    APICMD_REGISTER =       0x01,
    APICMD_DEVICES =        0x10,

    // Events
    APICMD_EVENT =          0x80

};

// Api client structure
struct HEApiClient {
#if defined(__linux__)
    pthread_t thread_client_listener;
    int sockfd;
#elif defined(_WIN32)
    HANDLE thread_client_listener;
    SOCKET sockfd;
#endif
    struct sockaddr_in addrInfo;
    enum ApiClientType clientType;
    uint8_t connected;
    uint8_t authenticated;
    char recvBuffer[MAX_RECV_BUFFER_SIZE];
};

// API server structure
struct HEApiServer {
#if defined(__linux__)
    pthread_t thread_server_listener;
    int sockfd;
#elif defined(_WIN32)
    HANDLE thread_server_listener;
    SOCKET sockfd;
#endif
    uint16_t port;
    enum ApiServerState status;
    struct HEApiClient clients[MAX_API_CLIENT_COUNT];
    uint8_t clientCount;
};

extern struct HEApiServer apiServer;

int heapi_create_server (uint8_t create_thread);

void heapi_trigger_event (enum ApiEventType eventType, struct HEDev *device);

void heapi_cleanup ();

#endif