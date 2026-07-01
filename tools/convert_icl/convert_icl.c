#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <ctype.h>

#include <zlib.h>

#include <ic-libs/fs/fs.h>
#include <ic-libs/fs/fs_ocl.h>

//#define ACLE_SECTOR_SIZE	262144
#define fs_block_size 500*1024

/** блок vmdk трогать не будем - запомним */
unsigned char *vmbuf;
uint32_t vmsize;

/** эталонный маленький сектор с СКЦ */
const int icl_sector_size=159;
char icl_sector[]={
	0x97, 0x00, 0x00, 0x00, 0xAC, 0x00, 0x00, 0x00, 
	0x78, 0xDA, 0xE3, 0x61, 0x60, 0x60, 0x60, 0x61, 
	0x80, 0x80, 0x05, 0x40, 0x3C, 0x03, 0x88, 0x19, 
	0x81, 0x98, 0x9F, 0x21, 0xDC, 0x45, 0x37, 0xDC, 
	0xD9, 0x31, 0xCC, 0xC8, 0xC2, 0xDC, 0xC2, 0xD8, 
	0xD0, 0xC8, 0x8C, 0x91, 0x81, 0x09, 0xAC, 0x48, 
	0x82, 0x41, 0x3F, 0xB8, 0xB2, 0xB8, 0x24, 0x35, 
	0x57, 0x2F, 0x37, 0xB1, 0x40, 0xD7, 0x58, 0xCF, 
	0x58, 0xCF, 0x42, 0x37, 0x3D, 0x35, 0xAF, 0x24, 
	0x3F, 0x9F, 0x85, 0xA1, 0xD8, 0xF9, 0xCA, 0xD3, 
	0xFD, 0x1D, 0x0C, 0x79, 0x5C, 0x97, 0x7F, 0xFE, 
	0xBB, 0x3B, 0xBD, 0xA8, 0x35, 0xB6, 0x85, 0x67, 
	0x6F, 0xA8, 0x78, 0xA8, 0xE0, 0x6E, 0x23, 0x43, 
	0xB5, 0xE5, 0x0A, 0xB2, 0x8D, 0x0C, 0x0C, 0x92, 
	0x68, 0xBA, 0xCD, 0xF5, 0x0C, 0x0D, 0xE0, 0xDA, 
	0x8F, 0x8A, 0xDB, 0xB2, 0xB9, 0x4F, 0xB9, 0x70, 
	0x47, 0x4F, 0xA4, 0x72, 0x72, 0x5D, 0xED, 0xC3, 
	0x48, 0xD5, 0xBB, 0xCC, 0x3F, 0xBB, 0x33, 0xCE, 
	0x9C, 0x95, 0x60, 0xCF, 0x38, 0xB4, 0x48, 0x66, 
	0xDF, 0x01, 0x00, 0x60, 0xAE, 0x33, 0x20
};

static void usage()
{
	printf("convert_icl - utility for convertion AMDZ-5.5 ICL into ICL-sector for AMDZ-LE/GX/GXM/GXMH\n");
	printf("Usage:\n\tconvert_icl icl.txt [DEFAULT_DISK_№] [DEFAULT_PART_№]\n");
	printf("\n");
	printf("\ticl.txt\t\tfile with original AMDZ-5.5+ icl-lists that should be converted\n");
	printf("\tDEFAULT_DISK_№\tdefault HDD-disk № used to create icl-lists on AMDZ-5.5 [default = 1]\n");
	printf("\tDEFAULT_PART_№\tdefault partition starting № [in default, for C:\\ = 1, D:\\ = 2, etc]\n");
}

#include <sys/wait.h>
#include <errno.h>

