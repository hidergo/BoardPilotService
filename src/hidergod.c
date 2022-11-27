#include <stdio.h>
#include <hidapi.h>
#include "hedev.h"
#include "heapi.h"
#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif
#include <wchar.h>
#include <signal.h>


#define MAX_STR 255

void cleanup ();


void exit_handler(int n) {
	cleanup();

	exit(0);
}

int main(int argc, char **argv)
{   
	signal(SIGINT, exit_handler);
    int err;
    printf("Starting hid:ergo daemon\n");
    printf("* Defined devices: %i\n", (int)PRODUCT_COUNT);

   	hedev_print_products();

    printf("Starting API server...\n");
    err = heapi_create_server(1);
	if(err) {
		printf("[ERROR] Failed to start API Server!\n");
		return 1;
	}
	printf("* API Server started\n");

	// Init HIDAPI library
	hid_init();

	// Init hedev
	hedev_init();

	printf("* Delaying before first poll...\n");
	
	while(1) {
		// Sleep BEFORE first poll
		#if defined(__linux__)
			sleep(2);
		#elif defined(_WIN32)
			Sleep(2000);
		#endif
		hedev_poll_usb_devices();
	}

	cleanup();

	return 0;
}

// Cleanup
void cleanup () {
	// Clean HID devices

	// Exit HIDAPI
	hid_exit();

	// Cleanup for the server
	heapi_cleanup();

}