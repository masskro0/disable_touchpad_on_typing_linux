#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/* Generates the commands for enabling and disabling touchpad. It uses xinput as the system interface.
 *
 * @param enable_tp: Pointer to string which will contain the enabling command.
 * @param disable_tp: Pointer to string which will contain the disabling command.
 * @param id: String containing the touchpad ID of 'xinput list'. 
 */
void generateCommands(char** enable_tp, char** disable_tp, char* id) {
    int len_id = strlen(id);

    char* enable_ = "xinput enable ";
    int len_enable = strlen(enable_);
    *enable_tp = (char*) malloc(len_enable + len_id + 1);
    memcpy(*enable_tp, enable_, len_enable);
    memcpy(*enable_tp + len_enable, id, len_id);
    (*enable_tp)[len_enable + len_id] = '\0';
    
    char* disable_ = "xinput disable ";
    int len_disable = strlen(disable_);
    *disable_tp = (char*) malloc(len_disable + len_id + 1);
    memcpy(*disable_tp, disable_, len_disable);
    memcpy(*disable_tp + len_disable, id, len_id);
    (*disable_tp)[len_disable + len_id] = '\0';
}

/* Determines a >>possible<< keystroke event file. Since xinput lists many keyboards, which are also virtual,
 * you might get the wrong event file. Use the flag -e 'number' to fix that. The event files are in '/dev/input/'.
 *
 * @param event_file: Pointer to string which will contain the event file path.
 */
void getEventFile(char** event_file) {
    // Get the first keyboard ID from 'xinput list'.
    char* get_id_cmd = "xinput --list | cut -d\[ -f1 | grep -i keyboard | egrep -iv 'virtual|video|button|bus' | egrep -o 'id=[0-9]+' | egrep -o '[0-9]+'";
    FILE* fpipe = popen(get_id_cmd, "r");
    size_t n;
    char* keyboardID = NULL;
    getline(&keyboardID, &n, fpipe);
    pclose(fpipe);

    // Get the path to the keystroke event file using the above determined ID.
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

/* Determine the touchpad ID by using 'xinput list'.
 *
 * @param id: Pointer to string which will contain the touchpad ID.
 */
void getTouchpadID(char** id) {
    char* get_id_cmd = "xinput --list --long | grep Touchpad | egrep -o 'id=[0-9]+' | egrep -o '[0-9]+'";
    FILE* fpipe = popen(get_id_cmd, "r");
    size_t n = 0;
    getline(id, &n, fpipe);
    pclose(fpipe);
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

    char* touchpadID = NULL;
    getTouchpadID(&touchpadID);

    char* enable_tp;            // Command to enable the touchpad.
    char* disable_tp;           // Command to disable the touchpad.
    char* event_file;           // Path to event file registring the keystrokes.
    generateCommands(&enable_tp, &disable_tp, touchpadID);
    if (keyboard_event_id != -1) {
        char* base_cmd = "/dev/input/event";
        event_file = malloc(strlen(base_cmd) + sizeof(char) * 3);
        sprintf(event_file, "%s%d", base_cmd, keyboard_event_id);
    } else {
        getEventFile(&event_file);
        event_file[strlen(event_file) - 1] = '\0';
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
                        if (sec_last_stroke != -1) {
                            // Sleep for timeout milliseconds after last keystroke.
                            gettimeofday(&current, NULL);
                            usec_remaining = (long)timeout * 1000
                                           - (current.tv_sec - sec_last_stroke) * 1000000
                                           - (current.tv_usec - usec_last_stroke);
                            usleep(usec_remaining);
                            goto read_remaining_keystrokes;
                        }
                    system(enable_tp);
                }
            }
        }
    }

    // System cleanup to avoid memory leaks and enable touchpad again.
    system(enable_tp);
    free(enable_tp);
    free(disable_tp);
    free(event_file);
    free(touchpadID);
    return 0;
}