int execute_parser (char *icl_filename, char *str_diskno, char *str_partno) {
	int res = 0;
	const char *_argv[5];
	_argv[0] = "./convert_icl.pl";
	_argv[1] = icl_filename;
	_argv[2] = str_diskno;
	_argv[3] = str_partno;
	_argv[4] = NULL;


	int pid, status;
	// делаем fork
	pid = fork();
	if (pid == 0) {
		//execvp(args[0], &args[0]);
		execvp(_argv[0], (char * const *)_argv);
		// вывод сообщения об ошибке в случае если execvp не выполнится
		printf("error - failed to fork&exec\n");
		res = -1;
		goto out;
	}

	// ожидание окончания работы дочернего процесса
	else {
//		while (wait(&status) != pid)       /* wait for completion  */
//			;
//		sleep(3);
		if (wait(&status) == -1) {
			printf("error - failed to wait\n");
			res = -2;
			goto out;
		}

		// дочерний процесс завершился сам (с помощью exit() или _exit())
		if (WIFEXITED(status)) {
//printf("status = %d\n", WEXITSTATUS(status));
			if (WEXITSTATUS(status)==0) {
				res = 0;
				goto out;
			}
			else if (WEXITSTATUS(status)!=0) {
				printf("error - something wrong with the parser\n");
				res = 1;
				goto out;
			}
		}

		// выполнение дочернего процесса завершено сигналом
		if (WIFSIGNALED(status)) {
			printf("error - terminated by a signal #%i\n", WTERMSIG(status));
			res = -3;
			goto out;
		}
			
		if (WCOREDUMP(status)) {
			printf("error - dumped core\n");
			res = -5;
			goto out;
		}
			
		// выполнение дочернего процесса приостановлено сигналом
		if (WIFSTOPPED(status)) {
			printf("error - stopped by a signal #%i\n", WSTOPSIG(status));
			res = -6;
			goto out;
		}
			
	// в результате wait(&status) убивает дочерний процесс и предотвращает появление zombie-process
	}
out:
	return res;
}

int read_icl_string(char *filename, char *path, int *diskno, int *partno) {
	int c;
	int i = 0;

	FILE* fd = NULL;
	fd = fopen(filename, "r");
	if (fd == NULL) {
		printf("fopen error\n");
		return -1;
	}

	c = fgetc(fd);
	if ((c < '0' && c > '9') || c == -1)
		return -1;
	else
		*diskno = c - '0';

	c = fgetc(fd);
//	if (isdigit(c) != 0)
	if ((c < '0' && c > '9') || c == -1)
		return -1;
	else
		*partno = c - '0';

	while ((c = fgetc(fd))!= '\n') {
		path[i++] = c;
	}
	return 0;
}

