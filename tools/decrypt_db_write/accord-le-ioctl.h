#ifndef _ACCORDLE_IOCTL_H_
#define _ACCORDLE_IOCTL_H_

/** Кусок данных для записи во flash-память */
struct acle_data_chunk {
	/** Смещение в байтах */
	size_t offset;
	/** Количество данных в байтах*/
	size_t count;
	/** Данные */
	const void *data;
};

struct acle_login_request {
	uint8_t hash[32];
	uint32_t *userno;
	uint32_t *user_flags;
};

struct acle_rng_request {
	size_t count;
	void *buf;
};

#define LE_SERVICE_MODE	0x01
#define LE_ACCORD_MODE	0x02

enum {
	LE_MODE_ON = 0,
	LE_MODE_ADMIN = 1,
	LE_MODE_OFF = 2,
};

/** Состояние контроллера */
struct acle_state {
	uint8_t mode;
	uint32_t serial;
	uint16_t nusers;
	uint16_t logged_userno;
	uint8_t logged_user_flags;
	uint8_t hw_flags;
};

#define ACLE_CMD_MAGIC		'a'
#define ACLE_CMD_WRITE		_IOW(ACLE_CMD_MAGIC, 1, struct acle_data_chunk)
#define ACLE_CMD_ERASE_SECTOR	_IOW(ACLE_CMD_MAGIC, 2, int)
#define ACLE_CMD_LOGIN		_IOWR(ACLE_CMD_MAGIC, 3, struct acle_login_request)
#define ACLE_GET_BOARD_COUNT	_IOR(ACLE_CMD_MAGIC, 4, int *)
#define ACLE_CMD_LOGOUT		_IO(ACLE_CMD_MAGIC, 5)
#define ACLE_CMD_GET_RANDOM_BYTES	_IOWR(ACLE_CMD_MAGIC, 6, void *)
#define ACLE_CMD_GET_STATE	_IOR(ACLE_CMD_MAGIC, 7, struct acle_state *)
#define ACLE_CMD_WRITE_TO_BUF0	_IOW(ACLE_CMD_MAGIC, 8, struct acle_data_chunk)

#define ACLE_PAGE_SIZE		256
#define ACLE_SECTOR_SIZE	262144

#endif /* _ACCORDLE_IOCTL_H_ */
