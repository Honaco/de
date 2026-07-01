/** @file fs.h API библиотеки контроля целостности объектов ФС */
#ifndef __IC_LIBS__FS__FS_H
#define __IC_LIBS__FS__FS_H

#include <ic-libs/fs/fs_ocl.h>
#include <ic-libs/misc/list.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fs_disk_list;

/** Типы разделов. */
enum {
	FS_VOLUME_TYPE_PARTITION = 0,
	FS_VOLUME_TYPE_IMAGE_PARTITION,
	FS_VOLUME_TYPE_LVM_VOLUME
};

/** Базовая структура для идентификаторов разделов (разделов дисков, lvm-разделов). */
struct fs_volume_id {
	struct fs_volume_id_ops *ops;
};

/** Содержит список виртуальных операций для идентификатора раздела. */
struct fs_volume_id_ops {
	int type;
	int (*copy)(struct fs_volume_id *volume_id, struct fs_volume_id **pcopy);
	int (*compare)(struct fs_volume_id *volume_id1, struct fs_volume_id *volume_id2);
	struct fs_volume *(*find_volume)(struct fs_volume_id *volume_id, struct fs_disk_list *disk_list);
	int (*write)(struct fs_volume_id *volume_id, struct write_buf *wbuf);
	void (*destroy)(struct fs_volume_id *volume_id);
};

/** Возвращает тип идентификатора раздела. */
static inline int fs_volume_id_type(struct fs_volume_id *volume_id)
{
	return volume_id->ops->type;
}

/** Cоздает копию идентификатора раздела. */
static inline int fs_volume_id_copy(struct fs_volume_id *volume_id, struct fs_volume_id **pcopy)
{
	return volume_id->ops->copy(volume_id, pcopy);
}

/** Сравнивает два раздела идентификатора раздела. */
static inline int fs_volume_id_compare(struct fs_volume_id *volume_id1, struct fs_volume_id *volume_id2)
{
	return volume_id1->ops->compare(volume_id1, volume_id2);
}

/** Удаляет идентификатор раздела. */
static inline void fs_volume_id_destroy(struct fs_volume_id *volume_id)
{
	volume_id->ops->destroy(volume_id);
}

/** Идентификатор раздела диска. */
struct fs_partition_id {
	struct fs_volume_id_ops *ops;	/**< Указатель на таблицу виртуальных функций. */
	char *disk_uuid;		/**< Уникальный идентификатор диска. */
	int partition_number;		/**< Номер раздела. */
};

/** Идентификатор раздела образа диска. */
struct fs_image_partition_id {
	struct fs_volume_id_ops *ops;	/**< Указатель на таблицу виртуальных функций. */
	char *image_name;		/**< Имя образа диска. */
	int partition_number;		/**< Номер раздела. */
};

/** Идентификатор lvm-раздела. */
struct fs_lvm_volume_id {
	struct fs_volume_id_ops *ops;	/**< Указатель на таблицу виртуальных функций. */
	char *volume_group;		/**< Имя группы. */
	char *logical_volume;		/**< Имя раздела группы.*/
};

int fs_partition_id_create(const char *disk_uuid, int partition_number,
			   struct fs_partition_id **ppartition_id);
int fs_image_partition_id_create(const char *image_name, int partition_number,
				 struct fs_image_partition_id **pimage_partition_id);
int fs_lvm_volume_id_create(const char *volume_group, const char *logical_volume,
			    struct fs_lvm_volume_id **plvm_volume_id);

#define FS_VOLUME_MEMBERS \
	struct fs_volume_ops *ops; \
\
	int mount_count;		/**< Счетчик монтирования раздела. */ \
	char *mount_dir;		/**< Директория где смонтирован данный раздел. \
					     Если раздел не смонтирован, то равно NULL. */ \
	char *devnode;			/**< Путь к файлу устройства раздела в /dev. */ \
	char *fstype;			/**< Тип файловой системы раздела, если имеется. \
					     Если не имеется, то NULL.  */ \
	char *syspath			/**< Путь в /sys по которому содержится \
					     инфомация о данном разделе. */