ssize_t read_icl(void *buf, uint32_t offset, uint32_t size) {
/*	int res = 0;
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
*/
	memcpy(buf, icl_sector + offset, size);
	return size;
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
unsigned char *uncompress_icl(int *size) {
	int res;
	uint32_t offset = 0;
	uint32_t uncompress_size;
	uint32_t compress_size;
	uint64_t header;
	unsigned char *compress;
	unsigned char *uncompress;

	res = read_icl(&header, offset, sizeof(header));
	if (res <= 0)
		return NULL;

	uncompress_size = header >> 32;
	compress_size = (uint32_t)header;

	compress = (unsigned char*)malloc(compress_size);

	res = read_icl(compress, offset + sizeof(header), compress_size);
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

static void print_indent(int indent)
{
	while (indent > 0) {
		printf("  ");

		indent--;
	}
}

static void print_icl_hash(fs_icl_hash_t *hash)
{
	int i;
	const unsigned char *buf = hash->buf;

	printf("[");
	for (i = 0; i < sizeof(hash->buf); i++) {
		printf("%02x", buf[i]);
	}
	printf("]");
}

static void print_icl_object_flags(unsigned int flags)
{
	printf("[");
	if (flags & FS_ICL_FLAG_EACH_FILE) {
		printf(" EACH_FILE");
	}
	if (flags & FS_ICL_FLAG_RECURSIVE) {
		printf(" RECURSIVE");
	}
	if (flags & FS_ICL_FLAG_CONTROL_DATA) {
		printf(" CONTROL_DATA");
	}
	if (flags & FS_ICL_FLAG_CONTROL_ATTRS) {
		printf(" CONTROL_ATTRS");
	}
	printf(" ]");
}

static void print_icl_dir_masks(char **masks)
{
	char **cur_masks;

	printf("[");
	if (masks != NULL) {
		cur_masks = masks;
		while (*cur_masks != NULL) {
			printf(" '%s'", *cur_masks);
			cur_masks++;
		}
	}
	printf(" ]");
}

static void print_icl_file_attrs(struct fs_icl_file_attrs *attrs)
{
	printf("[mode: %04o, uid: %d, git: %d, size: %d, mtime: %d]",
	       attrs->mode, attrs->uid, attrs->gid, attrs->size, attrs->mtime);
}

static void print_icl_dir_attrs(struct fs_icl_dir_attrs *attrs)
{
	printf("[mode: %04o, uid: %d, git: %d]",
	       attrs->mode, attrs->uid, attrs->gid);
}

static void print_icl_dir_info(int indent, struct fs_icl_dir_info *di)
{
	int i;

#if 0
	if (di->nsubdirs == 0 && di->nfiles == 0) {
		printf("!!! dir_info error !!!");
		exit(1);
	}
#endif

	for (i = 0; i < di->nsubdirs; i++) {
		print_indent(indent);
		printf("+%s", di->subdir_names[i]);
		if (di->subdir_attrs) {
			printf(" ");
			print_icl_dir_attrs(di->subdir_attrs + i);
		}
		printf("\n");

		print_icl_dir_info(indent + 1, di->subdirs[i]);
	}

	for (i = 0; i < di->nfiles; i++) {
		print_indent(indent);
		printf(" %s", di->filenames[i]);
		if (di->file_hashes) {
			printf(" ");
			print_icl_hash(di->file_hashes +i);
		}
 		if (di->file_attrs) {
			printf(" ");
			print_icl_file_attrs(di->file_attrs + i);
		}
		printf("\n");
	}
}

static void print_icl_dir_object(struct fs_icl_dir *dir)
{
	printf("  D %s", dir->base.name);
	printf(" ");
	print_icl_object_flags(dir->flags);
	printf(" ");
	print_icl_dir_masks(dir->masks);
	printf("\n");

	if(dir->flags & FS_ICL_FLAG_EACH_FILE) {
		if (dir->flags & FS_ICL_FLAG_CONTROL_ATTRS) {
			printf("    <root> ");
			print_icl_dir_attrs(dir->root_dir_attrs);
			printf("\n");
		}
		print_icl_dir_info(3, dir->root_dir);
	} else {
		printf("    ");
		print_icl_hash(dir->hash);
		printf("\n");
	}
}

static void print_icl_file_object(struct fs_icl_file *file)
{
	printf("  F %s", file->base.name);
	printf(" ");
	print_icl_object_flags(file->flags);
	printf("\n");

	if (file->flags & (FS_ICL_FLAG_CONTROL_DATA | FS_ICL_FLAG_CONTROL_ATTRS)) {
		printf("    ");
		if (file->flags & FS_ICL_FLAG_CONTROL_DATA) {
			print_icl_hash(file->hash);
			printf(" ");
		}

		if (file->flags & FS_ICL_FLAG_CONTROL_ATTRS) {
			print_icl_file_attrs(file->attrs);
			printf(" ");
		}
		printf("\n");
	}
}

static void print_icl_object(struct fs_icl_object *obj)
{
	if (obj->type == FS_ICL_OBJ_FILE) {
		print_icl_file_object((struct fs_icl_file *) obj);
	} else /* if (obj->type == FS_ICL_OBJ_DIR) */ {
		print_icl_dir_object((struct fs_icl_dir *) obj);
	}
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

static void print_icl_volume(struct fs_icl_volume *volume)
{
	struct fs_icl_object *obj;

	print_volume_id(volume->volume_id);

	obj = fs_icl_volume_first_object(volume);
	while (obj != NULL) {
		print_icl_object(obj);
		obj = fs_icl_volume_next_object(obj);
	}
}

//void print_hd_label(struct fs_icl *icl) {
void print_icl(struct fs_icl *icl) {
	struct fs_icl_volume *volume;

	volume = fs_icl_first_volume(icl);
	while (volume != NULL) {
//		print_volume_id(volume->volume_id);
		print_icl_volume(volume);

		volume = fs_icl_next_volume(icl, volume);
	}
}

/**
 * @internal
 * Удаляет объект содержащий информацию о разделе с контролируемыми объектами.
 *
 * fs_volume_id_destroy = ...(volume_id)->ops->destroy(volume_id)
 */
static void destroy_volume(struct fs_icl *icl, struct fs_icl_volume *volume)
{
	fs_ocl_destroy(volume->ocl);

	icl->nvolumes--;
	list_del(&volume->volume);
	fs_volume_id_destroy(volume->volume_id);
	free(volume);
}

void clear_icl_files1(struct fs_icl *icl) {
	struct list_head *p, *next;

	list_for_each_safe(p, next, &icl->volume_list) {
		struct fs_icl_volume *volume;

		volume = list_entry(p, struct fs_icl_volume, volume);
		//fs_icl_remove_object(icl, volume->volume_id, "/System.map-3.3.8-gentoo");
		fs_icl_remove_object(icl, volume->volume_id, "/System.map-3.7.10-gentoo");
	}
	return;
}

void clear_icl_files2(struct fs_icl *icl) {
	struct list_head *p, *next;

	list_for_each_safe(p, next, &icl->volume_list) {
		struct fs_icl_volume *volume;

		volume = list_entry(p, struct fs_icl_volume, volume);
		fs_icl_remove_object(icl, volume->volume_id, "/System.map-3.3.8-gentoo");
		//fs_icl_remove_object(icl, volume->volume_id, "/System.map-3.7.10-gentoo");
	}
	return;
}

void clear_icl(struct fs_icl *icl) {
	struct list_head *p, *next;

	list_for_each_safe(p, next, &icl->volume_list) {
		struct fs_icl_volume *volume;

		volume = list_entry(p, struct fs_icl_volume, volume);
		destroy_volume(icl, volume);
	}
	return;
}

struct fs_icl_volume *create_default_volume(struct fs_icl *icl, struct fs_icl_volume *volume, int part_number) {
	int type = fs_volume_id_type(volume->volume_id);
	if (type == FS_VOLUME_TYPE_PARTITION) {
		struct fs_volume_id *pcopy;
		int res = 0;
		struct fs_icl_volume *_volume = malloc (sizeof(*_volume));
		if (_volume == NULL)
			return NULL;

		volume->volume_id->ops->copy(volume->volume_id, &pcopy);

		_volume->volume_id = pcopy;
		((struct fs_partition_id *) (_volume->volume_id))->partition_number = part_number;
		res = fs_ocl_create(&_volume->ocl);
		if (res != FS_OK) {
			free(_volume);
			return NULL;
		}
		list_add(&_volume->volume, &icl->volume_list);
		icl->nvolumes++;

		return _volume;
	}
	return NULL;
}

int main (int argc, const char **argv) {
	int res = 0;
	char icl_filename[PATH_MAX+1] = {0};
	char icl_filename_tmp[PATH_MAX+1+3] = {0};
	char saved_icl[] = "./fs.bin";

	int diskno;
	int partno;
	int flags = 0;
	flags += FS_OCL_FLAG_CONTROL_DATA;	// we`ll use data control in ICL-check

	char path[PATH_MAX+1] = {0};
	fs_ocl_hash_t hash;
		
	if (argc < 2 || argc > 4) {
		usage();
		return 0;
	}

	if (argv[1] != NULL)
		strncpy(icl_filename, argv[1], PATH_MAX);
	else
		strncpy(icl_filename, "icl.txt", PATH_MAX);
	sprintf(icl_filename_tmp, "%s.tmp", icl_filename);

	if (argc >= 2 &&  argv[2] != NULL)
		diskno = atoi(argv[2]);
	else
		diskno = 1;

	if (argc >= 3 && argv[3] != NULL)
		partno = atoi(argv[3]);
	else
		partno = 1;

	/** чтобы корректно использовать ic-libs нужно сделать это */
	struct fs_icl *icl;
	fs_icl_create(&icl);
	
	/** создадим */
	unsigned char *icl_mem;
	int icl_mem_size;

	icl_mem = uncompress_icl(&icl_mem_size);
	if (icl_mem == NULL) {
		printf("error uncompressing icl\n");
		res = 1;
		goto out2;
	}
	if ((fs_icl_read_from_mem(icl_mem, (size_t)icl_mem_size, &icl)) != FS_OK) {
		printf("error reading icl buffer\n");
		res = 1;
		goto out3;
	}

	printf("Preparing icl list...\n");
	/**
	 * удаляем лишний объект целостности
	 * просто влом делать новый дама...
	 */
	clear_icl_files1(icl);
//	print_icl(icl);


	/** partition 1, тут должен остаться один файл */
	struct fs_icl_volume *first_volume = fs_icl_first_volume(icl);

	/** обнулим хеш */
	strncpy((void *)&hash, "\0", FS_OCL_HASH_SIZE);
//	hash = form_hash("117B772880D38629F5A7B0EB5E9970629243544C53D3C4574FEF3C055745117B");

	int icl_eof = 0;
	int i;
	for (i=0; icl_eof != 1; ++i) {
		memset(path, 0, PATH_MAX+1);
		char str_diskno[3];
		sprintf(str_diskno, "%d", diskno);
		char str_partno[3];
		sprintf(str_partno, "%d", partno);

		res = execute_parser(icl_filename, str_diskno, str_partno);
		if (res != 0)
			goto out3;

		res = read_icl_string(icl_filename_tmp, path, &diskno, &partno);
		if (res == -1) {
			icl_eof = 1;
			break;
		}

		if (partno == 1) {
			/** тут просто добавим в существующий раздел */
			res = fs_ocl_add_file_info(first_volume->ocl, path, flags, &hash, NULL, NULL);
			if (res != FS_OK) {
				printf("error: can't add file '%s' to icl\n", path);
				res = 1;
				goto out3;
			}
		}
		else {
			/** тут создадим новый раздел на том же диске */
			struct fs_icl_volume *_volume = create_default_volume(icl, first_volume, partno);
			if (_volume == NULL) {
				printf("error: can't create partition %d\n", partno);
				res = 1;
				goto out3;
			}
			/** и добавим файл уже в созданный раздел */
			res = fs_ocl_add_file_info(_volume->ocl, path, flags, &hash, NULL, NULL);
			if (res != FS_OK) {
				printf("error: can't add file '%s' to icl\n", path);
				res = 1;
				goto out3;
			}
		}
//		print_icl(icl);
	}

	/**
	 * удаляем лишний файл
	 * если его удалить раньше - придется самому создавать volume-ы
	 */
	clear_icl_files2(icl);
	printf("Got following ICL list...\n");
	print_icl(icl);

	unsigned char *_icl_mem;
  	size_t _size;

  	/** пишем в буфер */
	if (fs_icl_write_to_mem(icl, &_icl_mem, &_size) != FS_OK) {
		printf("error writing icl to buffer\n");
		res = 1;
		goto out2;
	}

	/** сжимаем и записываем в файл */
	res = compress_icl(saved_icl, _icl_mem, _size);
	if (res) {
		printf("error writing icl to file: %d\n", res);
		res = 1;
		goto out4;
	}

out4:
	free(_icl_mem);

/**
 * еще должны очищаться другие буферы
 * но т.к. это нарезка из ic-libs
 * как это корректно делать неочевидно
 */
out3:
	free(icl_mem);
	free(vmbuf);
out2:
	fs_icl_destroy(icl);
//out1:
	return 0;
}
