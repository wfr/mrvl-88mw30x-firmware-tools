# mrvl-88mw30x-firmware-tools

## Marvell 88MW300/302 Wi-Fi Microcontroller
Marvell Semiconductor’s 88MW30x is a series of ARM-based microcontroller SoCs.

 * [Product page](https://origin-www.marvell.com/microcontrollers/88MW300/302/)
 * [Data sheet](https://www.marvell.com/microcontrollers/assets/MV-S109936-01C.pdf)
 * [SDK EZ-Connect Lite](http://marvell-iot.github.io/)

The 88MW300 contains 4 types of firmware:

 * boot
 * appfw -- the focus of this project.
 * wififw
 * mcufw

Marvell's official SDK is mostly based on free software. However the final step
in the build chain employs a proprietary tool called `axf2firmware` that
converts the app firmware into an undocumented binary format. An excellent, initial
investigation of this file format was
[conducted by Uri Shaked](https://hackernoon.com/inside-the-bulb-adventures-in-reverse-engineering-smart-bulb-firmware-1b81ce2694a6).

This project aims to provide documentation and simple tools for converting the
app firmware back to an ELF format, that be loaded into static analysis tools
for further investigation.

## Tools
 * `axf2firmware` replicates its proprietary counterpart.
 * `firmware2elf` reconstructs an ELF file.
 * `fwinfo`

## File format
Firmware files begin with a header.
The byte order is Little-Endian.

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Magic marker: "MRVL"                                          |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | Unknown1: 0x2E9CF17B                                          |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | Creation time                                                 |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | Number of segments                                            |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | ELF version                                                   |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|

                            Firmware header
           Note that one tick mark represents one bit position.

     Magic marker: const BYTE[4]
         "MRVL"

     Unknown1: const DWORD
         0x2E9CF17B
         Hardcoded in the official SDK. Might refer to SDK version.

     Creation time: DWORD
         UNIX timestamp (local timezone)

     Number of segments: DWORD
         Number of program segments.
         Must be <= 9.

     ELF version:
         identical to Elf32_EHdr.e_version

This header is followed by *N* segment headers.

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Segment type: always 2                                        |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | Offset                                                        |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | Size                                                          |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | Virtual address                                               |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
    | CRC-32 checksum                                               |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|

                            Segment header
           Note that one tick mark represents one bit position.


     Segment type: DWORD
         Hardcoded as 0x2 in axf2firmware

     Offset: DWORD
         Location of segment data in this file

     Size: DWORD
         Size of segment data.
         Must be divisible by 4.

     Virtual address: DWORD
         Virtual memory address

     Checksum: DWORD
         CRC-32 checksum of the padded data segment.
         This variant of CRC-32:
          * no preset to 0xffffffff (-1)
          * no post-invert

The rest of the firmware file is comprised of the segment data. Note that
segments can be padded to align with 4 bytes. Padding value = 0xFF.

## Context
I only had very few samples of app firmware available. They all seem to be comprised of 3 segments, corresponding to

    Code RAM     @ 0x00100000
    Flash Memory @ 0x1F000000
    SRAM         @ 0x20000000

"Table 18: System Address Memory Map" in the data sheet might be helpful to further contextualize the data.

## Examples
    
    $ bin/fwinfo samples/hello_world.bin

	$ bin/firmware2elf samples/hello_world.bin /tmp/out.elf

	$ readelf -a /tmp/out.elf

## Appendix
### Authors
This repository is part of the [OpenMiHome project](https://github.com/openmihome). Authors include:

 * Wolfgang Frisch ([GitHub](https://github.com/wfr))

### Links
 * [Product page](https://origin-www.marvell.com/microcontrollers/88MW300/302/)
 * [Data sheet](https://www.marvell.com/microcontrollers/assets/MV-S109936-01C.pdf)
 * [SDK EZ-Connect Lite](http://marvell-iot.github.io/)
 * [ELF file format](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)
 * [libelf by example](ftp://ftp2.uk.freebsd.org/sites/downloads.sourceforge.net/e/el/elftoolchain/Documentation/libelf-by-example/20120308/libelf-by-example.pdf)

