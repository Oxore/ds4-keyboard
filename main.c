#include <linux/uinput.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <inttypes.h>

enum mapping_type {
    MT_SINGLE = 0,
    MT_DOUBLE = 1,
    MT_SINGLE_SINGLE = 2,
    MT_DOUBLE_SINGLE = 3,
    MT_SINGLE_DOUBLE = 4,
    MT_DOUBLE_DOUBLE = 5,
};

enum side {
    SIDE_NO = 0,
    SIDE_LEFT = 1,
    SIDE_RIGHT = 2,
};

struct mapping {
    enum side first;
    uint32_t keys;
    uint16_t code;
};

enum keys_mask {
    KMASK_LEFT = 1 << 0,
    KMASK_RIGHT = 1 << 1,
    KMASK_UP = 1 << 2,
    KMASK_DOWN = 1 << 3,
    KMASK_WEST = 1 << 4,
    KMASK_EAST = 1 << 5,
    KMASK_NORTH = 1 << 6,
    KMASK_SOUTH = 1 << 7,
    KMASK_LT = 1 << 8,
    KMASK_LB = 1 << 9,
    KMASK_RT = 1 << 10,
    KMASK_RB = 1 << 11,
    KMASK_OPTIONS = 1 << 12,
    KMASK_SHARE = 1 << 13,
    KMASK_PS = 1 << 14,
    KMASK_THUMBL = 1 << 15,
    KMASK_THUMBR = 1 << 16,
    KMASK_SIDE_SHIFT = 30,
};

struct state {
    uint32_t keys; // keys_mask
};

#define MAPPINGS_NUM 105
struct mapping g_mapping[MAPPINGS_NUM] = {
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH,    .code = KEY_ENTER },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST,     .code = KEY_SPACE },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH,    .code = KEY_ESC },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST,     .code = KEY_BACKSPACE },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN,     .code = KEY_GRAVE },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT,     .code = KEY_TAB },
    { .first = SIDE_LEFT,   .keys = KMASK_UP,       .code = KEY_CAPSLOCK },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT,    .code = KEY_BACKSLASH },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_DOWN,   .code = KEY_A },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_LEFT,   .code = KEY_B },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_UP,     .code = KEY_C },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_RIGHT,  .code = KEY_D },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_DOWN,    .code = KEY_E },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_LEFT,    .code = KEY_F },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_UP,      .code = KEY_G },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_RIGHT,   .code = KEY_H },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_DOWN,   .code = KEY_I },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_LEFT,   .code = KEY_J },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_UP,     .code = KEY_K },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_RIGHT,  .code = KEY_L },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_DOWN,    .code = KEY_M },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_LEFT,    .code = KEY_N },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_UP,      .code = KEY_O },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_RIGHT,   .code = KEY_P },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_SOUTH,   .code = KEY_Q },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_WEST,    .code = KEY_R },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_NORTH,   .code = KEY_S },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_EAST,    .code = KEY_T },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_SOUTH,   .code = KEY_U },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_WEST,    .code = KEY_V },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_NORTH,   .code = KEY_W },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_EAST,    .code = KEY_X },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_SOUTH,     .code = KEY_Y },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_WEST,      .code = KEY_Z },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_NORTH,     .code = KEY_LEFTBRACE },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_EAST,      .code = KEY_RIGHTBRACE },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_SOUTH,  .code = KEY_COMMA },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_WEST,   .code = KEY_DOT },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_NORTH,  .code = KEY_SEMICOLON },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_EAST,   .code = KEY_APOSTROPHE },
};

bool should_stop = false;

static void sigint_handler(int _value)
{
    (void) _value;
    should_stop = true;
    printf("SIGINT handled\n");
}

static void setup_output_device(int fd) {
    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (ssize_t i = 1; i < KEY_MAX; i++)
        ioctl(fd, UI_SET_KEYBIT, i);
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Example device");
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
}

