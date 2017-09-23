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
#include "marvel-88mw30x-firmware.h"
#include "crc32.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <err.h>
#include <fcntl.h>
#include <assert.h>

int main(int argc, char** argv) {
	if (argc != 2) {
		errx(EXIT_FAILURE, "usage: %s firmware-file", argv[0]);
	}

	FILE *fin = fopen(argv[1], "rb");
	assert(fin != NULL);
	struct MarvellFirmware *fw = read_marvel_firmware(fin);
	fclose(fin);

	printf("MRVL\n");
	printf("ctime:        %u\n", fw->header.ctime);
	printf("num_segments: %d\n", fw->header.num_segments);
	printf("ELF version:  %08x\n", fw->header.elf_version);
	for (int i = 0; i < fw->header.num_segments; i++) {
		struct MarvellSegmentHeader *sh = &fw->seghdrs[i];
		printf("segment %d:\n", i);
		printf("  Offset:            %8x\n", sh->offset);
		printf("  Size:              %8x\n", sh->size);
		printf("  Virtual address:   %8x\n", sh->vaddr);
		printf("  Checksum:          %08x\n", sh->checksum);

		uint32_t checksum = crc32_byte(fw->segments[i], sh->size);
		printf("  Checksum (actual): %08x\n", checksum);
	}

	free_mrvl_firmware(fw);

	return 0;
}

