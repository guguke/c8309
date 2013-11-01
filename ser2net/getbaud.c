#include <termios.h>
#include <unistd.h>
#include <stdio.h>

int main() {
	struct termios tios;
	tcgetattr(0, &tios);
	speed_t ispeed = cfgetispeed(&tios);
	speed_t ospeed = cfgetospeed(&tios);
	printf("baud rate in: 0%o\n", ispeed);
	printf("baud rate out: 0%o\n", ospeed);
	return 0;
}














