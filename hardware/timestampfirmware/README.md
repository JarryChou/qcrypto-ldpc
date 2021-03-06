Hardware {#hardware}
====

This README is located in `qcrypto/hardware/timestampfirmware`.

This directory contains the the firmware for a Cypress FX2 chip, together
with some support files for the usb part of the timestamp unit.

The files in this directory are:

 * **Makefile**           used to generate the firmware file in various formats,
                    and also to generate all the memory map and direct
		    assembler files. Generates hexcode which can be uploaded
		    into the FX2 chip with the upload script.

 * **fifobithelpers.h**  Some definitions which lines of the core timestamp unit
                    are connected to the fx2 chip

 * **fx2regs_c.h**        external file for fx2 register definitions 
                    from Christer Weinigel, Nordnav Technologies A

 * **tp8.c**              The firmware code itself

 * **upload**             A loader script to get the firmware into an unprogrammed
                    FX2 chip. A modified code can be used to flash an EEPROM
		    but this requires the Vend_Ax.hex file from the FX2
		    development kit and I have no idea about the license

 * **usbtimetagio.h**     Header file, mainly for ioctl definitions for a higher
                    level usb driver on a host machine, but reused here for
		    command parsing in the EP1 interpreter function.


PREREQUISITS:

The code is written for a sdcc compiler, which should be installed on the
compiling system, probably together with the corresponding binutils.


COMPILATION:

A modified version of the code can be generated via the makefile; for the code
to work properly, it is necessary to check that the descriptor tables in the
final code end up on an even address. This information is extracted out of the
.rst files when calling the make command. In case they end up on an odd
address, the filler variable xuxu needs to be adjusted accordingly.

UPLOADING:

The upload script loads the firmware in a pristine FX2 chip, which then
re-enumberates as the new USB device. The code can also loaded into the
external EEPROM.

USB ISSUES:

The code uses a somewhat dirty USB device identification by retaining the
vendor ID for the FX2 (Cypress, ID 0x04b4), and a phantasy device identifier 
(namely 0x1234). If Cypress decides to assign that particular device at some
point, there  may be a conflict with this firmware.

