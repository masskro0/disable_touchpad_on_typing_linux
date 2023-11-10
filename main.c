#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>


void generateCommands(char** enable_tp, char** disable_tp, char* id) {
	char* enable_ = "xinput enable ";
	char* disable_ = "xinput disable ";
	int len_enable = strlen(enable_);
	int len_disable = strlen(disable_);
	int len_id = strlen(id);
	*enable_tp = (char*) malloc(len_enable + len_id + 1);
	memcpy(*enable_tp, enable_, len_enable);
	memcpy(*enable_tp + len_enable, id, len_id);
	(*enable_tp)[len_enable + len_id] = '\0';
	*disable_tp = (char*) malloc(len_disable + len_id + 1);
	memcpy(*disable_tp, disable_, len_disable);
	memcpy(*disable_tp + len_disable, id, len_id);
	(*disable_tp)[len_disable + len_id] = '\0';
}

void getEventFile(char** event_file) {
    char* get_id_cmd = "xinput --list | cut -d\[ -f1 | grep -i keyboard | egrep -iv 'virtual|video|button|bus' | egrep -o 'id=[0-9]+' | egrep -o '[0-9]+'";
    FILE* fpipe = popen(get_id_cmd, "r");
    size_t n;
    char* keyboardID = NULL;
    getline(&keyboardID, &n, fpipe);
    pclose(fpipe);
    
    char* base_cmd1 = "xinput list-props ";
    char* base_cmd2 = " | grep -o '/dev/input.*' | rev | cut -c 2- | rev";
    char* get_keystrokes_file_cmd = (char*) malloc(strlen(base_cmd1) + strlen(base_cmd2) + sizeof(char) * 3);
    sprintf(get_keystrokes_file_cmd, "%s%d%s", base_cmd1, atoi(keyboardID), base_cmd2);
    fpipe = popen(get_keystrokes_file_cmd, "r");
    size_t n2 = 0;
    *event_file = NULL;
    getline(event_file, &n2, fpipe);
    free(get_keystrokes_file_cmd);
    free(keyboardID);
    pclose(fpipe);
} 

void getTouchpadID(char** id) {
	char* get_id_cmd = "xinput --list --long | grep Touchpad | egrep -o 'id=[0-9]+' | egrep -o '[0-9]+'";
	FILE* fpipe = popen(get_id_cmd, "r");
	size_t n = 0;
	getline(id, &n, fpipe);
	pclose(fpipe);
}

void printHelp(double timeout) {
    printf("-e: ID of the keyboard event, e.g. 12 if your keystrokes are dumped into /dev/input/event12. "
           "This program tries to autodetect your event ID, but if it doesn't work then enter it manually.\n"
           "-h: This helper message.\n"
           "-t: Timeout between the last keystroke and enabling the touchpad in MILLISECONDS. ");
    printf("Default: %d\n", (int) timeout);
}

static volatile int running = 1;

void onExit() {
    running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, onExit);
    int c;
	double timeout = 2000.0;
	int keyboard_event_id = -1;

	if (argc < 2)
	{
	    printf("Use the -h flag to see available options. Using default timeout of %d milliseconds.\n", (int) timeout);
	}
    while ((c=getopt(argc, argv, "he:t:")) != -1) {
        switch (c) {
            case 'h':
                printHelp(timeout);
                return 1;
            case 'e':
                keyboard_event_id = atoi(optarg);
                break;
            case 't':
                timeout = atof(optarg);
                break;
            case '?':
                printHelp(timeout);
                return 1;
        default:
            printHelp(timeout);
            abort();
        }    
    }
    
	clock_t begin = clock();
	clock_t now;
	double t;

	char* id = NULL;
	getTouchpadID(&id);

	char* enable_tp;
	char* disable_tp;
	char* event_file;
    generateCommands(&enable_tp, &disable_tp, id);
	if (keyboard_event_id != -1) {
	    char* base_cmd = "/dev/input/event";
	    event_file = malloc(strlen(base_cmd) + sizeof(char) * 3);
        sprintf(event_file, "%s%d", base_cmd, keyboard_event_id);
	} else {
	    getEventFile(&event_file);
        event_file[strlen(event_file) - 1] = '\0';
	}

    struct input_event ev;
    int fd = open(event_file, O_RDONLY);
    long flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
		
	while (running) {				
		read(fd, &ev, sizeof(ev));
    	if ((ev.type == EV_KEY) && (ev.value == 0)) {
		    system(disable_tp);
            begin = clock();
			t = 0.0;
			while (t < timeout) {
				now = clock();
				t = (double)(now - begin) / CLOCKS_PER_SEC * 1000.0;
				read(fd, &ev, sizeof(ev));
				if ((ev.type == EV_KEY) && (ev.value == 0)) {
					begin = clock();
				}
			}
			system(enable_tp);
		}
	}
	
    system(enable_tp);
    free(enable_tp);
    free(disable_tp);
    free(event_file);
	free(id);
	return 0;
}
