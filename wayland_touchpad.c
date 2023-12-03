#include <dirent.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Determines a >>possible<< keystroke event file. Since /dev/input/by-id lists may list many keyboards, you might get
 * the wrong event file. Use the flag -e 'number' to fix that. The event files are in '/dev/input/eventX'.
 *
 * @param event_file: Pointer to string which will contain the event file path.
 */
void getEventFile(char** event_file) {
    DIR *d;
    struct dirent *entry;
    char* dir_path = "/dev/input/by-id/";
    d = opendir(dir_path);
    int len;
    char* keyboard_name = NULL;
    size_t path_len;
    
    // Iterate over event files in directory and store the name of the last file ending with -kbd.
    if (d) {
        while ((entry = readdir(d)) != NULL) {
            len = strlen(entry->d_name);
            if (!strcmp(entry->d_name + len - 4, "-kbd")) {
            	if (*event_file != NULL) {
            	    free(*event_file);
            	}
                keyboard_name = strdup(entry->d_name);
                path_len = strlen(dir_path) + strlen(keyboard_name) + 2;
                *event_file = malloc(path_len);
                snprintf(*event_file, path_len, "/dev/input/by-id/%s", keyboard_name);
                free(keyboard_name);
            }
        }
        closedir(d);
    }
}

/* Print the helper message which is triggered by the -h flag.*/
void printHelp() {
    printf("-e: ID of the keyboard event, e.g. 12 if your keystrokes are dumped into /dev/input/event12. "
           "This program tries to autodetect your event ID, but if it doesn't work then enter it manually.\n"
           "-h: This helper message.\n"
           "-t: Timeout between the last keystroke and enabling the touchpad in MILLISECONDS. ");
}

// Determines whether the main loop is running or not.
static volatile int running = 1;

/* Catches a SIGINT signal and causes the main loop to break.*/
void onExit() {
    running = 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, onExit);
    int c;                          // Used for command line parsing.
    double timeout = 1000.0;        // Timeout between last keystroke and re-enabling the touchpad.
    int keyboard_event_id = -1;     // Keystroke event ID passed by the user.

    if (argc < 2)
    {
        printf("Use the -h flag to see available options. Using default timeout of %d milliseconds.\n", (int) timeout);
    }
    while ((c=getopt(argc, argv, "he:t:")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                return 1;
            case 'e':
                keyboard_event_id = atoi(optarg);
                break;
            case 't':
                timeout = atof(optarg);
                break;
            case '?':
                printHelp();
                return 1;
        default:
            printHelp();
            abort();
        }
    }

    // Commands to enable/disable the touchpad.
    char* enable_tp = "gsettings set org.gnome.desktop.peripherals.touchpad send-events enabled"; 
    char* disable_tp = "gsettings set org.gnome.desktop.peripherals.touchpad send-events disabled";
    char* event_file = NULL;    // Path to event file registring the keystrokes.
    if (keyboard_event_id != -1) {
        char* base_cmd = "/dev/input/event";
        event_file = malloc(strlen(base_cmd) + sizeof(char) * 3);
        sprintf(event_file, "%s%d", base_cmd, keyboard_event_id);
    } else {
        getEventFile(&event_file);
    }
    
    if (event_file == NULL) {
        printf("Could not open event file\n");
        return 1;
    }

    struct input_event ev;      // Handles the keystroke detection.
    int fd = open(event_file, O_RDONLY);
    long flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);

    // Register file descriptors for polling.
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;

    // Time variables.
    int sec_last_stroke;
    int usec_last_stroke;
    long usec_remaining;
    struct timeval current;

    while (running) {
        // Check for incoming keystrokes.
        if (poll(fds, 1, 500) > 0) {
            if (fds[0].revents) {
                read(fd, &ev, sizeof(ev));
                if ((ev.type == EV_KEY) && (ev.value == 0)) {
                    // Keystroke detected. Disable touchpad.
                    system(disable_tp);
                    usleep(timeout * 1000);
                    read_remaining_keystrokes:
                        sec_last_stroke = -1;
                        usec_last_stroke = -1;
                        while(read(fd, &ev, sizeof(ev)) != -1) {
                            // Read remaining keystrokes up to now.
                            if ((ev.type == EV_KEY) && (ev.value == 0)) {
                                sec_last_stroke = ev.time.tv_sec;
                                usec_last_stroke = ev.time.tv_usec;
                            }
                        }
                        if (sec_last_stroke > 0) {
                            // Sleep for timeout milliseconds after last keystroke.
                            gettimeofday(&current, NULL);
                            usec_remaining = (long)timeout * 1000
                                           - (current.tv_sec - sec_last_stroke) * 1000000
                                           - (current.tv_usec - usec_last_stroke);
                            if (usec_remaining > 0) {
                                usleep(usec_remaining);
                                goto read_remaining_keystrokes;
                            }
                        }
                    system(enable_tp);
                }
            }
        }
    }

    // System cleanup to avoid memory leaks and enable touchpad again.
    system(enable_tp);
    free(event_file);
    return 0;
}
