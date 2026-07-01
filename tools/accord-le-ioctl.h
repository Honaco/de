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

/** Данные аутнетификации передаваемы в login */
struct acle_login_request {
    /** Свертка от пароля и идентификатора*/
	uint8_t hash[32]; 
    /** Номер аутентифицируемого пользователя */
	uint32_t *userno;
    /** Флаги аутнетификации */
	uint32_t *user_flags;
};

/** Стуктура данных получения случайных чисел */
struct acle_rng_request {
    /** Колиичесвто байт для чтения ДСЧ */
	size_t count;
    /** Буфер для полчаемых случайных байт из аппартного ДСЧ*/
	char *buf;
};

// sw-флаги, порядковые номера битов
// По ACC_MISC_HAVE_SDCARD_BIT/ACC_MISC_HAVE_RTC_BIT можно понять, есть ли мезонин на GX+
//
// вроде сообщения для ос, что watchdog установлен, но 'не сработал'
#define  ACC_MISC_WD_ENABLED_BIT  0
// потециально есть 'набортный' считыватель TM
#define  ACC_MISC_HAVE_TMI_BIT    1
// есть валидный уникальный номер
#define  ACC_MISC_HAVE_UID_BIT    2
// серийник 'несменяемый'
#define  ACC_MISC_HARD_SN_BIT     3
// лля GX+ - есть SD-карта (реально)
#define  ACC_MISC_HAVE_SDCARD_BIT 4
// для GX+ - есть RTC (реально)
#define  ACC_MISC_HAVE_RTC_BIT    5
// для GX+ - сработал датчик вскрытия
#define  ACC_MISC_INTRUDER_BIT    6

// hw-флаги
#define LE_SERVICE_MODE      1
#define LE_ACCORD_MODE       2
#define LE_WD_ON             4
// присутствует ли one-wire-interface
#define LE_HAVE_OWI          8
#define LE_HAVE_UID          16
#define LE_HARD_SN           32
#define LE_HAVE_SDCARD       64
#define LE_HAVE_RTC          128
#define LE_INTRODUCER        256
enum {
    LE_INTERNAL_FIRMWARE_CHECK_RESULT = 1 << 28
};

enum {
	LE_MODE_ON = 0,
	LE_MODE_ADMIN = 1,
	LE_MODE_OFF = 2,
    LE_MODE_USER = 3,
};

/** Структура описания адресов и размеров сегмента  памяти  базы данных*/
typedef struct area_rec_s {
    uint32_t code; ///< Идентификатор области памяти
    uint32_t address; ///< смещение до области памяти
    uint32_t size; ///< размер области памяти
} area_rec_t;

#define  NUM_AREAS 6
#define  AREA_CODE_CONFIG     0x10
#define  AREA_CODE_BIOS       0x11
#define  AREA_CODE_USER_BASE  0x12
#define  AREA_CODE_COMMON     0x13
#define  AREA_CODE_LOG        0x14
#define  AREA_CODE_OSIMG      0x15
static const char MEMTABLE_VALID_SIGNATURE[] = "MEMTABLE";

/** Струткурв описания областей памяти базы данных СДЗ */
typedef  struct memtable_s
{
    uint8_t signature[8]; ///< Сигнатура
    area_rec_t area_rec[NUM_AREAS]; ///< Список областей базы даннх СДЗ
} memtable_t;

#define MEMTABLE_HW_VERSION_LO 0x35    
#define MEMTABLE_HW_VERSION_HI 0x01

