#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <linux/limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include <zlib.h>

#include <ic-libs/fs/fs.h>
#include <ic-libs/fs/fs_ocl.h>

#define ACLE_SECTOR_SIZE	262144
#define fs_block_size 500*1024

/** блок vmdk трогать не будем - запомним */
unsigned char *vmbuf;
uint32_t vmsize;

static void usage()
{
	printf("convert_hd_serial - utility for convertion volume ID in ICL-sector of AMDZ-LE/GX/GXM/GXMH\n");
	printf("Usage:\n\tconvert_hd_serial icl.txt OLD_LABEL NEW_LABEL\n");
	printf("\n");
	printf("\ticl.txt\t\tfile with original icl-lists that should be converted\n");
	printf("\tOLD_LABEL\told label for volume that should be altered\n");
	printf("\tNEW_LABEL\tnew label for volume that should be inserted\n");
}

ssize_t read_icl(char *filename, void *buf, uint32_t offset, uint32_t size) {
	int res = 0;
	ssize_t readed;

	FILE* fd = NULL;
	fd = fopen(filename,"r");
	if(fd == NULL) {
		printf("fopen error\n");
		return -1;
	}

	if (offset != 0) {
		res = fseek(fd, offset, SEEK_SET);
		if (res) {
			printf("fseek error\n");
			return -2;
		}
	}

	readed = fread(buf, 1, size, fd);
	if (readed <= 0) {
		printf("fread error\n");
		return -3;
	}
	
	fclose(fd);
	return readed;
}

int write_icl(char *filename, uint8_t *buf, ssize_t size) {
	ssize_t written;

	FILE* fd = NULL;
	fd = fopen(filename, "w+");
	if(fd == NULL) {
		printf("fopen error\n");
		return -1;
	}

/*	if (offset != 0) {
		res = fseek(fd, offset, SEEK_SET);
		if (res) {
			printf("fseek error\n");
			return -2;
		}
	}
*/
	written = fwrite(buf, 1, size, fd);
	if (written <= 0) {
		printf("fwrite error\n");
		return -3;
	}
	
	fclose(fd);
	return written;
}

unsigned char *read_fs_block_from_buf(unsigned char *raw_data,
                                      int *size/*,
                                      QList<fs_vmdk_image>
                                      *vmdk_disk_list*/)
{
	uint32_t offset = 0;
	
	uint32_t block_vm_size;
	uint32_t block_icl_size;
	uint32_t vmdk_size;
	uint32_t buf_size;
	unsigned char *icl_buf;
//	unsigned char *vm_buf;
	
//	QList<struct fs_vmdk_image> vmlist;

	memcpy(&block_vm_size, raw_data + offset, sizeof(block_vm_size));
	memcpy(&vmdk_size, raw_data + offset + sizeof(block_vm_size), sizeof(vmdk_size));

	vmbuf = (unsigned char *)malloc(vmdk_size);
	memcpy(vmbuf, raw_data + offset + sizeof(block_vm_size) + sizeof(vmdk_size), vmdk_size);
	
	memcpy(&block_icl_size, raw_data + offset + sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size, sizeof(block_icl_size));
	memcpy(&buf_size, raw_data + offset + sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size + sizeof(block_icl_size), sizeof(buf_size));
	
	icl_buf = (unsigned char *)malloc(buf_size);
	memcpy(icl_buf, raw_data + offset + sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size + sizeof(block_icl_size) + sizeof(buf_size), buf_size);

//	QByteArray ba = QByteArray::fromRawData((char *)vm_buf, vmdk_size);
//	QDataStream vm(&ba, QIODevice::ReadOnly);
//	vm >> vmlist;

	*size = buf_size;
//	*vmdk_disk_list = vmlist;
	// прихраним этот блок
//	vmbuf = vm_buf;
	vmsize = vmdk_size;

//if (strcmp((const char *)vm_buf, "") == 0)
//	memcpy(vmbuf, "0", vmdk_size);

	return icl_buf;
}

