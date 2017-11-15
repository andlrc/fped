#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	INTERFACE	0
#define	VENDOR_ID	0x0426
#define PRODUCT_ID	0x3011
#define FPED_DOWN	((unsigned char[]) { 0x01, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00 })
#define FPED_UP		((unsigned char[]) { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 })

int main(int argc, char **argv)
{
	struct libusb_context *usb_context;
	struct libusb_device_handle *dev_handle;
	unsigned char buf[8];
	int rc;
	int received;
	int is_down;
	char *down_cmd;
	char *up_cmd;

#define	READ_USB()							\
	do {								\
		libusb_bulk_transfer(dev_handle,			\
				     0x81 | LIBUSB_ENDPOINT_IN,		\
				     buf, sizeof(buf),			\
				     &received, 1000 * 1000);		\
	} while(0);

	if (argc != 2 && argc != 3) {
		fprintf(stderr, "Usage: fped <down> [<up>]\n");
		return EXIT_FAILURE;
	}
	down_cmd = argv[1];
	up_cmd = argc > 2 ? argv[2] : 0;

	if ((rc = libusb_init(&usb_context))) {
		fprintf(stderr, "%s\n", libusb_strerror(rc));
		return EXIT_FAILURE;
	}

	libusb_set_debug(usb_context, LIBUSB_LOG_LEVEL_ERROR);

	dev_handle = libusb_open_device_with_vid_pid(usb_context,
						     VENDOR_ID,
						     PRODUCT_ID);
	if (!dev_handle) {
		fprintf(stderr, "%s\n",
			libusb_strerror(LIBUSB_ERROR_NO_DEVICE));
		return EXIT_FAILURE;
	}

	/* Detach active driver */
	if (libusb_kernel_driver_active(dev_handle, INTERFACE)) {
		rc = libusb_detach_kernel_driver(dev_handle, INTERFACE);
		if (rc) {
			fprintf(stderr, "%s\n", libusb_strerror(rc));
			return EXIT_FAILURE;
		}
	}

	if ((rc = libusb_claim_interface(dev_handle, INTERFACE))) {
		fprintf(stderr, "%s\n", libusb_strerror(rc));
		return EXIT_FAILURE;
	}

	/* Main loop */
	is_down = 0;
	for (;;) {
		READ_USB();

		if (received == sizeof(buf)) {
			if (!is_down) {
				if (memcmp(FPED_DOWN, buf, sizeof(buf)) == 0) {
					is_down = 1;
					system(down_cmd);
				}
			} else {
				if (memcmp(FPED_UP, buf, sizeof(buf)) == 0) {
					READ_USB();
					if (memcmp(FPED_UP, buf,
						   sizeof(buf)) == 0) {
						is_down = 0;
						if (up_cmd)
							system(up_cmd);
					}
				}
			}
		}
	}

	if ((rc = libusb_release_interface(dev_handle, INTERFACE))) {
		fprintf(stderr, "%s\n", libusb_strerror(rc));
		return EXIT_FAILURE;
	}

	libusb_close(dev_handle);
	libusb_exit(usb_context);

	return EXIT_SUCCESS;
}
