#include <linux/uinput.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>

bool should_stop = false;

void sigint_handler(int _value)
{
    (void) _value;
    should_stop = true;
    printf("SIGINT handled\n");
}

void emit(int fd, int type, int code, int val)
{
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = val;
    /* timestamp values below are ignored */
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(fd, &ie, sizeof(ie));
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Error: No input device specified\n");
        exit(1);
    }
    const char *input_path = argv[1];
    int ifd = open(input_path, O_RDONLY);
    if (ifd == -1) {
        fprintf(stderr, "\"%s\": ", input_path);
        perror("open");
        exit(1);
    }
    int ofd = open("/dev/uinput", O_WRONLY);
    if (ofd == -1) {
        fprintf(stderr, "\"/dev/uinput\": ");
        perror("open");
        exit(1);
    }
    signal(SIGINT, sigint_handler);

    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(ofd, UI_SET_EVBIT, EV_KEY);
    ioctl(ofd, UI_SET_KEYBIT, KEY_SPACE);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Example device");

    ioctl(ofd, UI_DEV_SETUP, &usetup);
    ioctl(ofd, UI_DEV_CREATE);

    /*
     * On UI_DEV_CREATE the kernel will create the device node for this
     * device. We are inserting a pause here so that userspace has time
     * to detect, initialize the new device, and can start listening to
     * the event, otherwise it will not notice the event we are about
     * to send. This pause is only needed in our example code!
     */
    sleep(1);

    while (!should_stop) {
        struct input_event ev;
        ssize_t ret = read(ifd, &ev, sizeof(ev));
        switch (ev.type) {
        case EV_KEY:
            printf("EV_KEY, code=%u, value=%d\n", ev.code, ev.value);
            break;
        case EV_ABS:
            /*
             * d-pad is EV_ABS
             * left-right: code ABS_HAT0X, left=-1, right=1
             * down-up: code ABS_HAT0Y, up=-1, down=1
             */
            if (ev.code == ABS_HAT0X || ev.code == ABS_HAT0Y) {
                printf("type=%u, code=%u, value=%d\n", ev.type, ev.code, ev.value);
            }
            break;
        case EV_SYN:
            break;
        default:
            printf("type=%u, code=%u, value=%d\n", ev.type, ev.code, ev.value);
            break;
        }
    }

    /* Key press, report the event, send key release, and report again */
    emit(ofd, EV_KEY, KEY_SPACE, 1);
    emit(ofd, EV_SYN, SYN_REPORT, 0);
    emit(ofd, EV_KEY, KEY_SPACE, 0);
    emit(ofd, EV_SYN, SYN_REPORT, 0);

    /*
     * Give userspace some time to read the events before we destroy the
     * device with UI_DEV_DESTOY.
     */
    sleep(1);

    ioctl(ofd, UI_DEV_DESTROY);
    close(ofd);

    return 0;
}
