/* SPDX-License-Identifier: Unlicense
 */

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

// TODO Impl repeating key when right thumb-stick tilted enough in any way
// TODO Impl switching between keyboard and regular mode by BTN_MODE (the PS key)

/* Analog Binarization Threshold */
#define ABT 64
/* Analog Binarization Hysteresis */
#define ABH 20

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
    KMASK_THUMBL_DOWN = 1 << 17,
    KMASK_THUMBL_UP = 1 << 18,
    KMASK_THUMBL_LEFT = 1 << 19,
    KMASK_THUMBL_RIGHT = 1 << 20,
    KMASK_THUMBR_DOWN = 1 << 21,
    KMASK_THUMBR_UP = 1 << 22,
    KMASK_THUMBR_LEFT = 1 << 23,
    KMASK_THUMBR_RIGHT = 1 << 24,
    KMASK_PRESSED = 1 << 29,
    KMASK_SIDE_SHIFT = 30,
    KMASK_CHORD_KEYS =
        KMASK_RIGHT | KMASK_LEFT | KMASK_UP | KMASK_DOWN | KMASK_WEST |
        KMASK_EAST | KMASK_NORTH | KMASK_SOUTH,
};

struct state {
    uint32_t keys; // keys_mask
};

#define MAPPINGS_NUM 105
struct mapping g_mapping[MAPPINGS_NUM] = {
    /* Right single */
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST,     .code = KEY_BACKSPACE },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH,    .code = KEY_ENTER },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH,    .code = KEY_SPACE },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST,     .code = KEY_ESC },
    /* Left single */
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN,     .code = KEY_COMPOSE },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT,     .code = KEY_TAB },
    { .first = SIDE_LEFT,   .keys = KMASK_UP,       .code = KEY_DELETE},
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT,    .code = KEY_CAPSLOCK },
    /* Right single to left single */
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_LEFT,    .code = KEY_H },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_DOWN,    .code = KEY_J },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_UP,      .code = KEY_K },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_RIGHT,   .code = KEY_L },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_LEFT,   .code = KEY_G },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_DOWN,   .code = KEY_B },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_UP,     .code = KEY_N },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_RIGHT,  .code = KEY_M },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_LEFT,   .code = KEY_O },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_DOWN,   .code = KEY_P },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_UP,     .code = KEY_LEFTBRACE },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_RIGHT,  .code = KEY_RIGHTBRACE },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_LEFT,    .code = KEY_COMMA },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_DOWN,    .code = KEY_DOT },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_UP,      .code = KEY_SEMICOLON },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_RIGHT,   .code = KEY_APOSTROPHE },
    /* Left single to right single */
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_WEST,    .code = KEY_A },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_SOUTH,   .code = KEY_S },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_NORTH,   .code = KEY_D },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_EAST,    .code = KEY_F },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_WEST,      .code = KEY_Q },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_SOUTH,     .code = KEY_W },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_NORTH,     .code = KEY_E },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_EAST,      .code = KEY_R },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_WEST,    .code = KEY_Z },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_SOUTH,   .code = KEY_X },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_NORTH,   .code = KEY_C },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_EAST,    .code = KEY_V },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_WEST,   .code = KEY_T },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_SOUTH,  .code = KEY_Y },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_NORTH,  .code = KEY_U },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_EAST,   .code = KEY_I },
    /* Right double to left single */
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_SOUTH | KMASK_LEFT,   .code = KEY_F5 },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_SOUTH | KMASK_UP,     .code = KEY_F6 },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_SOUTH | KMASK_DOWN,   .code = KEY_F7 },
    { .first = SIDE_RIGHT,  .keys = KMASK_WEST | KMASK_SOUTH | KMASK_RIGHT,  .code = KEY_F8 },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_EAST | KMASK_LEFT,   .code = KEY_F9 },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_EAST | KMASK_UP,     .code = KEY_F10 },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_EAST | KMASK_DOWN,   .code = KEY_F11 },
    { .first = SIDE_RIGHT,  .keys = KMASK_SOUTH | KMASK_EAST | KMASK_RIGHT,  .code = KEY_F12 },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_WEST | KMASK_LEFT,   .code = KEY_F1 },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_WEST | KMASK_UP,     .code = KEY_F2 },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_WEST | KMASK_DOWN,   .code = KEY_F3 },
    { .first = SIDE_RIGHT,  .keys = KMASK_NORTH | KMASK_WEST | KMASK_RIGHT,  .code = KEY_F4 },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_LEFT,   .code = KEY_HOME },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_UP,     .code = KEY_PAGEUP },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_DOWN,   .code = KEY_PAGEDOWN },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_RIGHT,  .code = KEY_END },
    /* Left double to right single */
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_DOWN | KMASK_WEST,    .code = KEY_5 },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_DOWN | KMASK_SOUTH,   .code = KEY_6 },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_DOWN | KMASK_NORTH,   .code = KEY_7 },
    { .first = SIDE_LEFT,   .keys = KMASK_LEFT | KMASK_DOWN | KMASK_EAST,    .code = KEY_8 },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_LEFT | KMASK_WEST,      .code = KEY_1 },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_LEFT | KMASK_SOUTH,     .code = KEY_2 },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_LEFT | KMASK_NORTH,     .code = KEY_3 },
    { .first = SIDE_LEFT,   .keys = KMASK_UP | KMASK_LEFT | KMASK_EAST,      .code = KEY_4 },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_RIGHT | KMASK_WEST,    .code = KEY_9 },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_RIGHT | KMASK_SOUTH,   .code = KEY_0 },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_RIGHT | KMASK_NORTH,   .code = KEY_MINUS },
    { .first = SIDE_LEFT,   .keys = KMASK_DOWN | KMASK_RIGHT | KMASK_EAST,    .code = KEY_EQUAL },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_UP | KMASK_WEST,   .code = KEY_GRAVE },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_UP | KMASK_SOUTH,  .code = KEY_BACKSLASH },
    { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_UP | KMASK_NORTH,  .code = KEY_SLASH },
    // { .first = SIDE_LEFT,   .keys = KMASK_RIGHT | KMASK_UP | KMASK_EAST,   .code = KEY_ },
    /* Right double to left double */
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_LEFT | KMASK_DOWN,     .code = KEY_INSERT },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_UP | KMASK_LEFT,       .code = KEY_SCROLLLOCK },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_DOWN | KMASK_RIGHT,    .code = KEY_PAUSE },
    { .first = SIDE_RIGHT,  .keys = KMASK_EAST | KMASK_NORTH | KMASK_RIGHT | KMASK_UP,      .code = KEY_SYSRQ },
    /* Implicily used modifiers and other keys that must be registered via ioctl(UI_SET_EVBIT) */
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_LEFTSHIFT },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_RIGHTSHIFT },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_LEFTALT },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_RIGHTALT },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_LEFTCTRL },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_RIGHTCTRL },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_LEFTMETA },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_RIGHTMETA },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_LEFT },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_RIGHT },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_UP },
    { .first = SIDE_NO,     .keys = 0,  .code = KEY_DOWN },
};