unsigned char *form_fs_block_to_write(/*QList<fs_vmdk_image> vmdk_disk_list,*/ unsigned char *data, int size, int *out_size) {
	uint32_t block_vm_size;
	uint32_t block_icl_size;
	uint32_t vmdk_size = vmsize;
	uint32_t buf_size;
	unsigned char* output_buf;
	unsigned char *vmdk_buf = vmbuf;

	buf_size = size;

//	QByteArray ba;
//	QDataStream vm(&ba, QIODevice::ReadWrite);
//	in.setVersion(QDataStream::Qt_4_7);
//	vm << vmdk_disk_list;
//	vmdk_size = ba.size();
//	vmdk_buf = (unsigned char*)malloc(vmdk_size);
//	memcpy(vmdk_buf, ba, vmdk_size);

	block_vm_size = sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size;
	block_icl_size = sizeof(block_icl_size) + sizeof(buf_size) + buf_size;

	output_buf = (unsigned char *)malloc(sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size + sizeof(block_icl_size) + sizeof(buf_size) + buf_size);

	memcpy(output_buf, &block_vm_size, sizeof(block_vm_size));
	memcpy(output_buf + sizeof(block_vm_size), &vmdk_size, sizeof(vmdk_size));

	memcpy(output_buf + sizeof(block_vm_size) + sizeof(vmdk_size), vmdk_buf, vmdk_size);
//	memcpy(output_buf + sizeof(block_vm_size) + sizeof(vmdk_size), "0", vmdk_size);

	memcpy(output_buf + sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size, &block_icl_size, sizeof(block_icl_size));
	memcpy(output_buf + sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size + sizeof(block_icl_size), &buf_size, sizeof(buf_size));
	memcpy(output_buf + sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size + sizeof(block_icl_size) + sizeof(buf_size), data, buf_size);

	*out_size = sizeof(block_vm_size) + sizeof(vmdk_size) + vmdk_size + sizeof(block_icl_size) + sizeof(buf_size) + buf_size;
	return output_buf;
}

int uncompress_ic(unsigned char **dest, uLongf dest_len, unsigned char *sourse, int source_len) {
	*dest = (unsigned char*)malloc(dest_len);
	int res = uncompress(*dest, &dest_len, sourse, (uLongf)source_len);
	if (res)
		return -1;
	return 0;
}

int compress_ic(unsigned char **dest, uLongf *dest_len, unsigned char *sourse, int source_len) {
	*dest = (unsigned char*)malloc(source_len);
	*dest_len = (uLongf)source_len;
	
	int res = compress2(*dest, dest_len, sourse, (uLongf)source_len, Z_BEST_COMPRESSION);
	if (res)
		return -1;
	return 0;
}

unsigned char *wrap_compress_buf(unsigned char *buf, int size, int uncompress_size, int *out_size) {
	unsigned char *tmp_buf = (unsigned char*)malloc(size + sizeof(uint32_t) + sizeof(uint32_t));
	
	memcpy(tmp_buf, &size, sizeof(uint32_t));
	memcpy(tmp_buf + sizeof(uint32_t), &uncompress_size, sizeof(uint32_t));
	memcpy(tmp_buf + sizeof(uint32_t) + sizeof(uint32_t), buf, size);

	*out_size = size + sizeof(uint32_t) + sizeof(uint32_t);
	return tmp_buf;
}

/* unsigned char *read_fs_block_from_device (int *size, QList<fs_vmdk_image> *vmdk_disk_list) */
unsigned char *uncompress_icl(char *filename, int *size) {
	int res;
	uint32_t offset = 0;
	uint32_t uncompress_size;
	uint32_t compress_size;
	uint64_t header;
	unsigned char *compress;
	unsigned char *uncompress;

	res = read_icl(filename, &header, offset, sizeof(header));
	if (res <= 0)
  		return NULL;
	uncompress_size = header >> 32;
	compress_size = (uint32_t)header;

	compress = (unsigned char*)malloc(compress_size);

	res = read_icl(filename, compress, offset + sizeof(header), compress_size);
	if (res <= 0)
		return NULL;
	
	res = uncompress_ic(&uncompress, (uLongf)uncompress_size, compress, compress_size);
	if (res)
		return NULL;
	return read_fs_block_from_buf(uncompress, size/*, vmdk_disk_list*/);

}

/*int save_fs_block_to_device(unsigned char *icl_buf, int icl_size, QList<fs_vmdk_image> vlist) */
int compress_icl(char *filename, unsigned char *icl_buf, int icl_size) {
	unsigned char *_buf;
	unsigned char *compress_buf;
	unsigned char *buf_to_write;
	uLongf compress_buf_size;
	int _size;
	int buf_to_write_size;

//	uint32_t offset = 0; //hw_block_size + hd_block_size + reg_block_size;

	_buf = form_fs_block_to_write(/*vlist, */icl_buf, icl_size, &_size);

	/* Сжимаем буфер */
	int res = compress_ic(&compress_buf, &compress_buf_size, _buf, _size);
	if (res)
		return -1;

	buf_to_write = wrap_compress_buf(compress_buf, compress_buf_size, _size, &buf_to_write_size);
	
	/* Проверяем размер перед записью */
	if (buf_to_write_size >= fs_block_size)
		return -2;

	/* Записываем буфер в память */
	res = write_icl(filename, buf_to_write, /*offset, */buf_to_write_size);
	if (res <= 0) {
		return -3;
	}
	return 0;
}