/** Состояние контроллера */
struct acle_state {
    uint8_t mode; ///< режим работы контролреа
    uint32_t serial; ///< сеийный номер
    uint16_t nusers; ///< колличесвто учетных записей  в базе 
    uint16_t logged_userno; ///< номер залогиневшегося пользователя 
    uint8_t logged_user_flags; ///< атрибуты залогиневшегося пользоватлея 
    uint32_t hw_flags; ///< параметры контроллера (преобразованные (неполностью) sw_flags)
    uint8_t fw_ver_hi; ///< Майджор версия прошивки  РКБ СДЗ
    uint8_t fw_ver_lo; ///< Минор версии прошивки  РКБ СДЗ
    uint16_t sw_flags; ///< Атрибуты контроллера (as-is)
    uint32_t flash_id; ///< Идентификтаор флеш памяти БД СДЗ
    memtable_t memtabel; ///< Разметка базы данных СДЗ
    uint32_t watchdog_timer; ///< Время от PowerOn до Reset Watchdog
    uint32_t board_uid; ///< Уникальный идентификатор платы АМДЗ
    uint32_t flash_id_4; ///< 4 байта - идентификатора flash-памяти (есть и меньше)
    uint32_t sdc_first_sector; ///< первый сектор sd-карты контроллера ('контроллера' в смысле данные через него идут, через чип)
};
#pragma pack(push, 1)
struct acle_tmid {
	/** Тип */
	uint8_t type;
	/** Серийный номер */
	uint8_t sn[6];
	/** CRC от типа и серийного номера */
	uint8_t crc;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct acle_rtc_time {
    uint16_t year; /**< Год */
    uint8_t month; /**< Месяц */
    uint8_t day; /**< День */
    uint8_t hour; /**< Час */
    uint8_t minute; /**< Минута */
    uint8_t second; /**< Секунда */
};
#pragma pack(pop)

#pragma pack(push, 1)
struct acle_sd_chunk {
    uint32_t sector_number;
    uint32_t offset;
    uint32_t size;
	const void *data;

};
#pragma pack(pop)



#define ACLE_CMD_MAGIC		'a'
#define ACLE_CMD_WRITE		_IOW(ACLE_CMD_MAGIC, 1, struct acle_data_chunk)
#define ACLE_CMD_ERASE_SECTOR	_IOW(ACLE_CMD_MAGIC, 2, uint32_t)
#define ACLE_CMD_LOGIN		_IOWR(ACLE_CMD_MAGIC, 3, struct acle_login_request)
#define ACLE_GET_BOARD_COUNT	_IOR(ACLE_CMD_MAGIC, 4, int *)
#define ACLE_CMD_LOGOUT		_IO(ACLE_CMD_MAGIC, 5)
#define ACLE_CMD_GET_RANDOM_BYTES	_IOWR(ACLE_CMD_MAGIC, 6, void *)
#define ACLE_CMD_GET_STATE	_IOR(ACLE_CMD_MAGIC, 7, struct acle_state *)
#define ACLE_CMD_WRITE_TO_BUF0	_IOW(ACLE_CMD_MAGIC, 8, struct acle_data_chunk)
#define ACLE_CMD_READ_FROM_BUF0 _IOR(ACLE_CMD_MAGIC, 9, void *)
#define ACLE_CMD_TM_PRESENT _IOR(ACLE_CMD_MAGIC, 10, void *)
#define ACLE_CMD_TM_GET_ID _IOR(ACLE_CMD_MAGIC, 11, void *)
#define ACLE_CMD_TM_READ_PAGE _IOR(ACLE_CMD_MAGIC, 12, void *)
#define ACLE_CMD_TM_WRITE_PAGE _IOW(ACLE_CMD_MAGIC, 13, void *)
#define ACLE_DISABLE_WATCHDOG_TIMER _IOW(ACLE_CMD_MAGIC, 14, void *)
#define ACLE_CMD_SET_RTC _IOW(ACLE_CMD_MAGIC, 15, struct acle_rtc_time)
#define ACLE_CMD_GET_RTC _IOR(ACLE_CMD_MAGIC, 16, struct acle_rtc_time *)
#define ACLE_CMD_SD_READ _IOR(ACLE_CMD_MAGIC, 17, void *)
#define ACLE_CMD_SD_WRITE _IOW(ACLE_CMD_MAGIC, 18, void *)
#define ACLE_CMD_SET_RANDOM _IOW(ACLE_CMD_MAGIC, 19, int)
#define ACLE_CMD_SET_SDC _IOW(ACLE_CMD_MAGIC, 20, int)

#define ACLE_PAGE_SIZE		256
#define ACLE_SECTOR_SIZE	262144
// он же размер сектора sd-карты
#define ACLE_MAX_SD_BLOCK_SIZE 512
//#define ACLE_SECTOR_SIZE    65536

#endif /* _ACCORDLE_IOCTL_H_ */