bool should_stop = false;
int ifd = -1;

static void sigint_handler(int _value)
{
    (void) _value;
    should_stop = true;
    printf("Received SIGINT, quitting...\n");
    {
        int ret = ioctl(ifd, EVIOCGRAB, (void *)0);
        if (ret != 0) {
            fprintf(stderr, "Ungrab failed: ");
            perror("ioctl(ifd, EVIOCGRAB, 0)");
            exit(1);
        }
    }
    // Set default handler and re-issue the signal to unblock the read(3p) call
    signal(SIGINT, SIG_DFL);
    kill(0, SIGINT);
}

static void setup_output_device(int fd) {
    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (ssize_t i = 1; i < MAPPINGS_NUM; i++) {
        if (-1 == ioctl(fd, UI_SET_KEYBIT, g_mapping[i].code)) {
            fprintf(
                    stderr,
                    "ioctl(%d, UI_SET_KEYBIT, %u) = -1, errno=%d: ",
                    fd,
                    g_mapping[i].code,
                    errno);
            perror("");
        }
    }
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
        // TODO timestamp?
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
    printf("-> EV_KEY, code=%u, value=%d\n", code, 1);
}

static void emulate_key_release(int ofd, int code)
{
    emit(ofd, EV_KEY, code, 0);
    emit(ofd, EV_SYN, SYN_REPORT, 0);
    printf("-> EV_KEY, code=%u, value=%d\n", code, 0);
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
    printf("<- %s, code=%u, value=%d\n", ev.type == EV_KEY ? "EV_KEY" : "EV_ABS", ev.code, ev.value);
    if (which_side_state(state) == SIDE_NO) {
        const enum side side = which_side_key(ev);
        printf("first = %s\n", side == SIDE_LEFT ? "left" : side == SIDE_RIGHT ? "right" : "no");
        state.keys |= ((uint32_t)side & 3) << KMASK_SIDE_SHIFT;
    }
    if (ev.type == EV_KEY) {
        switch (ev.code) {
        case BTN_SOUTH:
            state.keys |= KMASK_SOUTH;
            state.keys |= KMASK_PRESSED;
            break;
        case BTN_EAST:
            state.keys |= KMASK_EAST;
            state.keys |= KMASK_PRESSED;
            break;
        case BTN_WEST:
            state.keys |= KMASK_WEST;
            state.keys |= KMASK_PRESSED;
            break;
        case BTN_NORTH:
            state.keys |= KMASK_NORTH;
            state.keys |= KMASK_PRESSED;
            break;
        case BTN_TL2:
            // Control
            state.keys |= KMASK_LT;
            emulate_key_press(ofd, KEY_LEFTCTRL);
            break;
        case BTN_TR2:
            // Shift
            state.keys |= KMASK_RT;
            emulate_key_press(ofd, KEY_LEFTSHIFT);
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
        case BTN_MODE:
            state.keys |= KMASK_PS;
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
                state.keys |= KMASK_PRESSED;
            } else if (ev.value == 1) {
                state.keys |= KMASK_RIGHT;
                state.keys |= KMASK_PRESSED;
            }
        } else if (ev.code == ABS_HAT0Y) {
            if (ev.value == -1) {
                state.keys |= KMASK_UP;
                state.keys |= KMASK_PRESSED;
            } else if (ev.value == 1) {
                state.keys |= KMASK_DOWN;
                state.keys |= KMASK_PRESSED;
            }
        } else if (ev.code == ABS_X) {
            if (ev.value == -1) {
                emulate_key_press(ofd, KEY_LEFT);
                state.keys |= KMASK_THUMBL_LEFT;
            } else if (ev.value == 1) {
                emulate_key_press(ofd, KEY_RIGHT);
                state.keys |= KMASK_THUMBL_RIGHT;
            }
        } else if (ev.code == ABS_Y) {
            if (ev.value == -1) {
                emulate_key_press(ofd, KEY_UP);
                state.keys |= KMASK_THUMBL_UP;
            } else if (ev.value == 1) {
                emulate_key_press(ofd, KEY_DOWN);
                state.keys |= KMASK_THUMBL_DOWN;
            }
        } else if (ev.code == ABS_RX) {
            if (ev.value == -1) {
                state.keys |= KMASK_THUMBR_LEFT;
            } else if (ev.value == 1) {
                state.keys |= KMASK_THUMBR_RIGHT;
            }
        } else if (ev.code == ABS_RY) {
            if (ev.value == -1) {
                state.keys |= KMASK_THUMBR_UP;
            } else if (ev.value == 1) {
                state.keys |= KMASK_THUMBR_DOWN;
            }
        }
    }
    return state;
}