static void print_volume_id(struct fs_volume_id *volume_id)
{
	int type = fs_volume_id_type(volume_id);

	if (type == FS_VOLUME_TYPE_PARTITION) {
		struct fs_partition_id *partition_id = (struct fs_partition_id *) volume_id;

		printf("%s partition %d\n", partition_id->disk_uuid, partition_id->partition_number);
	} else if (type == FS_VOLUME_TYPE_IMAGE_PARTITION) {
		struct fs_image_partition_id *image_partition_id =
			(struct fs_image_partition_id *) volume_id;

		printf("%s partition %d\n", image_partition_id->image_name, image_partition_id->partition_number);
	} else if (type == FS_VOLUME_TYPE_LVM_VOLUME ) {
		struct fs_lvm_volume_id *lvm_volume_id =
			(struct fs_lvm_volume_id *) volume_id;

		printf("%s/%s\n", lvm_volume_id->volume_group, lvm_volume_id->logical_volume);
	}
}

void print_hd_label(struct fs_icl *icl) {
	struct fs_icl_volume *volume;

	volume = fs_icl_first_volume(icl);
	while (volume != NULL) {
		print_volume_id(volume->volume_id);
		volume = fs_icl_next_volume(icl, volume);
	}
}

void change_hd_label(struct fs_icl *icl, char *old_label, char *new_label) {
	struct fs_icl_volume *volume;

	volume = fs_icl_first_volume(icl);
	while (volume != NULL) {
		struct fs_volume_id *volume_id = volume->volume_id;
		int type = fs_volume_id_type(volume_id);
		if (type == FS_VOLUME_TYPE_PARTITION) {
			struct fs_partition_id *partition_id = (struct fs_partition_id *)volume_id;
			if (strcmp(partition_id->disk_uuid, old_label) == 0) {
				free(partition_id->disk_uuid);
				partition_id->disk_uuid = strdup(new_label);
			}				
		}
		else if (type == FS_VOLUME_TYPE_IMAGE_PARTITION) {
//			((struct fs_image_partition_id *) volume_id)->image_name...
		}
		else if (type == FS_VOLUME_TYPE_LVM_VOLUME) {
//			((struct fs_lvm_volume_id *) volume_id)->volume_group...
		}
		volume = fs_icl_next_volume(icl, volume);
	}
}

int main (int argc, const char **argv) {
	int res = 0;
	char icl_filename[PATH_MAX+1] = {0};
	char *old_label;
	char *new_label;
	
	if (argc < 4) {
		usage();
		return 0;
	}

	if (argv[1] != NULL)
		strncpy(icl_filename, argv[1], PATH_MAX);
	else
		strncpy(icl_filename, "icl.txt", PATH_MAX);

	if (argv[2] != NULL) {
		old_label = strdup(argv[2]);
		if (old_label == NULL) {
			res = ENOMEM;
			goto end;
		}
	}
	if (argv[3] != NULL) {
		new_label = strdup(argv[3]);
		if (new_label == NULL) {
			res = ENOMEM;
			goto end1;
		}
	}

	/** чтобы корректно использовать ic-libs нужно сделать это */
	struct fs_icl *icl;
	fs_icl_create(&icl);

	unsigned char *icl_mem;
	int icl_mem_size;

	icl_mem = uncompress_icl(icl_filename, &icl_mem_size);
	if (icl_mem == NULL) {
		printf("error uncompressing icl\n");
		res = 1;
		goto end2;
	}
	if ((fs_icl_read_from_mem(icl_mem, (size_t)icl_mem_size, &icl)) != FS_OK) {
		printf("error reading icl buffer\n");
		res = 1;
		goto end3;
	}

	printf("Trying to find volume id`s... There are following volumes in ICL lists:\n");
	print_hd_label(icl);
	printf("\nTrying to change volume id`s ['%s' -> '%s']...\n\n", old_label, new_label);
	change_hd_label(icl, old_label, new_label);
	printf("Now there are following actual volumes in ICL lists:\n");
	print_hd_label(icl);

	unsigned char *_icl_mem;
  	size_t _size;

	if (fs_icl_write_to_mem(icl, &_icl_mem, &_size) != FS_OK) {
		printf("error writing icl to buffer\n");
		res = 1;
		goto end2;
	}

	res = compress_icl(/*strcat(*/icl_filename,/* ".altered"),*/ _icl_mem, _size);
	if (res) {
		printf("error writing icl to file: %d\n", res);
		res = 1;
		goto end4;
	}

// еще должны очищаться другие буферы из compress/uncompress и т.п.
end4:
	free(_icl_mem);
end3:
	free(icl_mem);
	free(vmbuf);
end2:
	fs_icl_destroy(icl);
	free(new_label);
end1:
	free(old_label);
end:
	return 0;
}