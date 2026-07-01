#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <iconv.h>
#include <langinfo.h>
#include <errno.h>

#include "accord-le-ioctl.h"

#include "aes.h"
#include "sha256.h"
#include "password.h"

#define PROG_COMPANY	"OKB SAPR"
#define PROG_VERSION	"1.0"
#define PROG_DATE		"May 7, 2012"

typedef struct {
    char aes[3];
    unsigned char version;
    unsigned char last_block_size;
} aescrypt_hdr;

typedef unsigned char sha256_t[32];

/*
 *  decrypt_stream
 *
 *  This function is called to decrypt the open data steam "infp".
 */
int decrypt_stream(FILE *infp, void *outbuf, char* passwd, int passlen)
{
    aes_context                 aes_ctx;
    sha256_context              sha_ctx;
    aescrypt_hdr                aeshdr;
    sha256_t                    digest;
    unsigned char               IV[16];
    unsigned char               iv_key[48];
    int                         i, j, n, bytes_read;
    size_t                      count;
    unsigned char               buffer[64], buffer2[32];
    unsigned char               *head, *tail;
    unsigned char               ipad[64], opad[64];
    int                         reached_eof = 0;

    int buf_offset=0;
    
    // Read the file header
    if ((bytes_read = fread(&aeshdr, 1, sizeof(aeshdr), infp)) !=
         sizeof(aescrypt_hdr))
    {
        if (feof(infp))
        {
            fprintf(stderr, "Error: Input file is too short.\n");
        }
        else
        {
            perror("Error reading the file header:");
        }
        return  -1;
    }

    if (!(aeshdr.aes[0] == 'A' && aeshdr.aes[1] == 'M' &&
          aeshdr.aes[2] == 'D'))
    {
        fprintf(stderr, "Error: Bad file header (corrupted? [%x, %x, %x, %x])\n", aeshdr.aes[0], aeshdr.aes[1], aeshdr.aes[2], aeshdr.version);
        return  -1;
    }

    // Validate the version number and take any version-specific actions
    if (aeshdr.version == 0)
    {
        // Let's just consider the least significant nibble to determine
        // the size of the last block
        aeshdr.last_block_size = (aeshdr.last_block_size & 0x0F);
    }
    else if (aeshdr.version > 0x90)//0x02)
    {
        fprintf(stderr, "Error: Unsupported enc-file version: %d\n",
                aeshdr.version);
        return  -1;
    }

    // Skip over extensions present v2 and later files
    if (aeshdr.version >= 0x02)
    {
        do
        {
            if ((bytes_read = fread(buffer, 1, 2, infp)) != 2)
            {
                if (feof(infp))
                {
                    fprintf(stderr, "Error: Input file is too short.\n");
                }
                else
                {
                    perror("Error reading the file extensions:");
                }
                return  -1;
            }
            // Determine the extension length, zero means no more extensions
            i = j = (((int)buffer[0]) << 8) | (int)buffer[1];
            while (i--)
            {
                if ((bytes_read = fread(buffer, 1, 1, infp)) != 1)
                {
                    if (feof(infp))
                    {
                        fprintf(stderr, "Error: Input file is too short.\n");
                    }
                    else
                    {
                        perror("Error reading the file extensions:");
                    }
                    return  -1;
                }
            }
        } while(j);
    }

    // Read the initialization vector from the file
    if ((bytes_read = fread(IV, 1, 16, infp)) != 16)
    {
        if (feof(infp))
        {
            fprintf(stderr, "Error: Input file is too short.\n");
        }
        else
        {
            perror("Error reading the initialization vector:");
        }
        return  -1;
    }

    // Hash the IV and password 8192 times
    memset(digest, 0, 32);
    memcpy(digest, IV, 16);
    for(i=0; i<8192; i++)
    {
        sha256_starts(  &sha_ctx);
        sha256_update(  &sha_ctx, digest, 32);
        sha256_update(  &sha_ctx,
                        (unsigned char*)passwd,
                        passlen);
        sha256_finish(  &sha_ctx,
                        digest);
    }

    // Set the AES encryption key
    aes_set_key(&aes_ctx, digest, 256);

    // Set the ipad and opad arrays with values as
    // per RFC 2104 (HMAC).  HMAC is defined as
    //   H(K XOR opad, H(K XOR ipad, text))
    memset(ipad, 0x36, 64);
    memset(opad, 0x5C, 64);

    for(i=0; i<32; i++)
    {
        ipad[i] ^= digest[i];
        opad[i] ^= digest[i];
    }

    sha256_starts(&sha_ctx);
    sha256_update(&sha_ctx, ipad, 64);

    // If this is a version 1 or later file, then read the IV and key
    // for decrypting the bulk of the file.
    if (aeshdr.version >= 0x01)
    {
        for(i=0; i<48; i+=16)
        {
            if ((bytes_read = fread(buffer, 1, 16, infp)) != 16)
            {
                if (feof(infp))
                {
                    fprintf(stderr, "Error: Input file is too short.\n");
                }
                else
                {
                    perror("Error reading input file IV and key:");
                }
                return  -1;
            }

            memcpy(buffer2, buffer, 16);

            sha256_update(&sha_ctx, buffer, 16);
            aes_decrypt(&aes_ctx, buffer, buffer);

            // XOR plain text block with previous encrypted
            // output (i.e., use CBC)
            for(j=0; j<16; j++)
            {
                iv_key[i+j] = (buffer[j] ^ IV[j]);
            }

            // Update the IV (CBC mode)
            memcpy(IV, buffer2, 16);
        }

        // Verify that the HMAC is correct
        sha256_finish(&sha_ctx, digest);
        sha256_starts(&sha_ctx);
        sha256_update(&sha_ctx, opad, 64);
        sha256_update(&sha_ctx, digest, 32);
        sha256_finish(&sha_ctx, digest);

        if ((bytes_read = fread(buffer, 1, 32, infp)) != 32)
        {
            if (feof(infp))
            {
                fprintf(stderr, "Error: Input file is too short.\n");
            }
            else
            {
                perror("Error reading input file digest:");
            }
            return  -1;
        }

        if (memcmp(digest, buffer, 32))
        {
            fprintf(stderr, "Error: Message has been altered or password is incorrect\n");
            return  -1;
        }

        // Re-load the IV and encryption key with the IV and
        // key to now encrypt the datafile.  Also, reset the HMAC
        // computation.
        memcpy(IV, iv_key, 16);

        // Set the AES encryption key
        aes_set_key(&aes_ctx, iv_key+16, 256);

        // Set the ipad and opad arrays with values as
        // per RFC 2104 (HMAC).  HMAC is defined as
        //   H(K XOR opad, H(K XOR ipad, text))
        memset(ipad, 0x36, 64);
        memset(opad, 0x5C, 64);

        for(i=0; i<32; i++)
        {
            ipad[i] ^= iv_key[i+16];
            opad[i] ^= iv_key[i+16];
        }

        // Wipe the IV and encryption mey from memory
        memset(iv_key, 0, 48);

        sha256_starts(&sha_ctx);
        sha256_update(&sha_ctx, ipad, 64);
    }
    
    // Decrypt the balance of the file

    // Attempt to initialize the ring buffer with contents from the file.
    // Attempt to read 48 octets of the file into the ring buffer.
    if ((bytes_read = fread(buffer, 1, 48, infp)) < 48)
    {
        if (!feof(infp))
        {
            perror("Error reading input file ring:");
            return  -1;
        }
        else
        {
            // If there are less than 48 octets, the only valid count
            // is 32 for version 0 (HMAC) and 33 for version 1 or
            // greater files ( file size modulo + HMAC)
            if ((aeshdr.version == 0x00 && bytes_read != 32) ||
                (aeshdr.version >= 0x01 && bytes_read != 33))
            {
                fprintf(stderr, "Error: Input file is corrupt (1:%d).\n",
                        bytes_read);
                return -1;
            }
            else
            {
                // Version 0 files would have the last block size
                // read as part of the header, so let's grab that
                // value now for version 1 files.
                if (aeshdr.version >= 0x01)
                {
                    // The first octet must be the indicator of the
                    // last block size.
                    aeshdr.last_block_size = (buffer[0] & 0x0F);
                }
                // If this initial read indicates there is no encrypted
                // data, then there should be 0 in the last_block_size field
                if (aeshdr.last_block_size != 0)
                {
                    fprintf(stderr, "Error: Input file is corrupt (2).\n");
                    return -1;
                }
            }
            reached_eof = 1;
        }
    }
    head = buffer + 48;
    tail = buffer;

    while(!reached_eof)
    {
        // Check to see if the head of the buffer is past the ring buffer
        if (head == (buffer + 64))
        {
            head = buffer;
        }

        if ((bytes_read = fread(head, 1, 16, infp)) < 16)
        {
            if (!feof(infp))
            {
                perror("Error reading input file:");
                return  -1;
            }
            else
            {
                // The last block for v0 must be 16 and for v1 it must be 1
                if ((aeshdr.version == 0x00 && bytes_read > 0) ||
                    (aeshdr.version >= 0x01 && bytes_read != 1))
                {
                    fprintf(stderr, "Error: Input file is corrupt (3:%d).\n",
                            bytes_read);
                    return -1;
                }

                // If this is a v1 file, then the file modulo is located
                // in the ring buffer at tail + 16 (with consideration
                // given to wrapping around the ring, in which case
                // it would be at buffer[0])
                if (aeshdr.version >= 0x01)
                {
                    if ((tail + 16) < (buffer + 64))
                    {
                        aeshdr.last_block_size = (tail[16] & 0x0F);
                    }
                    else
                    {
                        aeshdr.last_block_size = (buffer[0] & 0x0F);
                    }
                }

                // Indicate that we've reached the end of the file
                reached_eof = 1;
            }
        }

        // Process data that has been read.  Note that if the last
        // read operation returned no additional data, there is still
        // one one ciphertext block for us to process if this is a v0 file.
        if ((bytes_read > 0) || (aeshdr.version == 0x00))
        {
            // Advance the head of the buffer forward
            if (bytes_read > 0)
            {
                head += 16;
            }

            memcpy(buffer2, tail, 16);

            sha256_update(&sha_ctx, tail, 16);
            aes_decrypt(&aes_ctx, tail, tail);

            // XOR plain text block with previous encrypted
            // output (i.e., use CBC)
            for(i=0; i<16; i++)
            {
                tail[i] ^= IV[i];
            }

            // Update the IV (CBC mode)
            memcpy(IV, buffer2, 16);

            // If this is the final block, then we may
            // write less than 16 octets
            n = ((!reached_eof) ||
                 (aeshdr.last_block_size == 0)) ? 16 : aeshdr.last_block_size;

            // Write the decrypted block
/*            if ((i = fwrite(tail, 1, n, outfp)) != n)
            {
                perror("Error writing decrypted block:");
                return  -1;
            }*/
            count = n;
            memcpy (outbuf+buf_offset, (void*)tail, count);
            buf_offset = buf_offset + n;

//          printf("'%s' ", tail);

            // Move the tail of the ring buffer forward
            tail += 16;
            if (tail == (buffer+64))
            {
                tail = buffer;
            }
        }
    }

    // Verify that the HMAC is correct
    sha256_finish(&sha_ctx, digest);
    sha256_starts(&sha_ctx);
    sha256_update(&sha_ctx, opad, 64);
    sha256_update(&sha_ctx, digest, 32);
    sha256_finish(&sha_ctx, digest);

    // Copy the HMAC read from the file into buffer2
    if (aeshdr.version == 0x00)
    {
        memcpy(buffer2, tail, 16);
        tail += 16;
        if (tail == (buffer + 64))
        {
            tail = buffer;
        }
        memcpy(buffer2+16, tail, 16);
    }
    else
    {
        memcpy(buffer2, tail+1, 15);
        tail += 16;
        if (tail == (buffer + 64))
        {
            tail = buffer;
        }
        memcpy(buffer2+15, tail, 16);
        tail += 16;
        if (tail == (buffer + 64))
        {
            tail = buffer;
        }
        memcpy(buffer2+31, tail, 1);
    }

    if (memcmp(digest, buffer2, 32))
    {
        if (aeshdr.version == 0x00)
        {
            fprintf(stderr, "Error: Message has been altered or password is incorrect\n");
        }
        else
        {
            fprintf(stderr, "Error: Message has been altered and should not be trusted\n");
        }

        return -1;
    }

    return 0;
}