/** Базовая структура для разделов (разделов дисков, lvm-разделов). */
struct fs_volume {
	FS_VOLUME_MEMBERS;
};

/** Содержит список виртуальных операций для раздела. */
struct fs_volume_ops {
	int type;
	int (*create_id)(struct fs_volume *volume, struct fs_volume_id **pvolume_id);
	struct fs_disk_list *(*get_disk_list)(struct fs_volume *volume);
};

/** Возвращает тип раздела */
static inline int fs_volume_type(struct fs_volume *volume)
{
	return volume->ops->type;
}

/** Создает идентификатор раздела. */
static inline int fs_volume_create_id(struct fs_volume *volume, struct fs_volume_id **pvolume_id)
{
	return volume->ops->create_id(volume, pvolume_id);
}

/** Содержит информацию о дисках. */
struct fs_disk_list {
	struct list_head disk_list;	/**< Список дисков. */
	struct list_head image_disk_list;	/**< Список образов дисков. */
	struct list_head lvm_group_list;

	int mount_points;		/**< Счетчик смонтированных разделов дисков. */
	char *base_mount_dir;		/**< Базовый каталог в котором создаются подкаталоги
					     для монтирования разделов дисков. */
};

/** Содержит информацию о диске. */
struct fs_disk {
	struct list_head disk;		/**< Элемент списка дисков. */
	struct list_head part_list;	/**< Список разделов диска. */

	struct fs_disk_list *disk_list; /**< Указатель на список дисков в котором
					     содержится данный диск. */

	char *syspath;			/**< Путь в /sys по которому содержится
					     инфомация о данном диске. */
	char *devnode;			/**< Путь к файлу устройства диска в /dev. */

	char *uuid;			/**< Уникальный идентификатор диска. */
};

/** Содержит информацию о разделе. */
struct fs_partition {
	FS_VOLUME_MEMBERS;

	struct list_head part;		/**< Элемент списка разделов. */

	struct fs_disk *disk;		/**< Указатель на диск на котором находится данный раздел */

	int number;			/**< Номер раздела. */
	char *uuid;			/**< uuid раздела, если имеется. Если не имеется, то NULL. */
};

/** Содержит информацию об образе диска. */
struct fs_image_disk {
	struct list_head disk;		/**< Элемент списка дисков. */
	struct list_head part_list;	/**< Список разделов диска. */

	struct fs_disk_list *disk_list; /**< Указатель на список дисков в котором
					     содержится данный диск. */

	char *syspath;			/**< Путь в /sys по которому содержится
					     инфомация о данном диске. */
	char *devnode;			/**< Путь к файлу устройства диска в /dev. */

	char *name;			/**< Имя образа диска. */
	struct fs_volume *volume;	/**< Раздел на котором находится образ диска. */
	char *image_file;		/**< Имя файла с образом диска. */
};

/** Содержит информацию о разделе с образа диска. */
struct fs_image_partition {
	FS_VOLUME_MEMBERS;

	struct list_head part;		/**< Элемент списка разделов. */

	struct fs_image_disk *disk;	/**< Указатель на диск на котором находится данный раздел */

	int number;			/**< Номер раздела. */
	char *uuid;			/**< uuid раздела, если имеется. Если не имеется, то NULL. */
};

/** Содержит информацию о lvm-группе. */
struct fs_lvm_group {
	struct list_head group;		/**< Элемент списка групп. */
	struct list_head volume_list;	/**< Список разделов группы */

	struct fs_disk_list *disk_list; /**< Указатель на список дисков в котором
					     содержится данная группа. */

	char *name;			/**< Имя группы. */
};

/** Содержит информацию о lvm-разделе. */
struct fs_lvm_volume {
	FS_VOLUME_MEMBERS;

	struct list_head volume;	/**< Элемент списка разделов. */
	struct fs_lvm_group *group;	/**< Указатель на группу в которой содержится данный раздел */

	char *name;			/**< Имя раздела. */
};

int fs_disk_list_create(struct fs_disk_list **plist);
void fs_disk_list_destroy(struct fs_disk_list *list);