static struct state keyrelease(struct state state, struct input_event ev, int ofd) {
    printf("<- %s, code=%u, value=%d\n", ev.type == EV_KEY ? "EV_KEY" : "EV_ABS", ev.code, ev.value);
    if (state.keys & KMASK_PRESSED) {
        for (ssize_t i = 0; i < MAPPINGS_NUM; i++) {
            struct mapping mapping = g_mapping[i];
            if (mapping.code && mapping.first == which_side_state(state) &&
                    (state.keys & KMASK_CHORD_KEYS) == mapping.keys) {
                emulate_key(ofd, mapping.code);
            }
        }
    }
    if (ev.type == EV_KEY) {
        switch (ev.code) {
        case BTN_SOUTH:
            state.keys &= ~KMASK_SOUTH;
            state.keys &= ~KMASK_PRESSED;
            break;
        case BTN_EAST:
            state.keys &= ~KMASK_EAST;
            state.keys &= ~KMASK_PRESSED;
            break;
        case BTN_WEST:
            state.keys &= ~KMASK_WEST;
            state.keys &= ~KMASK_PRESSED;
            break;
        case BTN_NORTH:
            state.keys &= ~KMASK_NORTH;
            state.keys &= ~KMASK_PRESSED;
            break;
        case BTN_TL2:
            // Control
            state.keys &= ~KMASK_LT;
            emulate_key_release(ofd, KEY_LEFTCTRL);
            break;
        case BTN_TR2:
            // Shift
            state.keys &= ~KMASK_RT;
            emulate_key_release(ofd, KEY_LEFTSHIFT);
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
            /* D-pad takes part in chords too */
            state.keys &= ~KMASK_PRESSED;
        } else if (ev.code == ABS_HAT0Y) {
            state.keys &= ~(KMASK_UP | KMASK_DOWN);
            /* D-pad takes part in chords too */
            state.keys &= ~KMASK_PRESSED;
        } else if (ev.code == ABS_X) {
            if (state.keys & KMASK_THUMBL_LEFT)
                emulate_key_release(ofd, KEY_LEFT);
            else if (state.keys & KMASK_THUMBL_RIGHT)
                emulate_key_release(ofd, KEY_RIGHT);
            state.keys &= ~(KMASK_THUMBL_LEFT | KMASK_THUMBL_RIGHT);
        } else if (ev.code == ABS_Y) {
            if (state.keys & KMASK_THUMBL_UP)
                emulate_key_release(ofd, KEY_UP);
            else if (state.keys & KMASK_THUMBL_DOWN)
                emulate_key_release(ofd, KEY_DOWN);
            state.keys &= ~(KMASK_THUMBL_UP | KMASK_THUMBL_DOWN);
        } else if (ev.code == ABS_RX) {
            state.keys &= ~(KMASK_THUMBR_LEFT | KMASK_THUMBR_RIGHT);
        } else if (ev.code == ABS_RY) {
            state.keys &= ~(KMASK_THUMBR_UP | KMASK_THUMBR_DOWN);
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
    ifd = open(input_path, O_RDONLY);
    if (ifd == -1) {
        fprintf(stderr, "\"%s\": ", input_path);
        perror("open");
        exit(1);
    }
    {
        int ret = ioctl(ifd, EVIOCGRAB, (void *)1);
        if (ret != 0) {
            fprintf(stderr, "Grab failed: ");
            perror("ioctl(ifd, EVIOCGRAB, 1)");
            exit(1);
        }
    }
    int ofd = open("/dev/uinput", O_WRONLY);
    if (ofd == -1) {
        fprintf(stderr, "\"/dev/uinput\": ");
        perror("open");
        exit(1);
    }
    signal(SIGINT, sigint_handler);

    setup_output_device(ofd);

    struct state state = {0};
    int8_t abs_previous[ABS_CNT] = {0};

    while (!should_stop) {
        struct input_event ev;
        ssize_t ret = read(ifd, &ev, sizeof(ev));
        if (ret == -1) {
            perror("read");
            exit(1);
        }
        switch (ev.type) {
        case EV_KEY:
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
                if (ev.value == 1 || ev.value == -1) {
                    state = keypress(state, ev, ofd);
                } else if (ev.value == 0) {
                    state = keyrelease(state, ev, ofd);
                }
            } else if (ev.code == ABS_X || ev.code == ABS_Y || ev.code == ABS_RX || ev.code == ABS_RY) {
                /*
                 * Center stick position is about 127 or 128 on any axis.
                 * Gonna convert these values to be consistent with d-pad
                 * "analog" values.
                 */
                if ((ev.value - INT8_MAX) > (ABT + ABH)) {
                    ev.value = 1;
                } else if ((ev.value - INT8_MAX) < -(ABT + ABH)) {
                    ev.value = -1;
                } else if (((ev.value - INT8_MAX) > -(ABT - ABH)) &&
                        ((ev.value - INT8_MAX) < (ABT - ABH))) {
                    ev.value = 0;
                } else {
                    break;
                }
                /* Some filtering with abs_previous to mitigate duplicate events */
                if (abs_previous[ev.code] != ev.value) {
                    if (ev.value == 1 || ev.value == -1) {
                        if (abs_previous[ev.code] != 0) {
                            const int8_t value = ev.value;
                            ev.value = 0;
                            state = keyrelease(state, ev, ofd);
                            ev.value = value;
                        }
                        state = keypress(state, ev, ofd);
                        abs_previous[ev.code] = ev.value;
                    } else if (ev.value == 0) {
                        state = keyrelease(state, ev, ofd);
                        abs_previous[ev.code] = ev.value;
                    }
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
