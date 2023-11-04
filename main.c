#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <time.h>


int main() {
    struct input_event ev;
    int fd = open("/dev/input/event12", O_RDONLY);	// TODO
    long flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
	double timeout = 2000.0;						// TODO
	clock_t begin = clock();
	clock_t now;
	double t;
	while (1) {				
		read(fd, &ev, sizeof(ev));
		begin = clock();
    	if ((ev.type == EV_KEY) && (ev.value == 0)) {
		    system("xinput --disable 13");			// TODO
			now = clock();
			t = (double)(now - begin) / CLOCKS_PER_SEC * 1000.0;
			while (t < timeout) {
				now = clock();
				t = (double)(now - begin) / CLOCKS_PER_SEC * 1000.0;
				read(fd, &ev, sizeof(ev));
				if ((ev.type == EV_KEY) && (ev.value == 0)) {
					begin = clock();
				}
			}
			system("xinput --enable 13");			// TODO
		}
	}
	return 0;
} 