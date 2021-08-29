// Copyright (c) 2021 Rumen G. Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Rumen Bogdanovski <rumen@skyarchive.org>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_fits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#define FITS_HEADER_SIZE 2880

int save_file(char *file_name, char *data, int size) {
#if defined(INDIGO_WINDOWS)
	int handle = open(file_name, O_CREAT | O_WRONLY | O_BINARY, 0);
#else
	int handle = open(file_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
#endif
	if (handle < 0) {
		return -1;
	}
	int res = write(handle, data, size);
	if (res < size) {
		close(handle);
		return -1;
	}
	close(handle);
	return 0;
}

int open_file(const char *file_name, char **data, int *size) {
	int image_size = 0;
	if (!strcmp(file_name, "")) {
		errno = ENOENT;
		return -1;
	}
	FILE *file;
	file = fopen(file_name, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		image_size = (size_t)ftell(file);
		fseek(file, 0, SEEK_SET);
		if (*data == NULL) {
			*data = (char *)malloc(image_size + 1);
		} else {
			*data = (char *)realloc(*data, image_size + 1);
		}
		*size = fread(*data, image_size, 1, file);
		fclose(file);
		*size = image_size;
		return 0;
	}
	return -1;
}

int convert_raw_to_fits(char *infile_name) {
	char *in_data = NULL;
	char *out_data = NULL;
	int size = 0;

	int res = open_file(infile_name, &in_data, &size);
	if (res != 0) {
		if (in_data) free(in_data);
		return -1;
	}

	res = indigo_raw_to_fits(in_data, &out_data, &size, NULL);
	if (res != 0) {
		if (in_data) free(in_data);
		if (out_data) free(out_data);
		return -1;
	}

	char outfile_name[PATH_MAX];
	strncpy(outfile_name, infile_name, PATH_MAX);
	/* relace replace suffix with .fits */
	char *dot = strrchr(outfile_name, '.');
	if (dot) {
		strcpy(dot, ".fits");
	} else {
		snprintf(outfile_name, PATH_MAX, "%s.fits", infile_name);
	}

	res = save_file(outfile_name, out_data, size);
	if (in_data) free(in_data);
	if (out_data) free(out_data);

	return res;
}
