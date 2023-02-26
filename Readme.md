# Dualshock 4 Keyboard
> Use your DS4 gamepad as a keyboard with touchpad while laying on the couch!

This is a proof of concept implementation of virtual keyboard device that
enables you to press any of these keyboard buttons with Dualshock 4 gamepad. It
currently works only on Linux and have non-trivial way to set up and run.

## Installation on Linux

You will need a C compiler (GCC or Clang) and `make` utility. Download and
unpack zip archive of this repo or clone it with git. Then run the following
from the root of the downloaded repo directory:

```
make
```

It will produce a binary named `main`.

## Run

First of all you need to be able to open `/dev/uinput` as a user, unless you
decided to do everything from root. Do this for your `user` as root if you want
to be able to run the virtual keyboard as user:

```
chown user:user /dev/uinput
```

### USB

Open up a file `/proc/bus/input/events` with some text editor or viewer. For example:

```
less /proc/bus/input/devices
```

Then find there an entry for `"Sony Interactive Entertainment Wireless Controller"`. It will look like this:

```
I: Bus=0003 Vendor=054c Product=09cc Version=8111
N: Name="Sony Interactive Entertainment Wireless Controller"
P: Phys=usb-0000:02:00.0-1.4.2.4/input3
S: Sysfs=/devices/pci0000:00/0000:00:01.2/0000:02:00.0/usb1/1-1/1-1.4/1-1.4.2/1-1.4.2.4/1-1.4.2.4:1.3/0003:054C:09CC.000B/input/input27
U: Uniq=d0:bc:c1:a2:be:9f
H: Handlers=js0 event23
B: PROP=0
B: EV=1b
B: KEY=7fdb000000000000 0 0 0 0
B: ABS=3003f
B: MSC=10
```

If you don't see any entries for "Sony Interactive Entertainment something something..." and you are sure that your USB cable is good, then go mess around with your kernel and find a way to enable DS4 support in the kernel.

The line `H: Handlers=js0 event23` is what we need. `event23` is the gamepad event source that is gonna be intercepted and substituted but the program. You may end up with different number in this file name. You must use the proper file name you've got from `/proc/bus/input/devices` in the following commands instead of `event23`.

Once again, if you are planning to use virtual keyboard as `user`, run this as root, to allow `user` to access the event source:

```
chown user:user /dev/input/event23
```

Then run the program like this:

```
./main /dev/input/event23
```

### Bluetooth

TODO: Describe how to run when the gamepad is connected via Bluetooth

## Usage 

Now you can issue keyboard keypresses by pressing some combinations of the gamepad buttons as shown in the following chart:

![Key mapping](layout.svg)

The key press is registered when you let go at least one of the buttons in the combination (chord) you pressed. So you first press the desired chord and then release all the buttons to commit a keypress that corresponds to the chord.

## Meta

Authors:
- Vladimir Novikov – oxore@protonmail.com

This is free and unencumbered software released into the public domain. See
``UNLICENSE`` for more information.

<!-- Markdown link & img dfn's -->
[readme-template]: https://github.com/dbader/readme-template
