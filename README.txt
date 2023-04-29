STM32H503RB test board

This project includes a small OSHPark board for the new H503
microcontroller from ST MIcro, and firmware to run on the board.


THE CHIP

The STM32H503 is a low-end microcontroller by ST Microelectronics:
-- Cortex-M33 CPU
-- 250 MHz
-- 128K flash
-- 32K RAM
-- typical peripherals for a small micro, including USB Device.
-- ETM trace port


THE FIRMWARE

The firmware build is managed by an STM32CubeIDE project. The skeleton
of the project was auto-generated from the H503.ioc file. My firmware is
a simple command line interpeter with the following commands:

-- dump memory
-- modify memory
-- fill memory
-- RAM test
-- input from a pin
-- output to a pin
-- calibrate the CPU clock (to make sure the crystal, PLL, and spin-delay loop are correct)
-- execute a single LDR to a given memory location
-- execute a single STR to a given memory location
-- print a summary of the flash and RAM
-- print the help

The command line processor is simple and easily expanded with new commands.

For similar L412 and H743 projects, rather than using an FTDI
USB-to-serial cable, or an on-board USB-to-serial chip, I decided to
learn how to use the native on-chip USB for the serial console. I got a
lot of help from Jean Roch's tutorial on using the STM32 USB CDC
library:
https://nefastor.com/microcontrollers/stm32/usb/stm32cube-usb-device-library

When I went to port this firmware over to the H503 I found that, since
the chip is very new, the USB_Device library is not yet available. With
some help from ST tech support, I was able to borrow code from similar
chips (the H743 and L552), and adapt it to the H503.


THREADS

I usually write bare metal firmware, with no OS or RTOS, but I use a
simple non-preemtive threading system, which enables me to architect a
system as a set of cooperating processes. As the concept developed over
many years it has gone by the name of "wait-wake", "plastic fork", and
"not-an-OS", but currently goes by the name of "Bear Metal Threads".
Its characteristics include:

-- very small: the core consists of 23 assembly instructions, plus a
   little more for the creation of new threads. 
-- very fast: a thread switch takes about 130 nanoseconds on a 250 MHz M33.
-- non-preemptive: there is no scheduler.
-- threads can run at interrupt level, which can provide prioritization.


OPENMP

One reason I created this project is as a platform to experiment with
bare metal OpenMP. 

OpenMP is a parallel processing extension for C/C++ and Fortran that is
built into GCC. It can be used on modern multi-core PCs running Windows
and Linux. It is most commonly associated with massively parallel
supercomputers such as Frontier and Fugaku (https://www.top500.org). I
aim to show that it can also be useful for multi-threaded and parallel
programming on single and multi-core embedded processors.

This is experimental and ongoing.


THE GCC TOOLCHAIN

There are two reason this firmware requires a special toolchain:

-- The threading system reserves r9 for the preempted thread stack. The
entire firmware must be compiled with the GCC flag "-ffixed-r9", so that
user code does not clobber r9. According to the AAPCS specification, r9
should be untouched by libraries. I have never seen a precompiled ibrary
that observes this rule. Fortunately, the ST-supplied code is always
compiled along with your firmware. Unfortunately, the precompiled newlib
that comes with the standard ARM bare metal GCC toolchain does not
follow the rule and clobbers r9.

-- GCC includes OpenMP, however it is only enabled for platforms that
have a large OS, such as Linux or Windows. The bare metal version of the
toolchain does not have OpenMP enabled. My bare metal toolchain has
OPenMP enabled.

The Cortex-M33 GCC 12.2 toolchain to be used with this project is found at:
https://github.com/jrengdahl/cross/releases


THE BOARD

The KiCAD project for the board is included, or you can order three
boards from OSHPark for $16.
https://oshpark.com/shared_projects/t9n5Q26a

The board uses the Arduino Nano form factor, but is only partially
pin-compatible. It will fit into a Nano socket, such as this:
https://www.amazon.com/gp/product/B095XVVGQ8. Since the H503 is only
available in a 64-pin LQFP at the moment, in order to fit it on the
board I had to turn it 45 degrees, and delete the middle three pins from
each row of the connector. On the other hand, all pins on the
microcontroller either have a job, are brought out to pins, or are
brought out to solderable test points.

Interesting things about the board are:
-- Arduino Nano form factor
-- can be powered from the USB port
-- uses the on-chip USB for a virtual serial console
-- trace port

Otherwise the board is very minimal: power reulator, crystal oscillator,
connectors, a reset button, the BOOT0 button is also an GPIO input, and
one LED.

The microcontroller pinout is recorded in a CubeMX project,
H503.kicad/H503.ioc. This is a sample CubeMX project I used as reference
for the board design, which shows a possible maximum usage of the pins.
The separate H503.ioc used by the firmware project only enables the pins
that are currently supported by the firmware.

One of my retirement presents to myself was a Segger J-Trace probe, so I
put a 20-pin trace port on this board rather than the usual four-pin
Serial Wire Debug port. If you don't have a trace probe you can populate
a 10-pin connector instead, and use a J-link or ST-Link probe. (You
can't fit a 10-pin connector onto a 20 pin header because the end of the
connector won't fit between the 5th and 6th pairs of pins).

One reason I prefer the M7 processors is that they include an on-chip
trace buffer, and a trace probe is not required. I use the free Segger
Ozone debugger, which can display trace for either on-chip or off-chip
trace. Unfortunately, since COVID, M7 chips are nearly impossible for a
hobbyist to obtain. That is why I have been focusing on the L412, and
more recently the H503. (Try lcsc.com if you need M7 chips, I found some
affordable H7B0).

Trace is not working yet.

Building the board is not hard. I do my soldering under a 1959
StereoZoom microscope using a very fine-tip pencil, and SMT solder paste
applied with a toothpick, or wire solder for through-hole parts. One
thing to watch out for is the USB connector. If you use too little
solder you risk ripping the connector off the board if you are not real
careful with the USB cable. If you use too much solder it can puddle
under the connector and short out the signal pins.