void usage(const char *argv0)
{
	fprintf(stderr, "%s\tv%s\t%s, %s\n", argv0, PROG_VERSION, PROG_COMPANY, PROG_DATE);
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - write encrypted database into AccordLE memory chip\n", argv0);
	fprintf(stderr, "\tUsage: %s [OPTIONS] filename\n", argv0);
	fprintf(stderr, "\tOPTIONS:\n");
	fprintf(stderr, "\t\t-p, --password=PASSWORD\tpassword to AMDZ database file.\n");
}

int main(int argc, char *argv[])
{
	int res = 0, dev_fd, j;//, data_fd, i;
	uint8_t buf[ACLE_SECTOR_SIZE];
//	ssize_t _read;
	char *filename;
	int offset = 5; /* user`s db-offset */
	//int limit = 64; /* 64 sectors total */

    FILE *infp = NULL;

    int passlen=0;
//    FILE *infp = NULL;
    encryptmode_t mode = DEC;
//    char *infile = NULL;
    char pass_input[MAX_PASSWD_LEN+1], pass[MAX_PASSWD_LEN+1];

	struct option options[] = {
		{"password", required_argument, NULL, 'p'},
		{NULL},
	};

	while (1) {
		int c = getopt_long(argc, argv, "p:", options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'p':
			passlen = passwd_to_utf16(optarg, strlen((char *)optarg), MAX_PASSWD_LEN+1, pass);
			if (passlen < 0)
			{
				printf ("error - invalid password\n");
				return -1;
			}
			break;
		default:
			fprintf(stderr, "Error: Unknown option '%c'\n", c);
		}
	}

	if (optind != argc - 1) {
		usage(argv[0]);
		exit(1);
	}

	filename = argv[optind];

    // Prompt for password if not provided on the command line
    if (passlen == 0)
    {
        passlen = read_password(pass_input, mode);

        switch (passlen)
        {
            case 0: //no password in input
                fprintf(stderr, "Error: No password supplied.\n");
                res = -1;
                goto err;
            case AESCRYPT_READPWD_FOPEN:
            case AESCRYPT_READPWD_FILENO:
            case AESCRYPT_READPWD_TCGETATTR:
            case AESCRYPT_READPWD_TCSETATTR:
            case AESCRYPT_READPWD_FGETC:
            case AESCRYPT_READPWD_TOOLONG:
                fprintf(stderr, "Error in read_password: %s.\n",
                        read_password_error(passlen));
                res = -1;
                goto err;
            case AESCRYPT_READPWD_NOMATCH:
                fprintf(stderr, "Error: Passwords don't match.\n");
                res = -1;
                goto err;
        }

        passlen = passwd_to_utf16(  pass_input,
                                    strlen(pass_input),
                                    MAX_PASSWD_LEN+1,
                                    pass);
        if (passlen < 0)
        {
            res = -1;
            goto err;
        }
    }

	dev_fd = open("/dev/accord-le", O_RDWR);
	if (dev_fd == -1) {
		fprintf(stderr, "failed to open device: %m\n");
		res = 1;
        goto err;
	}

	/*data_fd = open(filename, O_RDONLY);
	if (data_fd == -1) {
		fprintf(stderr, "failed to open %s: %m\n", filename);
		goto err1;
	}*/

    if ((infp = fopen(filename, "r")) == NULL)
    {
            fprintf(stderr, "Error opening input file %s : ", filename);
            res = -1;
            goto err1;
    }

    memset(buf, 0xff, ACLE_SECTOR_SIZE);

	res = decrypt_stream(infp, buf, pass, passlen);

	        // If there was an error, remove the output file
        if (res)
        {
            fprintf(stderr, "failed to decrypt database file: %d\n", res);
            goto err1;
        }

/*
FILE *outfp = NULL;
outfp = fopen ("./tmp", "w");
fwrite(buf, ACLE_SECTOR_SIZE, 1, outfp);*/

    struct acle_data_chunk page = {
        .offset = offset * ACLE_SECTOR_SIZE,
        .count = ACLE_PAGE_SIZE,
    };

    fprintf(stderr, "erasing sector #%d\n", offset);
    res = ioctl(dev_fd, ACLE_CMD_ERASE_SECTOR, offset);
    if (res) {
        fprintf(stderr, "failed to erase sector #%d: %m\n", offset);
        goto err1;
    }

    page.data = buf;

    for (j = 0; j < ACLE_SECTOR_SIZE / ACLE_PAGE_SIZE; ++j) {
        fprintf(stderr, "writing page #%d\n", page.offset / ACLE_PAGE_SIZE);
        res = ioctl(dev_fd, ACLE_CMD_WRITE, &page);
        if (res) {
            fprintf(stderr, "failed to write page #%d: %m\n", page.offset / ACLE_PAGE_SIZE);
            res = 1;
            goto err1;
        }
        
        page.offset += ACLE_PAGE_SIZE;
        page.data += ACLE_PAGE_SIZE;
    }

    memset(buf, 0xff, ACLE_SECTOR_SIZE);

    //data_fd = NULL;


    /*for(i=0; i<ACLE_SECTOR_SIZE;i++)
    	printf("%x\t", buf[i]);*/

/*
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
			fprintf(stderr, "writing page #%d\n",
				page.offset / ACLE_PAGE_SIZE);
			res = ioctl(dev_fd, ACLE_CMD_WRITE, &page);
			if (res) {
				close(dev_fd);
				fprintf(stderr, "failed to write page #%d: %m\n",
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
*/
	fprintf(stderr, "data successfully written\n");
	res = 0;

//err2:
//	close(data_fd);
err1:
	close(dev_fd);
    if (infp != NULL)
    {
        fclose(infp);
    }
err:
	// For security reasons, erase the password
    memset(pass, 0, passlen);
	return res;
}