int fs_disk_list_add_lvm_groups(struct fs_disk_list *disk_list);

int fs_disk_list_add_image_disk(struct fs_disk_list *disk_list,
				struct fs_volume *volume, const char *image_file,
				const char *image_name, struct fs_image_disk **pdisk);
int fs_disk_list_remove_image_disk(struct fs_disk_list *disk_list, struct fs_image_disk *disk);

struct fs_volume *fs_disk_list_find_volume(struct fs_disk_list *list, struct fs_volume_id *volume_id);

struct fs_disk *fs_disk_list_first_disk(struct fs_disk_list *list);
struct fs_disk *fs_disk_list_next_disk(struct fs_disk_list *list, struct fs_disk *disk);
struct fs_partition *fs_disk_partition_by_number(struct fs_disk *disk, int part_number);
struct fs_partition *fs_disk_first_partition(struct fs_disk *disk);
struct fs_partition *fs_disk_next_partition(struct fs_disk *disk, struct fs_partition *part);

struct fs_image_disk *fs_disk_list_first_image_disk(struct fs_disk_list *list);
struct fs_image_disk *fs_disk_list_next_image_disk(struct fs_disk_list *list, struct fs_image_disk *disk);
struct fs_image_partition *fs_image_disk_partition_by_number(struct fs_image_disk *disk, int part_number);
struct fs_image_partition *fs_image_disk_first_partition(struct fs_image_disk *disk);
struct fs_image_partition *fs_image_disk_next_partition(struct fs_image_disk *disk, struct fs_image_partition *part);

struct fs_lvm_group *fs_disk_list_first_lvm_group(struct fs_disk_list *list);
struct fs_lvm_group *fs_disk_list_next_lvm_group(struct fs_disk_list *list, struct fs_lvm_group *group);
struct fs_lvm_volume *fs_lvm_group_first_volume(struct fs_lvm_group *group);
struct fs_lvm_volume *fs_lvm_group_next_volume(struct fs_lvm_group *group, struct fs_lvm_volume *volume);

int fs_volume_get_mount_point(struct fs_volume *volume, int rw, const char **pmount_point);
int fs_volume_release_mount_point(struct fs_volume *volume);

/** Содержит информацию о контролируемых файлах и каталогах. */
struct fs_icl {
	struct list_head volume_list;	/**< Список разделов. */
	int nvolumes;			/**< Количество разделов. */
};

/** Содержит информацию о контролируемых файлах и каталогах для раздела. */
struct fs_icl_volume {
	struct list_head volume;	/**< Элемент списка разделов. */

	struct fs_ocl *ocl;		/**< Содержит список контролируемых объетов. */
	struct fs_volume_id *volume_id;	/**< Идентификатор носителя. */
};


int fs_icl_create(struct fs_icl **picl);
void fs_icl_destroy(struct fs_icl *icl);

int fs_icl_add_file(struct fs_icl *icl, struct fs_volume *volume, const char *filename,
		    unsigned int flags, struct fs_ocl_file **pfile);
int fs_icl_add_file_info(struct fs_icl *icl, struct fs_volume *volume, const char *filename,
			 unsigned int flags, fs_ocl_hash_t *hash, struct fs_ocl_file_attrs *attrs,
			 struct fs_ocl_file **pfile);
int fs_icl_add_dir(struct fs_icl *icl, struct fs_volume *volume, const char *dirname,
		   char *const *masks, unsigned int flags, struct fs_ocl_dir **pdir);
struct fs_ocl_object *fs_icl_find_object(struct fs_icl *icl, struct fs_volume_id *volume_id, const char *name);
int fs_icl_remove_object(struct fs_icl *icl, struct fs_volume_id *volume_id, const char *name);

struct fs_icl_volume *fs_icl_first_volume(struct fs_icl *icl);
struct fs_icl_volume *fs_icl_next_volume(struct fs_icl *icl, struct fs_icl_volume *volume);
struct fs_ocl_object *fs_icl_volume_first_object(struct fs_icl_volume *volume);
struct fs_ocl_object *fs_icl_volume_next_object(struct fs_ocl_object *obj);
struct fs_ocl_object *fs_icl_volume_find_object(struct fs_icl_volume *volume, const char *name);

