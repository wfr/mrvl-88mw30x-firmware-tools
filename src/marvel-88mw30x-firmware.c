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
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <err.h>
#include <fcntl.h>
#include <assert.h>

static const char magic1[4] = {'M', 'R', 'V', 'L'};
static const uint32_t magic2 = 0x2E9CF17B;
static const uint32_t segment_magic = 2;


struct MarvellFirmware* new_mrvl_firmware() {
	struct MarvellFirmware *fw = malloc(sizeof(struct MarvellFirmware));
	if (!fw)
		errx(EXIT_FAILURE, "failed to allocate MarvellFirmware*");
	fw->seghdrs = NULL;
	fw->segments = NULL;

	memcpy(fw->header.mrvl, magic1, sizeof(magic1));
	fw->header.unknown1 = magic2;
	fw->header.ctime = 0;
	fw->header.num_segments = 0;
	fw->header.elf_version = 0;
	return fw;
}

void free_mrvl_firmware(struct MarvellFirmware* fw) {
	if (fw->seghdrs) {
		free(fw->seghdrs);
	}
	if (fw->segments) {
		for (int i = 0; i < fw->header.num_segments; i++) {
			if (fw->segments[i]) {
				free (fw->segments[i]);
			}
		}
		free(fw->segments);
	}
}

void read_marvel_header(FILE *f, struct MarvellHeader* hdr) {
	size_t res = fread(hdr, sizeof(struct MarvellHeader), 1, f);
	assert(res == 1);
	if (memcmp(hdr->mrvl, magic1, sizeof(magic1)) != 0)
		errx(EXIT_FAILURE, "magic1 does not match");
	if (hdr->unknown1 != magic2)
		errx(EXIT_FAILURE, "magic2 does not match");
}

struct MarvellFirmware* read_marvel_firmware(FILE* f) {
	struct MarvellFirmware *fw = new_mrvl_firmware();
	read_marvel_header(f, &fw->header);

	fw->seghdrs = malloc(sizeof(struct MarvellSegmentHeader) * fw->header.num_segments);
	assert(fw->seghdrs);
	fw->segments = malloc(sizeof(void*) * fw->header.num_segments);
	assert(fw->segments);

	size_t res = fread(fw->seghdrs, sizeof(struct MarvellSegmentHeader), fw->header.num_segments, f);
	if (res != fw->header.num_segments)
		errx(EXIT_FAILURE, "could not read %d segment headers", fw->header.num_segments);

	for (int i = 0; i < fw->header.num_segments; i++) {
		struct MarvellSegmentHeader *sh = &(fw->seghdrs[i]);
		if (sh->type != segment_magic)
			errx(EXIT_FAILURE, "segment_header[0] unexpected, %d != %d", sh->type, segment_magic);
	}

	for (int i = 0; i < fw->header.num_segments; i++) {
		fw->segments[i]= malloc(fw->seghdrs[i].size);
		assert(fw->segments[i]);
		fseek(f, fw->seghdrs[i].offset, SEEK_SET);
		size_t res = fread(fw->segments[i], fw->seghdrs[i].size, 1, f);
		assert (res == 1);
	}

	return fw;
}

