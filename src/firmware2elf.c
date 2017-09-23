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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <memory.h>

#include <libelf.h>
#include "marvel-88mw30x-firmware.h"


/* 
 * List of NULL-terminated strings,
 * used for construction of string table segments.
 */
struct stringlist {
	char *buf;
	size_t buflen;
};

void strlist_init(struct stringlist* st) {
	st->buf = strdup("");
	assert(st->buf);
	st->buflen = 1;
}

void strlist_free(struct stringlist* st) {
	free(st->buf);
	st->buf = NULL;
	st->buflen = 0;
}

int strlist_add(struct stringlist* st, const char *name) {
	size_t namelen = strlen(name) + 1;
	st->buf = realloc(st->buf, st->buflen + namelen);
	assert(st->buf);
	memcpy(st->buf + st->buflen, name, namelen);
	size_t ndx = st->buflen;
	st->buflen += namelen;
	return ndx;
}

int strlist_ndx(struct stringlist* st, const char* name) {
	size_t namelen = strlen(name) + 1;
	if (namelen > st->buflen)
		return 0;
	for (size_t i = 0; i <= (st->buflen - namelen); i++) {
		if (memcmp(name, st->buf + i, namelen) == 0)
			return i;
	}
	return 0;
}

int strlist_get(struct stringlist* st, const char* name) {
	int ndx = strlist_ndx(st, name);
	if (ndx == 0) {
		ndx = strlist_add(st, name);
	}
	return ndx;
}


/* .ARM.attributes section as found in Marvelll IOT SDK samples.
	Attribute Section: aeabi
	File Attributes
		Tag_CPU_name: "Cortex-M4"
		Tag_CPU_arch: v7E-M
		Tag_CPU_arch_profile: Microcontroller
		Tag_THUMB_ISA_use: Thumb-2
		Tag_FP_arch: VFPv4-D16
		Tag_ABI_PCS_wchar_t: 4
		Tag_ABI_FP_denormal: Needed
		Tag_ABI_FP_exceptions: Needed
		Tag_ABI_FP_number_model: IEEE 754
		Tag_ABI_align_needed: 8-byte
		Tag_ABI_enum_size: small
		Tag_ABI_HardFP_use: SP only
		Tag_ABI_optimization_goals: Aggressive Size
		Tag_CPU_unaligned_access: v6
*/
uint8_t _arm_attributes[] = {
	0x41, 0x34, 0x00, 0x00, 0x00, 0x61, 0x65, 0x61, 0x62, 0x69, 
	0x00, 0x01, 0x2A, 0x00, 0x00, 0x00, 0x05, 0x43, 0x6F, 0x72,
	0x74, 0x65, 0x78, 0x2D, 0x4D, 0x34, 0x00, 0x06, 0x0D, 0x07,
	0x4D, 0x09, 0x02, 0x0A, 0x06, 0x12, 0x04, 0x14, 0x01, 0x15,
	0x01, 0x17, 0x03, 0x18, 0x01, 0x1A, 0x01, 0x1B, 0x01, 0x1E, 
	0x04, 0x22, 0x01 };


