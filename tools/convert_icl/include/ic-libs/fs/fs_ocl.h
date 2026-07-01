/** @file fs_ocl.h API библиотеки контроля целостности объектов ФС */
#ifndef __IC_LIBS__FS__FS_OCL_H
#define __IC_LIBS__FS__FS_OCL_H

#include <ic-libs/fs/fs_err.h>
#include <ic-libs/misc/list.h>
#include <ic-libs/misc/rbtree.h>
#include <ic-libs/misc/read_buf.h>
#include <ic-libs/misc/write_buf.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Размер хэша. */
#define FS_OCL_HASH_SIZE	32

/** Тип данных для хэша. */
typedef struct {
	unsigned char buf[FS_OCL_HASH_SIZE];
} fs_ocl_hash_t;

/* typedef unsigned char fs_ocl_hash_t[FS_OCL_HASH_SIZE]; */

/** Определяет атрибуты контролируемые для файла. */
struct fs_ocl_file_attrs {
	uint32_t mode;
	uint16_t uid;
	uint16_t gid;
	uint32_t size;
	uint32_t mtime;
};

/** Определяет атрибуты контролируемые для каталога. */
struct fs_ocl_dir_attrs {
	uint32_t mode;
	uint16_t uid;
	uint16_t gid;
};

/** Определяет тип контролируемого объекта. */
enum {
	FS_OCL_OBJ_FILE = 0,
	FS_OCL_OBJ_DIR
};

/** Базовая структура описывающая контролируемый объект.*/
struct fs_ocl_object {
	struct rb_node obj;		/**< Элемент объектов раздела. */

	char *name;			/**< Имя (полный путь к объекту в файловой системе)
					     контролируемого объекта. */
	int type;			/**< Тип контролируемого объекта. */
};

/** Описывает контролируемый файл. */
struct fs_ocl_file {
	struct fs_ocl_object base;		/**< Базовая структура. */

	unsigned int flags;			/**< Флаги. */
	fs_ocl_hash_t *hash;			/**< Хэш для контролируемого файла. */
	struct fs_ocl_file_attrs *attrs;	/**< Атрибуты контролируемого файла. */
};

/** Описывает контролируемый каталог. */
struct fs_ocl_dir {
	struct fs_ocl_object base;	/**< Базовая структура. */

	unsigned int flags;		/**< Флаги. */
	char **masks;			/**< Список масок определяющий
					     файлы подлежащие контролю. */
	union {
		struct {
			struct fs_ocl_dir_info *root_dir;		/**< Информация о корневом каталоге. */
			struct fs_ocl_dir_attrs *root_dir_attrs;	/**< Атрибуты корневого каталога. */
		};
		fs_ocl_hash_t *hash;	/**< Хэш для контролируемого каталога.  */
	};
};

/** Содержит информацию об отдельном каталоге/подкаталоге контролируемого каталога. */
struct fs_ocl_dir_info {
	int nsubdirs;				/**< Количество подкаталогов. */
	char **subdir_names;			/**< Имена подкаталогов. */
	struct fs_ocl_dir_info **subdirs;	/**< Информация о подкаталогах. */
	struct fs_ocl_dir_attrs *subdir_attrs;	/**< Атрибуты подкаталогов. */

	int nfiles;				/**< Количество контролируемых файлов в каталоге. */
	char **filenames;			/**< Имена контролируемых файлов. */
	fs_ocl_hash_t *file_hashes;		/**< Хэши контролируемых файлов. */
	struct fs_ocl_file_attrs *file_attrs;	/**< Атрибуты файлов. */
};

/** Содержит информацию о контролируемых файлах и каталогах. */
struct fs_ocl {
	struct rb_root objs;		/**< Дерево (упорядоченный список)
					     контролируемых объектов (файлов и каталогов). */
	int nobjs;			/**< Количество контролируемых объектов. */
};

int fs_ocl_create(struct fs_ocl **pocl);
void fs_ocl_destroy(struct fs_ocl *ocl);

/** Флаги определяющие способ контроля для файлов и каталогов. */
enum {
	FS_OCL_FLAG_EACH_FILE		= 0x01,		/**< Контролировать каждый файл отдельно,
							     не вычислять только общий хэш на все. */
	FS_OCL_FLAG_RECURSIVE		= 0x02,		/**< С подкаталогами. */
	FS_OCL_FLAG_CONTROL_DATA	= 0x04,		/**< Контролировать содержимое файлов, если не задан,
							     то контролируется только наличие файлов. */
	FS_OCL_FLAG_CONTROL_ATTRS	= 0x08		/**< Контролировать атрибуты файлов и каталогов. */
};

int fs_ocl_add_file(struct fs_ocl *ocl, const char *base_path, const char *filename,
		    unsigned int flags, struct fs_ocl_file **pfile);
int fs_ocl_add_file_info(struct fs_ocl *ocl, const char *filename, unsigned int flags,
			 fs_ocl_hash_t *hash, struct fs_ocl_file_attrs *attrs, struct fs_ocl_file **pfile);
