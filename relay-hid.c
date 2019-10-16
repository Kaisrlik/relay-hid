/**
Copyright © 2019 Jan Kaisrlik

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <hidapi/hidapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

//Global variables
static unsigned short VENDOR_ID = 0x0519;
static unsigned short PRODUCT_ID = 0x2018;
static int verbose_flag = 0;

#define RELAY_OFF 1
#define RELAY_ON 241
#define RELAY_TOGGLE 0

static void usage(char *name) {
	fprintf(stderr, "Usage: %s [OPTION]\n", name);
	fprintf(stderr, "\n");
	fprintf(stderr, "  OPTION: \n");
	fprintf(stderr, "     -h, --help        display this help and exit\n");
	fprintf(stderr, "     --verbose         enable verbose loging messages\n");
	fprintf(stderr, "     -0, --off         change state of the relay to off\n");
	fprintf(stderr, "     -1, --on          change state of the relay to on\n");
	fprintf(stderr, "     -t, --toggle      toggle on and off state with 2s delay\n");
	fprintf(stderr, "     -i, --id          modify id of the device (it is not supported)\n");
	fprintf(stderr, "                       default: %04hx:%04hx\n", VENDOR_ID, PRODUCT_ID);
	fprintf(stderr, "     -l, --list        list all available devices\n");
	exit(1);
}

static int get_relays(unsigned short vendor_id, unsigned short product_id, struct hid_device_info **devs) {
	struct hid_device_info *relay;
	int relay_count = 0;
	// Initialize the hidapi library
	int res = hid_init();
	if (res < 0) {
		perror("hid_init");
		exit(1);
	}
	*devs = hid_enumerate(vendor_id, product_id);

	//Count the number of returned devices
	relay = *devs;
	for (relay_count = 0; relay != NULL; relay_count++) {
		relay = relay->next;
	}

	if (verbose_flag) {
		fprintf(stderr, "%d device found\n", relay_count);
	}

	return relay_count;
}

static int write_to_relay(struct hid_device_info *relay, int set_on){
	unsigned char buf[1];

	hid_device *handle = hid_open_path(relay->path);
	if (!handle) {
		perror("unable to open device");
		perror(relay->path);
		return -1;
	}

	buf[0] = set_on;
	int res = hid_write(handle, buf, 3);
	if (res < 0)
		perror("hid_write");

	hid_close(handle);
	return res;
}

static int write_to_all_relays(struct hid_device_info *relays, int set_on){
	struct hid_device_info *relay = relays;
	int ret = 0;
	while(relay != NULL) {
		write_to_relay(relay, set_on);
		if (ret < 0) {
			return ret;
		}
		relay = relay->next;
	}
	return ret;
}

static int close_hid(){
	// Finalize the hidapi library
	int res = hid_exit();
	if (res < 0) {
		perror("hid_exit");
	}
	return res;
}

void print_device_info(struct hid_device_info *relays){
	int i = 1;
	struct hid_device_info *relay = relays;
	while(relay != NULL) {
		//Output the device enumeration details if verbose is on
		fprintf(stderr, "Device %d\n", i);
		fprintf(stderr, "  type: %04hx %04hx\n", relay->vendor_id, relay->product_id);
		fprintf(stderr, "  path: %s\n", relay->path);
		fprintf(stderr, "  serial_number: %ls\n", relay->serial_number);
		fprintf(stderr, "  manufacturer: %ls\n", relay->manufacturer_string);
		fprintf(stderr, "  product:      %ls\n", relay->product_string);
		relay = relay->next;
	}
}

int main(int argc, char *argv[])
{
	int c;
	int relay_state = 0;

	while (1) {
		static struct option long_options[] =
		{
			{"verbose", no_argument, &verbose_flag, 1},
			{"list", no_argument, 0, 'l'},
			{"help", no_argument, 0, 'h'},
			{"id", required_argument, 0, 'i'},
			{"on", no_argument, 0, '0'},
			{"off", no_argument, 0, '1'},
			{"toggle", no_argument, 0, 't'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "01thlvi:", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;
			case '1':
				relay_state = RELAY_ON;
				break;
			case '0':
				relay_state = RELAY_OFF;
				break;
			case 't':
				relay_state = RELAY_TOGGLE;
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'i':
				printf ("option -i with value `%s'\n", optarg);
				break;
			case '?':
				/* getopt_long already printed an error message. */
				break;
			default:
				usage(argv[1]);
		}
	}

	if (verbose_flag)
		puts ("verbose flag is set");

	struct hid_device_info *relays, *relay;
	int relay_board_count = get_relays(VENDOR_ID, PRODUCT_ID, &relays);

	if (relay_board_count == 0) {
		fprintf(stderr, "No relay board has been found.\n");
		exit(1);
	}

	if (verbose_flag)
		print_device_info(relays);

	int ret;
	if (relay_state != RELAY_TOGGLE)
		ret = write_to_all_relays(relays, relay_state);
	else {
		ret = write_to_all_relays(relays, RELAY_ON);
		sleep(2);
		ret = write_to_all_relays(relays, RELAY_OFF);
	}

	ret = close_hid();
	return ret;
}
