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
#define _XOPEN_SOURCE 500

#include <err.h>
#include <gelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#include <libelf.h>
#include "marvel-88mw30x-firmware.h"
#include "crc32.h"


void print_phdr(GElf_Phdr *phdr) {
	printf("PHDR:\n");
	printf("    %-10s          %8x\n", "p_type",   phdr->p_type);
	printf("    %-10s  %16lx\n",       "p_offset", phdr->p_offset);
	printf("    %-10s  %16lx\n",       "p_vaddr",  phdr->p_vaddr);
	printf("    %-10s  %16lx\n",       "p_filesz", phdr->p_filesz);
}


int main(int argc, char** argv) {
	FILE *fin, *fout;
	Elf *e;
	GElf_Ehdr ehdr;
	GElf_Phdr phdr;
	size_t shstrndx, n;
	int i;

	if (argc != 3)
		errx(EXIT_FAILURE, "usage: %s axf-filename firmware-filename", argv[0]);
	
	if (elf_version(EV_CURRENT) ==  EV_NONE)
		errx(EXIT_FAILURE, "ELF library initialization failed: %s", elf_errmsg(-1));
	
	if (!(fin = fopen(argv[1], "rb"))) {
		err(EXIT_FAILURE, "open %s failed", argv[1]);
	}

	if ((e = elf_begin(fileno(fin), ELF_C_READ, NULL)) == NULL)
		errx(EXIT_FAILURE, "elf_begin() failed: %s", elf_errmsg(-1));

	if (elf_kind(e) != ELF_K_ELF)
		errx(EXIT_FAILURE, "%s is not an ELF object", argv[1]);

	if (gelf_getehdr(e, &ehdr) == NULL)
		errx(EXIT_FAILURE, "getehdr() failed: %s.", elf_errmsg(-1));

	if ((i = gelf_getclass(e)) == ELFCLASSNONE) 
		errx(EXIT_FAILURE, "getclass() failed: %s.", elf_errmsg(-1));
	
	if (i != ELFCLASS32)
		errx(EXIT_FAILURE, "this program only supports 32-bit ELF.");

	if (elf_getshdrstrndx(e, &shstrndx ) != 0)
		errx(EXIT_FAILURE, "elf_getshdrstrndx() failed: %s.", elf_errmsg(-1));
	
	if (elf_getphdrnum(e, &n) != 0)
		errx(EXIT_FAILURE, "elf_getphdrnum() failed: %s.", elf_errmsg(-1));

	struct MarvellFirmware *fw = new_mrvl_firmware();
	fw->header.ctime = (uint32_t) time(NULL);
	fw->header.elf_version = ehdr.e_version;
	fw->header.num_segments = 0;

	// count usable segments
	for (int i = 0; i < n; i++) {
		if (gelf_getphdr(e, i, &phdr) != &phdr)
			errx(EXIT_FAILURE, "getphdr() failed: %s.", elf_errmsg(-1));
		print_phdr(&phdr);
		if (phdr.p_type == PT_LOAD && phdr.p_filesz > 0) {
			fw->header.num_segments++;
		}
	}
	if (fw->header.num_segments == 0)
		errx(EXIT_FAILURE, "ELF contains no suitable segments");
	if (fw->header.num_segments > 9)
		errx(EXIT_FAILURE, "ELF contains more than the maximum allowed 9 segments");

	fw->seghdrs = malloc(sizeof(struct MarvellSegmentHeader) * fw->header.num_segments);
	assert(fw->seghdrs);
	memset(fw->seghdrs, 0, sizeof(struct MarvellSegmentHeader) * fw->header.num_segments);
	fw->segments = malloc(sizeof(void*) * fw->header.num_segments);
	assert(fw->segments);

	if (!(fout = fopen(argv[2], "wb"))) {
		err(EXIT_FAILURE, "open %s failed", argv[2]);
	}
	if (fwrite(&fw->header, sizeof(struct MarvellHeader), 1, fout) != 1)
		errx(EXIT_FAILURE, "cannot write firmware header.");
	if (fwrite(fw->seghdrs, sizeof(struct MarvellSegmentHeader) * fw->header.num_segments, 1, fout) != 1)
		errx(EXIT_FAILURE, "cannot write to firmware");
	
	// write segments, populate segment headers
	int si = 0;
	for (int i = 0; i < n; i++) {
		if (gelf_getphdr(e, i, &phdr) != &phdr)
			errx(EXIT_FAILURE, "getphdr() failed: %s.", elf_errmsg(-1));
		
		if (phdr.p_type == PT_LOAD && phdr.p_filesz > 0) {
			struct MarvellSegmentHeader *msh = &(fw->seghdrs[si]);
			msh->vaddr = phdr.p_vaddr;

			uint32_t padding = ((phdr.p_filesz + 3) & 0xfffffffc) - phdr.p_filesz;
			msh->size = phdr.p_filesz + padding;
			fw->segments[si] = malloc(msh->size);
			memset(fw->segments[si], 0xFF, padding);
			if (fseek(fin, phdr.p_offset, SEEK_SET))
				err(EXIT_FAILURE, "cannot seek to ELF segment.");
			if (fread(fw->segments[si], phdr.p_filesz, 1, fin) != 1)
				err(EXIT_FAILURE, "cannot read ELF segment.");
			msh->checksum = crc32_byte(fw->segments[si], msh->size);
			msh->offset = ftell(fout);
			if (fwrite(fw->segments[si], msh->size, 1, fout) != 1) {
				errx(EXIT_FAILURE, "cannot write firmware segment.");
			}
			msh->type = 2; // seems to be constant

			si++;
		}
	}

	// write segment headers
	fseek(fout, sizeof(struct MarvellHeader), SEEK_SET);
	for (int i = 0; i < fw->header.num_segments; i++) {
		if (fwrite(&(fw->seghdrs[i]), sizeof(struct MarvellSegmentHeader), 1, fout) != 1)
			errx(EXIT_FAILURE, "failed to write firmware segment.");
	}
	assert(ftell(fout) == fw->seghdrs[0].offset);

	elf_end(e);
	free_mrvl_firmware(fw);
	fclose(fin);
	fclose(fout);
	return 0;
}
