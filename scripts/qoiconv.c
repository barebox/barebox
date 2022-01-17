// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2021 Dominic Szablewski

/*
 * Adapted from https://github.com/phoboslab/qoi/blob/master/qoiconv.c
 * with the sole modification of removing the png write functionnality
 * hence only requiring stb_image.h
 */

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#include "stb_image.h"

#define QOI_IMPLEMENTATION
#include "../lib/gui/qoi.h"

#define STR_ENDS_WITH(S, E) (strcmp(S + strlen(S) - (sizeof(E)-1), E) == 0)

int main(int argc, char **argv) {
	void *pixels = NULL;
	int w, h, channels;
	int encoded = 0;

	if (argc < 3) {
		puts("Usage: qoiconv <infile> <outfile>");
		puts("Examples:");
		puts("  qoiconv input.png output.qoi");
		exit(1);
	}
	if (STR_ENDS_WITH(argv[1], ".png")) {
		if(!stbi_info(argv[1], &w, &h, &channels)) {
			printf("Couldn't read header %s\n", argv[1]);
			exit(1);
		}

		// Force all odd encodings to be RGBA
		if(channels != 3) {
			channels = 4;
		}

		pixels = (void *)stbi_load(argv[1], &w, &h, NULL, channels);
	}
	if (pixels == NULL) {
		printf("Couldn't load/decode %s\n", argv[1]);
		exit(1);
	}
	if (STR_ENDS_WITH(argv[2], ".qoi")) {
		encoded = qoi_write(argv[2], pixels, &(qoi_desc){
			.width = w,
			.height = h,
			.channels = channels,
			.colorspace = QOI_SRGB
		});
	}
	if (!encoded) {
		printf("Couldn't write/encode %s\n", argv[2]);
		exit(1);
	}

	free(pixels);
	return 0;
}
