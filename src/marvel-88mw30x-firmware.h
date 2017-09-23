/* 
 * This file is part of mrvl-88mw30x-firmware-tools
 * Copyright (c) 2017 Wolfgang Frisch.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <stdint.h>
#include <stdio.h>

struct __attribute__((packed, scalar_storage_order("little-endian"))) 
MarvellHeader {
	char mrvl[4];                         // "MRVL"
	uint32_t unknown1;                    // hardcoded constant
	uint32_t ctime;                       // UNIX timestamp
	uint32_t num_segments; 
	uint32_t elf_version;
};

struct __attribute__((packed, scalar_storage_order("little-endian"))) 
MarvellSegmentHeader {
	uint32_t type;                        // hardcoded as 2 in axf2firmware
	uint32_t offset;                      // segment data offset in file
	uint32_t size;                        // segment data size
	uint32_t vaddr;                       // memory address
	uint32_t checksum;                    // CRC32(segment data)
};

struct
MarvellFirmware {
	struct MarvellHeader header;
	struct MarvellSegmentHeader *seghdrs;
	uint8_t **segments;
};


extern struct MarvellFirmware* new_mrvl_firmware();
extern struct MarvellFirmware* read_marvel_firmware(FILE* f);
extern void free_mrvl_firmware(struct MarvellFirmware* fw);