static void emit(int fd, int type, int code, int val)
{
    struct input_event ie = {
        .type = type,
        .code = code,
        .value = val,
        /* timestamp values below are ignored */
        .time = {
            .tv_sec = 0,
            .tv_usec = 0,
        },
    };
    write(fd, &ie, sizeof(ie));
}

static void emulate_key_press(int ofd, int code)
{
    emit(ofd, EV_KEY, code, 1);
    emit(ofd, EV_SYN, SYN_REPORT, 0);
}

static void emulate_key_release(int ofd, int code)
{
    emit(ofd, EV_KEY, code, 0);
    emit(ofd, EV_SYN, SYN_REPORT, 0);
}

static void emulate_key(int ofd, int code)
{
    emulate_key_press(ofd, code);
    emulate_key_release(ofd, code);
}

static enum side which_side_key(struct input_event ev) {
    if (ev.type == EV_KEY) {
        switch (ev.code) {
        case BTN_SOUTH:
        case BTN_EAST:
        case BTN_WEST:
        case BTN_NORTH:
            return SIDE_RIGHT;
        };
    } else if (ev.type == EV_ABS) {
        /*
         * d-pad is EV_ABS
         * left-right: code ABS_HAT0X, left=-1, right=1
         * down-up: code ABS_HAT0Y, up=-1, down=1
         */
        if (ev.code == ABS_HAT0X || ev.code == ABS_HAT0Y)
            return SIDE_LEFT;
    }
    return SIDE_NO;
}

static enum side which_side_state(struct state state) {
    return (state.keys >> KMASK_SIDE_SHIFT) & 3;
}

static struct state keypress(struct state state, struct input_event ev, int ofd) {
    if (which_side_state(state) == SIDE_NO) {
        const enum side side = which_side_key(ev);
        printf("first = %s\n", side == SIDE_LEFT ? "left" : side == SIDE_RIGHT ? "right" : "no");
        state.keys |= ((uint32_t)side & 3) << KMASK_SIDE_SHIFT;
    }
    if (ev.type == EV_KEY) {
        switch (ev.code) {
        case BTN_SOUTH:
            state.keys |= KMASK_SOUTH;
            break;
        case BTN_EAST:
            state.keys |= KMASK_EAST;
            break;
        case BTN_WEST:
            state.keys |= KMASK_WEST;
            break;
        case BTN_NORTH:
            state.keys |= KMASK_NORTH;
            break;
        case BTN_TL2:
            // Shift
            state.keys |= KMASK_LT;
            emulate_key_press(ofd, KEY_LEFTSHIFT);
            break;
        case BTN_TR2:
            // Control
            state.keys |= KMASK_RT;
            emulate_key_press(ofd, KEY_LEFTCTRL);
            break;
        case BTN_TL:
            // Super
            state.keys |= KMASK_LB;
            emulate_key_press(ofd, KEY_LEFTMETA);
            break;
        case BTN_TR:
            // Alt
            state.keys |= KMASK_RB;
            emulate_key_press(ofd, KEY_LEFTALT);
            break;
        case BTN_SELECT:
            state.keys |= KMASK_SHARE;
            break;
        case BTN_START:
            state.keys |= KMASK_OPTIONS;
            break;
        };
    } else if (ev.type == EV_ABS) {
        /*
         * d-pad is EV_ABS
         * left-right: code ABS_HAT0X, left=-1, right=1
         * down-up: code ABS_HAT0Y, up=-1, down=1
         */
        if (ev.code == ABS_HAT0X) {
            if (ev.value == -1) {
                state.keys |= KMASK_LEFT;
            } else if (ev.value == 1) {
                state.keys |= KMASK_RIGHT;
            }
        } else if (ev.code == ABS_HAT0Y) {
            if (ev.value == -1) {
                state.keys |= KMASK_UP;
            } else if (ev.value == 1) {
                state.keys |= KMASK_DOWN;
            }
        }
    }
    return state;
}

