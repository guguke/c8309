
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
//#include <sstream>

#define EV_PRESSED 1
#define EV_RELEASED 0
#define EV_REPEAT 2

/*
* Purpose: Stuffs the Linux keyboard buffer with a key and
* reads it back out of the buffer.
* All key definitions can be found in input.h file:
* /usr/src/linux-headers-3.2.0-23/include/linux
*
*/
int main(int argc,char *argv[])
{
	/************************************************
	* IMPORTANT
	* you need to execute this code as the su or
	* sudo user in order to open the device properly.
	***********************************************/
	printf("Starting the keyboard buffer writer/reader \n");


	int fd = 0;
	char *device = "/dev/input/event1"; // This is the keyboard device as identified using both: $cat /proc/bus/input/devices
	// and looking in the var/log/Xorg.0.log searching for "keyboard"

	// Write a key to the keyboard buffer
	if( (fd = open(device, O_RDWR)) > 0 )
	{
		struct input_event event;
		//printf("The keyboard code is: %d \n", KEY_B); // Note: these are not the same as ASCII codes.

		// Press a key (stuff the keyboard with a keypress)
		event.type = EV_KEY;
		event.value = EV_PRESSED;
		event.code = KEY_L;
		write(fd, &event, sizeof(struct input_event));

		// Release the key
		event.value = EV_RELEASED;
		event.code = KEY_L;
		write(fd, &event, sizeof(struct input_event));

		// Press a key (stuff the keyboard with a keypress)
		event.type = EV_KEY;
		event.value = EV_PRESSED;
		event.code = KEY_L;
		write(fd, &event, sizeof(struct input_event));

		// Release the key
		event.value = EV_RELEASED;
		event.code = KEY_L;
		write(fd, &event, sizeof(struct input_event));
		// Press a key (stuff the keyboard with a keypress)
		event.type = EV_KEY;
		event.value = EV_PRESSED;
		event.code = KEY_ENTER;
		write(fd, &event, sizeof(struct input_event));

		// Release the key
		event.value = EV_RELEASED;
		event.code = KEY_ENTER;
		write(fd, &event, sizeof(struct input_event));

		close(fd);
	}
#if 0
	// Read the key back from the keyboard buffer
	int fd1 = 0;
	if( (fd1 = open(device, O_RDONLY)) > 0 ) // It's important to open a new file descriptor here or the program will block.
	{
		struct input_event event;
		unsigned int scan_code = 0;

		if(event.type != EV_KEY)
		{
			return 0; // Keyboard events are always of type EV_KEY
		}

		if(event.value == EV_RELEASED)
		{
			scan_code = event.code;
			printf("read back scan_code is: %d \n", scan_code);
		}
		close(fd1);
	}
#endif
	return 0;
}
