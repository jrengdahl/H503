STM32H503RB test board and firmware

This project includes a small OSHPark board for the new H503
microcontroller from ST MIcro, and firmware to run on the board.
The firmware includes OpenMP support for this bare metal microcontroller.


THE CHIP

The STM32H503 is a low-end microcontroller by ST Microelectronics:
-- Cortex-M33 CPU
-- 250 MHz
-- 128K flash
-- 32K RAM
-- typical peripherals for a small micro, including USB Device.
-- ETM trace port

When this chip first became available in early March of 2023, it was in
stock at Mouser for $4.33, and I was able to obtain a few. More recently
I see that they are out of stock with availability nearly a year out.
Try findchips.com to search for availability. Often such chips are more
easily available on a Nucleo board (although the Nucleo boards do not
have trace connectors).

The spec clock for the H503 is 250 Hz. It seems to run reliably up to 400 MHz.


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
-- set the CPU clock to a given frequency
-- execute a single LDR to a given memory location
-- execute a single STR to a given memory location
-- print a summary of the flash and RAM
-- run a benchmark of the thread switcher
-- display the on-chip temperature sensor reading
-- dump the thread stacks
-- display the state of the free memory system
-- run various OpenMP tests
-- set the debug verbodity level
-- print the help

The command line processor is simple and easily expanded with new commands.

The firmware takes about 40K of flash and 24K of RAM.

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
simple non-preemptive threading system, which enables me to architect a
system as a set of cooperating processes. As the concept developed over
many years it has gone by the name of "wait-wake", "plastic fork", and
"not-an-OS", but currently goes by the name of "Bear Metal Threads".
Its characteristics include:

-- very small: the code that gets used at runtime consists of 23 ASM
   instructions. There is a little more for creating threads at powerup. 
-- very fast: a thread switch takes 125 nanoseconds on a 250 MHz M33.
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

The following OpenMP features are working:
-- parallel
-- parallel for
-- single
-- task
-- firstprivate

I have started migrating the powerup code and command line interpreter
to using OpenMP primitives rather than the older threading calls to
spawn new threads.

This is experimental and ongoing.


THE GCC TOOLCHAIN

There are two reasons this firmware requires a special toolchain:

-- The threading system reserves r9 for the preempted thread stack. The
entire firmware must be compiled with the GCC flag "-ffixed-r9", so that
user code does not clobber r9. According to the AAPCS specification, r9
should be untouched by libraries. I have never seen a precompiled ibrary
that observes this rule. Fortunately, the ST-supplied code is always
compiled along with your firmware. Unfortunately, the precompiled newlib
that comes with the standard ARM bare metal GCC toolchain does not
follow the rule and clobbers r9.

-- GCC includes OpenMP, however it is only enabled for platforms that
have a large OS, such as Linux or Windows. The bare metal versions of
the toolchain do not have OpenMP enabled. My build of the GCC bare metal
toolchain has OpenMP enabled.

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
-- SPI flash

Otherwise the board is very minimal: power regulator, crystal
oscillator, connectors, a reset button, the BOOT0 button is also an GPIO
input, and one LED. I put a Winbond SPI flash on it because I want to
learn how to use them.

The microcontroller pinout is captured in a CubeMX project,
H503.kicad/H503.ioc. This is a sample CubeMX project I used as reference
for the board design, which shows a possible maximum usage of the pins.
The separate ./H503.ioc used by the firmware project only enables the
pins that are currently supported by the firmware.

Building the board is not hard. I do my soldering under a 1959
StereoZoom microscope using a very fine-tip pencil, and SMT solder paste
applied with a toothpick, or wire solder for through-hole parts. One
thing to watch out for is the USB connector. If you use too little
solder you risk ripping the connector off the board if you are not real
careful with the USB cable. If you use too much solder it can puddle
under the connector and short out the signal pins.

There is a newer version of the board that was manufactured by JLCPCB.
All the small commodity parts are located on the bottom side and are
soldered by JLCPCB. The CPU, connectors, and a couple parts that are
not sourced by JLCPCB are on the top side and have to be hand soldered.