int fs_ocl_add_dir(struct fs_ocl *ocl, const char *base_path, const char *dirname,
		   char *const *masks, unsigned int flags, struct fs_ocl_dir **pdir);
int fs_ocl_remove_object(struct fs_ocl *ocl, const char *name);

struct fs_ocl_object *fs_ocl_find_object(struct fs_ocl *ocl,  const char *name);
struct fs_ocl_object *fs_ocl_first_object(struct fs_ocl *ocl);
struct fs_ocl_object *fs_ocl_next_object(struct fs_ocl_object *obj);

int fs_ocl_write_to_mem(struct fs_ocl *ocl, unsigned char **pmem, size_t *pmem_size);
int fs_ocl_read_from_mem(unsigned char *mem, size_t mem_size, struct fs_ocl **pocl);

int _fs_ocl_write_to_wbuf(struct fs_ocl *ocl, struct write_buf *wbuf);
int _fs_ocl_read_from_rbuf(struct read_buf *rbuf, struct fs_ocl **pocl);

/** Флаги типов измнений контролируемых объектов. */
enum {							/* F D */
	FS_OCL_FLAG_FILE_NEW		= 0x01,		/*   + */
	FS_OCL_FLAG_FILE_REMOVED	= 0x02,		/* + + */
	FS_OCL_FLAG_FILE_MODIFIED	= 0x04,		/* + + */
	FS_OCL_FLAG_FILE_ATTR_MODE	= 0x08,		/* + + */
	FS_OCL_FLAG_FILE_ATTR_UID	= 0x10,		/* + + */
	FS_OCL_FLAG_FILE_ATTR_GID	= 0x20,		/* + + */
	FS_OCL_FLAG_FILE_ATTR_SIZE	= 0x40,		/* + + */
	FS_OCL_FLAG_FILE_ATTR_MTIME	= 0x80,		/* + + */

	FS_OCL_FLAG_DIR_ROOT_HASH	= 0x0100,	/*   + */
	FS_OCL_FLAG_DIR_REMOVED		= 0x0200,	/*   + */
	FS_OCL_FLAG_DIR_ATTR_MODE	= 0x0400,	/*   + */
	FS_OCL_FLAG_DIR_ATTR_UID	= 0x0800,	/*   + */
	FS_OCL_FLAG_DIR_ATTR_GID	= 0x1000	/*   + */
};

/** Содержит информацию об изменениях контролируемых объектов. */
struct fs_ocl_check_info {
	struct rb_root objs;		/**< Дерево (упорядоченный список)
					     объектов (файлов и каталогов) с изменениями. */
	int nobjs;			/**< Количество объектов. */
};

/** Базовая структура для структур описывающих изменения контролируемых объектов.*/
struct fs_ocl_check_object {
	struct rb_node obj;		/**< Элемент списка объектов. */
	int type;			/**< Тип контролируемого объекта. */
};

/** Описывает изменения для контролируемого файла. */
struct fs_ocl_check_file {
	struct fs_ocl_check_object base;/**< Базовая структура. */

	struct fs_ocl_file *file;	/**< Контролируемый файл для которого обнаружены изменения. */
	unsigned int flags;		/**< Типы изменений обнаруженные для файла. */
};

/** Описывает изменения для контролируемого каталога. */
struct fs_ocl_check_dir {
	struct fs_ocl_check_object base;/**< Базовая структура. */

	struct fs_ocl_dir *dir;		/**< Контролируемый каталог для которого обнаружены изменения. */
	struct list_head item_list;	/**< Список изменений обнаруженных для подобъектов каталога. */
};

/** Описывает отдельное изменение для подобъекта контролируемого каталога. */
struct fs_ocl_check_dir_item {
	struct list_head item;		/**< Элемент списка изменений каталога. */

	char *name;			/**< Имя подобъекта каталога для которого обнаружены изменения. */
	unsigned int flags;		/**< Типы изменений обнаруженные для подобъекта каталога. */
};

int fs_ocl_check_info_create(struct fs_ocl *ocl, const char *base_path, struct fs_ocl_check_info **pcheck_info);
void fs_ocl_check_info_destroy(struct fs_ocl_check_info *check_info);

struct fs_ocl_check_object *fs_ocl_check_find_object(struct fs_ocl_check_info *check_info, const char *name);

struct fs_ocl_check_object *fs_ocl_check_first_object(struct fs_ocl_check_info *check_info);
struct fs_ocl_check_object *fs_ocl_check_next_object(struct fs_ocl_check_object *obj);
struct fs_ocl_check_dir_item *fs_ocl_check_first_dir_item(struct fs_ocl_check_dir *dir);
struct fs_ocl_check_dir_item *fs_ocl_check_next_dir_item(struct fs_ocl_check_dir *dir,
							 struct fs_ocl_check_dir_item *item);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IC_LIBS__FS__FS_OCL_H */
