MyLib

/*
 * Copyright (c) 2009, 2023 Jonathan Engdahl
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * (This is the BSD license, see http://opensource.org/licenses/BSD-3-Clause)
 *
 */

These routines are replacements for some common library routines. They
are intended for use in very small firmwares on microprocessors with
limited firmware space, such as 64 Kbytes. The routines have the
following properties:

-- They are smaller than the versions included in glibs or newlib.

-- They do not pull through other library routines.

-- They are so simple that they did not have to be debugged. They
   were first developed on a platform with very limited debug
   support, so it was important that they worked the first time.

-- They are not necessarily fast; size and simplicity are more important.

-- They may be subsets of the POSIX version.

Use the header files that come with your compiler and library.

If you notice that the 64K flash of your microcontroller is suddenly
half filled with 32K of library routines, find which routine you called
is pulling in the stuff you don't need. For example, calling printf
pulls in a TON of other routines. Once you hae identified the culprit,
write a simple replacement that implements only the functionality you
need, and add it to this library.

The original version of this code was developed in 2009 for a home
project that was intended to result in a digital controller for my
trolling motor that had a burned out speed control switch. The
controller was a Luminary Micro Cortex-M3 eval board. The motor driver
was made for RC model planes. It worked fine on dry land, but when I got
it into the water it went poof after about a minute and the magic smoke
came out. Evidently the motor's resistance and current were compatible
with the RC drive circuit, but the inductance of the trolling motor was
a lot more than an airplane motor. Under load, the inductive kick took
out the catch diodes.

The motor controller wound up in the scrap pile, I got a new mechnical
switch for the boat motor and gave it away, but the firmware lives on,
and on, and on. It found many uses, including some products of my
employer. The files here are derived from the original code I wrote for
the Luminary Micro Cortex-M3, or are newly written since I retired.