int main(int argc, char** argv) {
	FILE *fin, *fout;
	Elf *e;
	Elf_Scn *scn;
	Elf_Data *data;
	Elf32_Ehdr *ehdr;
	/* TO DO: write program header. */
	//Elf32_Phdr *phdr;
	Elf32_Shdr *shdr;

	if (argc != 3) {
		errx(EXIT_FAILURE, "usage: %s input-firmware output-elf", argv[0]);
	}

	if (elf_version(EV_CURRENT) ==  EV_NONE) {
		errx(EXIT_FAILURE, "ELF library initialization failed: %s", elf_errmsg(-1));
	}

	if (!(fin = fopen(argv[1], "rb"))) {
		err(EXIT_FAILURE, "open %s failed", argv[1]);
	}

	struct MarvellFirmware *fw = read_marvel_firmware(fin);
	fclose(fin);
	printf("MRVL\n");
	printf("ctime:        %u\n", fw->header.ctime);
	printf("num_segments: %d\n", fw->header.num_segments);
	printf("ELF version:  %08x\n", fw->header.elf_version);
	for (int i = 0; i < fw->header.num_segments; i++) {
		struct MarvellSegmentHeader *sh = &fw->seghdrs[i];
		printf("segment %d:\n", i);
		//printf("  Offset:     %8x\n", sh->offset);
		printf("  Size:       %8x\n", sh->size);
		printf("  vaddr:      %8x\n", sh->vaddr);
		printf("  Checksum:   %08x\n", sh->checksum);
	}

	if (!(fout = fopen(argv[2], "wb"))) {
		err(EXIT_FAILURE, "open %s failed", argv[2]);
	}

	if ((e = elf_begin(fileno(fout), ELF_C_WRITE, NULL)) == NULL) {
		errx(EXIT_FAILURE, "elf_begin () failed: %s.", elf_errmsg (-1));
	}

	if ((ehdr = elf32_newehdr(e)) == NULL) {
		errx(EXIT_FAILURE, "elf32_newehdr () failed: %s.", elf_errmsg(-1));
	}

	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_machine = EM_ARM;
	ehdr->e_type = ET_EXEC;
	ehdr->e_flags = EF_ARM_EABI_VER5 | EF_ARM_ABI_FLOAT_SOFT;

	// if ((phdr = elf32_newphdr(e, 1)) == NULL) {
	//	errx(EXIT_FAILURE, "elf32_newphdr () failed: %s.", elf_errmsg (-1));
	//}
	
	struct stringlist secnames;
	strlist_init(&secnames);

	// Copy firmware segments
	for (int i = 0; i < fw->header.num_segments; i++) {
		if ((scn = elf_newscn(e)) == NULL) {
			errx(EXIT_FAILURE, "elf_newscn () failed: %s.", elf_errmsg (-1));
		}
		if ((data = elf_newdata(scn)) == NULL) {
			errx(EXIT_FAILURE, "elf_newdata () failed: %s.", elf_errmsg (-1));
		}

		data->d_align = 1;
		data->d_off   = 0LL;
		data->d_buf   = fw->segments[i];
		data->d_type = ELF_T_BYTE; // shouldn't matter for our purpose.
		data->d_size = fw->seghdrs[i].size;
		data->d_version = EV_CURRENT;

		if ((shdr = elf32_getshdr(scn)) == NULL) {
			errx(EXIT_FAILURE, "elf32_getshdr() failed: %s.",	elf_errmsg (-1));
		}
		
		shdr->sh_type = SHT_PROGBITS;
		shdr->sh_flags = SHF_ALLOC;
		shdr->sh_entsize = 0;
		shdr->sh_addr = fw->seghdrs[i].vaddr;

		// assumptions based on very few samples
		switch (i) {
			case 0:
				data->d_align = 8;
				shdr->sh_name = strlist_get(&secnames, ".init");
				shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
				break;
			case 1:
				data->d_align = 16;
				shdr->sh_name = strlist_get(&secnames, ".text");
				shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
				break;
			case 2:
				shdr->sh_name = strlist_get(&secnames, ".data");
				//shdr->sh_flags = SHF_ALLOC | SHF_WRITE;
				shdr->sh_flags = SHF_ALLOC | SHF_WRITE | SHF_EXECINSTR;
				break;
			default:
				printf("warning: firmware contains unknown segment %d\n", i);
				shdr->sh_name = strlist_get(&secnames, ".text");
		}
	}

	// assumption about entry point, as observed from samples
	if (fw->header.num_segments >= 2) {
		ehdr->e_entry = fw->seghdrs[1].vaddr + 1;
	}

	// .ARM.attributes
	if ((scn = elf_newscn(e)) == NULL) {
		errx(EXIT_FAILURE, "elf_newscn() failed: %s.", elf_errmsg (-1));
	}
	if ((data = elf_newdata(scn)) == NULL) {
		errx(EXIT_FAILURE, "elf_newdata() failed: %s.", elf_errmsg (-1));
	}
	data->d_align = 1;
	data->d_buf = _arm_attributes;
	data->d_off = 0LL;
	data->d_size = sizeof(_arm_attributes);
	data->d_type = ELF_T_BYTE;
	data->d_version = EV_CURRENT;

	if ((shdr = elf32_getshdr(scn)) == NULL) {
		errx(EXIT_FAILURE, "elf32_getshdr() failed: %s.", elf_errmsg (-1));
	}
	shdr->sh_name = strlist_get(&secnames, ".ARM.attributes");
	shdr->sh_type = SHT_ARM_ATTRIBUTES;
	shdr->sh_flags = 0;
	shdr->sh_entsize = 0;

	// add sechdr string section
	strlist_add(&secnames, ".shstrtab");
	if ((scn = elf_newscn(e)) == NULL) {
		errx(EXIT_FAILURE, "elf_newscn() failed: %s.", elf_errmsg (-1));
	}
	if ((data = elf_newdata(scn)) == NULL) {
		errx(EXIT_FAILURE, "elf_newdata() failed: %s.", elf_errmsg (-1));
	}
	data->d_align = 1;
	data->d_buf = secnames.buf;
	data->d_off = 0LL;
	data->d_size = secnames.buflen;
	data->d_type = ELF_T_BYTE;
	data->d_version = EV_CURRENT;

	if ((shdr = elf32_getshdr(scn)) == NULL) {
		errx(EXIT_FAILURE, "elf32_getshdr() failed: %s.", elf_errmsg (-1));
	}
	shdr->sh_name = strlist_ndx(&secnames, ".shstrtab");
	shdr->sh_type = SHT_STRTAB;
	shdr->sh_flags = SHF_STRINGS | SHF_ALLOC;
	shdr->sh_entsize = 0;

	ehdr->e_shstrndx = elf_ndxscn(scn);

	if (elf_update(e, ELF_C_NULL) < 0) {
		errx(EXIT_FAILURE, "elf_update(NULL) failed: %s.", elf_errmsg (-1));
	}

	// phdr->p_type = PT_PHDR;
	// phdr->p_offset = ehdr->e_phoff;
	// phdr->p_filesz = elf32_fsize(ELF_T_PHDR, 1, EV_CURRENT);
	// (void) elf_flagphdr(e, ELF_C_SET, ELF_F_DIRTY );

	if (elf_update(e, ELF_C_WRITE) < 0) {
		errx(EXIT_FAILURE, "elf_update() failed: %s.", elf_errmsg (-1));
	}

	(void) elf_end(e);
	fclose(fout);
	free_mrvl_firmware(fw);

	return 0;
}