TRACE

One of my retirement presents to myself was a Segger J-Trace probe, so
I put a 20-pin trace port on this board rather than the usual four-pin
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

In order to use ETM trace with Cortex-M4 and Cortex-M33 chips, the
following is required:

-- a chip that has trace port or on-chip trace buffer. As far as I know,
all STM32 in 100 pin packages or larger have trace ports. Trace ports on
smaller pinouts are less common. There is no easy way to find which chips
have trace capability, other than reading many data sheets. So far I
have found the STM32H503 and STM32L412 in LQFP-64, and the L412 in
LQFP-32, have 5-pin trace ports. In fact, the L412 seems to be the only
STM32 in LQFP-32 that has a trace port.

Some Cortex-M7 chips from ST Micro have 2K or 4K on-chip trace buffers.
These can be used with Ozone and an inexpensive J-Link Edu, though the
trace depth is limited.

-- a trace probe (unless you have an on-chip trace buffer). Trace probes
are expensive. I am familiar with Lauterbach, which costs in the
vicinity of $10,000. My Segger J-Trace was around $1800. In the UK or EU
check out pdqlogic.com, but they won't ship to the USA. Some people have
used inexpensive USB logic analyzers with the Sigrok software to capture
trace, but such a solution is harder to use
(https://essentialscrap.com/tips/arm_trace).

-- a trace capable debugger. I use Segger Ozone, which is included 
with J-Trace, or it is free for non-commercial use.

-- you have to allocate from two to five pins for the trace port. I have
always used the full five pins.

-- Your board has to have a trace port connector. The traces from the
chip to the connector should be laid out with high speed signalling in
mind. In theory, it might be possible to wire a trace probe to I/O pins
of a Nucleo board, but I would not expect this to work well at fast
clock rates. Though the H503 can run up to 250 MHz (400 MHz overclocked),
on my board the trace becomes flakey above 100 MHz.

-- You need to configure the trace pins. There are two ways to do this:

     -- your software can setup the pins and the ETM at powerup.
        Currently my L412 software does it this way. I based my ETM
        setup code on this project:
        https://github.com/PetteriAimonen/STM32_Trace_Example

     -- You can obtain or write an Ozone script to setup the trace
        pins. Once the pins are setup, Ozone can deal with the ETM and
        TPIU on its own. Segger provides compiled (.pex) scripts for
        certain chips. If your chip is not supported, or you have used a
        different pinout than Segger used, you either have to get them
        to create a .pex for you, or you can write your own script. This
        project includes a redistributable .jdebug script which sets up
        the pins for the H503.

   Having tried both methods, using the script works a lot better. Note
   that the H503 uses a different ETM than the L412. The L412 code will
   not work for the H503. It would be fairly difficult to research and
   write the ETM-M33 setup code, since you not only have to deal with
   the pins, but also the ETM and TPIU, for which I could find no
   easy-to-use documentation or example code. Also, when using the
   script, the trace starts at powerup, whereas if your software sets up
   the ETM, the trace only works after your code runs, and you have to
   stop and restart your program after the setup code before Ozone
   starts receiving trace data.


RTOS-AWARE TRACE

Segger's Ozone debugger can detect RTOS thread switches in order to make
the trace easier to read and understand. If you are using a supported
RTOS they have RTOS-awareness modules you can download and use. If your
RTOS is not supported, or home-brewed like mine, you can tell Ozone
about the thread switching code in the Ozone startup script. The .jdebug
script included with this project does that. There are two prequisites:
your thread switch code must exist in a subroutine call that Ozone can
find, and you have to use Dwarf-4 debug info. Newer editions of GCC,
such as 12.2, emit Dwarf-5 debug data by default, so you have to pass
-gdwarf-4 to the GCC and G++ compilers.

(There is a new version of Ozone that might support Dwarf-5, but I have not
tried Dwarf-5 yet).