int fs_icl_write_to_mem(struct fs_icl *icl, unsigned char **pmem, size_t *pmem_size);
int fs_icl_read_from_mem(unsigned char *mem, size_t mem_size, struct fs_icl **picl);

int fs_icl_check_info_create(struct fs_icl_volume *icl_volume, struct fs_volume *volume,
			     struct fs_ocl_check_info **pcheck_info);

#define fs_icl_hash_t fs_ocl_hash_t
#define fs_icl_file_attrs fs_ocl_file_attrs
#define fs_icl_dir_attrs fs_ocl_dir_attrs
#define fs_icl_object fs_ocl_object
#define fs_icl_file fs_ocl_file
#define fs_icl_dir fs_ocl_dir
#define fs_icl_dir_info fs_ocl_dir_info

#define FS_ICL_HASH_SIZE FS_OCL_HASH_SIZE

#define FS_ICL_OBJ_FILE FS_OCL_OBJ_FILE
#define FS_ICL_OBJ_DIR FS_OCL_OBJ_DIR

#define FS_ICL_FLAG_EACH_FILE FS_OCL_FLAG_EACH_FILE
#define FS_ICL_FLAG_RECURSIVE FS_OCL_FLAG_RECURSIVE
#define FS_ICL_FLAG_CONTROL_DATA FS_OCL_FLAG_CONTROL_DATA
#define FS_ICL_FLAG_CONTROL_ATTRS FS_OCL_FLAG_CONTROL_ATTRS


#define FS_ICL_FLAG_FILE_NEW FS_OCL_FLAG_FILE_NEW
#define FS_ICL_FLAG_FILE_REMOVED FS_OCL_FLAG_FILE_REMOVED
#define FS_ICL_FLAG_FILE_MODIFIED FS_OCL_FLAG_FILE_MODIFIED
#define FS_ICL_FLAG_FILE_ATTR_MODE FS_OCL_FLAG_FILE_ATTR_MODE
#define FS_ICL_FLAG_FILE_ATTR_UID FS_OCL_FLAG_FILE_ATTR_UID
#define FS_ICL_FLAG_FILE_ATTR_GID FS_OCL_FLAG_FILE_ATTR_GID
#define FS_ICL_FLAG_FILE_ATTR_SIZE FS_OCL_FLAG_FILE_ATTR_SIZE
#define FS_ICL_FLAG_FILE_ATTR_MTIME FS_OCL_FLAG_FILE_ATTR_MTIME
#define FS_ICL_FLAG_DIR_ROOT_HASH FS_OCL_FLAG_DIR_ROOT_HASH
#define FS_ICL_FLAG_DIR_REMOVED FS_OCL_FLAG_DIR_REMOVED
#define FS_ICL_FLAG_DIR_ATTR_MODE FS_OCL_FLAG_DIR_ATTR_MODE
#define FS_ICL_FLAG_DIR_ATTR_UID FS_OCL_FLAG_DIR_ATTR_UID
#define FS_ICL_FLAG_DIR_ATTR_GID FS_OCL_FLAG_DIR_ATTR_GID

#define fs_icl_check_info fs_ocl_check_info
#define fs_icl_check_object fs_ocl_check_object
#define fs_icl_check_file fs_ocl_check_file
#define fs_icl_check_dir fs_ocl_check_dir
#define fs_icl_check_dir_item fs_ocl_check_dir_item

#define fs_icl_check_info_destroy fs_ocl_check_info_destroy
#define fs_icl_check_find_object fs_ocl_check_find_object
#define fs_icl_check_first_object fs_ocl_check_first_object
#define fs_icl_check_next_object fs_ocl_check_next_object
#define fs_icl_check_first_dir_item fs_ocl_check_first_dir_item
#define fs_icl_check_next_dir_item fs_ocl_check_next_dir_item

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IC_LIBS__FS__FS_H */