static struct state keyrelease(struct state state, struct input_event ev, int ofd) {
    for (ssize_t i = 0; i < MAPPINGS_NUM; i++) {
        struct mapping mapping = g_mapping[i];
        if (mapping.code && mapping.first == which_side_key(ev) &&
                (state.keys & ~(3 << KMASK_SIDE_SHIFT)) == mapping.keys) {
            emulate_key(ofd, mapping.code);
        }
    }
    if (ev.type == EV_KEY) {
        switch (ev.code) {
        case BTN_SOUTH:
            state.keys &= ~KMASK_SOUTH;
            break;
        case BTN_EAST:
            state.keys &= ~KMASK_EAST;
            break;
        case BTN_WEST:
            state.keys &= ~KMASK_WEST;
            break;
        case BTN_NORTH:
            state.keys &= ~KMASK_NORTH;
            break;
        case BTN_TL2:
            // Shift
            state.keys &= ~KMASK_LT;
            emulate_key_release(ofd, KEY_LEFTSHIFT);
            break;
        case BTN_TR2:
            // Control
            state.keys &= ~KMASK_RT;
            emulate_key_release(ofd, KEY_LEFTCTRL);
            break;
        case BTN_TL:
            // Super
            state.keys &= ~KMASK_LB;
            emulate_key_release(ofd, KEY_LEFTMETA);
            break;
        case BTN_TR:
            // Alt
            state.keys &= ~KMASK_RB;
            emulate_key_release(ofd, KEY_LEFTALT);
            break;
        case BTN_SELECT:
            state.keys &= ~KMASK_SHARE;
            break;
        case BTN_START:
            state.keys &= ~KMASK_OPTIONS;
            break;
        };
    } else if (ev.type == EV_ABS) {
        /*
         * d-pad is EV_ABS
         * left-right: code ABS_HAT0X, left=-1, right=1
         * down-up: code ABS_HAT0Y, up=-1, down=1
         */
        if (ev.code == ABS_HAT0X) {
            state.keys &= ~(KMASK_LEFT | KMASK_RIGHT);
        } else if (ev.code == ABS_HAT0Y) {
            state.keys &= ~(KMASK_UP | KMASK_DOWN);
        }
    }
    if ((state.keys & ~(3 << KMASK_SIDE_SHIFT)) == 0) state.keys &= ~(3 << KMASK_SIDE_SHIFT);
    return state;
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

    setup_output_device(ofd);

    /*
     * On UI_DEV_CREATE the kernel will create the device node for this
     * device. We are inserting a pause here so that userspace has time
     * to detect, initialize the new device, and can start listening to
     * the event, otherwise it will not notice the event we are about
     * to send. This pause is only needed in our example code!
     */
    sleep(1);

    struct state state = {0};

    while (!should_stop) {
        struct input_event ev;
        ssize_t ret = read(ifd, &ev, sizeof(ev));
        if (ret == -1) {
            perror("read");
            exit(1);
        }
        switch (ev.type) {
        case EV_KEY:
            printf("EV_KEY, code=%u, value=%d\n", ev.code, ev.value);
            if (ev.value == 1) {
                state = keypress(state, ev, ofd);
            } else if (ev.value == 0) {
                state = keyrelease(state, ev, ofd);
            }
            break;
        case EV_ABS:
            /*
             * d-pad is EV_ABS
             * left-right: code ABS_HAT0X, left=-1, right=1
             * down-up: code ABS_HAT0Y, up=-1, down=1
             */
            if (ev.code == ABS_HAT0X || ev.code == ABS_HAT0Y) {
                printf("type=%u, code=%u, value=%d\n", ev.type, ev.code, ev.value);
                if (ev.value == 1 || ev.value == -1) {
                    state = keypress(state, ev, ofd);
                } else if (ev.value == 0) {
                    state = keyrelease(state, ev, ofd);
                }
            }
            break;
        default:
            break;
        }
    }

    ioctl(ofd, UI_DEV_DESTROY);
    close(ofd);

    return 0;
}
