#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "accord-le-ioctl.h"

void usage(const char *argv0)
{
	fprintf(stderr, "%s - write contents of a file into AccordLE memory chip\n", argv0);
	fprintf(stderr, "Usage: %s [OPTIONS] filename\n", argv0);
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "\t-o, --offset=OFFSET    write data starting at OFFSET sectors (decimal)"
		"in memory. Each sector is 262144 bytes long.\n");
}

int main(int argc, char *argv[])
{
	int res = 1, dev_fd, data_fd, i, j;
	uint8_t buf[ACLE_SECTOR_SIZE];
	ssize_t _read;
	char *filename;
	int offset = 0;
	//int limit = 64; /* 64 sectors total */

	struct option options[] = {
		{"offset", required_argument, NULL, 'o'},
		{NULL},
	};

	while (1) {
		int c = getopt_long(argc, argv, "o:", options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'o':
			offset = (int)strtol(optarg, NULL, 10);
			break;
		}
	}

	if (optind != argc - 1) {
		usage(argv[0]);
		exit(1);
	}

	filename = argv[optind];

	if (offset < 0 || offset > 127) {
		fprintf(stderr, "invalid offset value (0 <= offset < 128)\n");
		exit(1);
	}

	dev_fd = open("/dev/accord-le", O_RDWR);
	if (dev_fd == -1) {
		fprintf(stderr, "failed to open device: %m\n");
		exit(1);
	}

	data_fd = open(filename, O_RDONLY);
	if (data_fd == -1) {
		fprintf(stderr, "failed to open %s: %m\n", filename);
		goto err1;
	}

	memset(buf, 0xff, ACLE_SECTOR_SIZE);
	i = offset;

	struct acle_data_chunk page = {
		.offset = offset * ACLE_SECTOR_SIZE,
		.count = ACLE_PAGE_SIZE,
	};

	while ((_read = read(data_fd, buf, ACLE_SECTOR_SIZE)) > 0) {
		fprintf(stderr, "erasing sector #%d\n", i);
		res = ioctl(dev_fd, ACLE_CMD_ERASE_SECTOR, i);
		if (res) {
			fprintf(stderr, "failed to erase sector #%d: %m\n", i);
			goto err2;
		}

		page.data = buf;

		for (j = 0; j < ACLE_SECTOR_SIZE / ACLE_PAGE_SIZE; ++j) {
			fprintf(stderr, "writing page #%lu\n",
				page.offset / ACLE_PAGE_SIZE);
			res = ioctl(dev_fd, ACLE_CMD_WRITE, &page);
			if (res) {
				close(dev_fd);
				fprintf(stderr, "failed to write page #%lu: %m\n",
					page.offset / ACLE_PAGE_SIZE);
				exit(1);
			}
	
			page.offset += ACLE_PAGE_SIZE;
			page.data += ACLE_PAGE_SIZE;
		}

		++i;
		memset(buf, 0xff, ACLE_SECTOR_SIZE);
	}

	if (_read == -1) {
		fprintf(stderr, "failed to read from %s: %m\n", filename);
		goto err2;
	}

	fprintf(stderr, "data successfully written\n");
	res = 0;

err2:
	close(data_fd);
err1:
	close(dev_fd);
	return res;
}
