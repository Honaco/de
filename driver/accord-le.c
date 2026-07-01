#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <linux/compat.h>

#include "accord-le.h"
#include "accord-le-ioctl2.h"

// игнорировать ошибку из logout
#define ACCORDLE_CFG_IGNORE_LOGOUT_ERROR

#define SET_SDC_0

#define ACCORDLE_VID	0x1795
/** Аккорд-LE PCI-E v1.0 */
#define ACCORDLE_DID	0x0700
/** Аккорд-GX PCI-E v1.0 */
#define ACCORDGX_DID	0x0710
/** Аккорд-GX mini PCI-E v1.0 */
#define ACCORDGXM_DID	0x0715
/** Аккорд-GX mini PCI-E half height v1.0 */
#define ACCORDGXMH_DID	0x0720
/** Аккорд-GX2 M.2*/
#define ACCORDGX2AE_DID 0x0725
/** Аккорд-GX2 mini PCI-E half heigh*/
#define ACCORDGX2MH_DID 0x0730 
/** Аккорд-GX5 M.2*/
#define ACCORDGX5AE_DID 0x0735
/** Зарезервированная плата*/
#define RESERVE_DID 0x0740
#define RESERVE_DID2 0x0745

#define ACCORDSP_VID2	0x14E4
#define ACCORDSP_DID2	0x4358

#define DRIVER_NAME	"accord-le"
#ifndef DRIVER_VERSION
#define DRIVER_VERSION "0.9.999"
#endif
#define PREFIX		"accord-le: "

#define MAX_READ_SIZE	4096U
#define ACLE_IADATA_SIZE 512

//#define _ACLE_DEBUG_
#ifdef _ACLE_DEBUG_
static bool _acle_debug_ = true;
#else
static bool _acle_debug_ = false;
#endif

#define acle_debug_print(fmt, ...) do {\
  if (_acle_debug_) \
    printk(KERN_DEBUG PREFIX "%d:%s(): " fmt, __LINE__, __func__, ##__VA_ARGS__); \
} while (0)

/** Параметры микросхемы памяти */
static const struct flash_chip_info {
	/** Модель микросхемы */
	uint32_t chip_id;
	/** Полный объем памяти в байтах */
	size_t total_size;
	/** Размер сектора в байтах */
	size_t sector_size;
	/** Размер страницы в байтах */
	size_t page_size;
} flash_chip_table[] = {
	{M25P32_EXTID,  4 * 1024 * 1024,  64 * 1024,  256},
	{M25P64_EXTID,  8 * 1024 * 1024,  64 * 1024,  256},
	{M25P128_EXTID, 16 * 1024 * 1024, 256 * 1024, 256},
    {MT25QL128A_EXTID, 16*1024*1024,  64*1024,     256}, 
    {S25FL128P_EXTID,  16*1024*1024,  256*1024,    256}, 
    {N25Q256A_EXTID,   32*1024*1024,   64*1024,   256},
    {S25FL256S_EXTID,   32*1024*1024,   256*1024,   256},
    {S25FL512S_EXTID,   64*1024*1024,   256*1024,   256},
    {MT25QL512A_EXTID,    64*1024*1024,   64*1024,   256},
    {W25Q256JV_EXTID,   32*1024*1024, 64*1024, 256},
    {W25Q512JV_EXTID,   64*1024*1024, 64*1024, 256},
    {W25Q256JVD_EXTID,  32*1024*1024, 64*1024, 256},
    {W25Q512JVD_EXTID,  64*1024*1024, 64*1024, 256},
//	{M25P16_EXTID,  2 * 1024 * 1024,  64 * 1024,  256},
//	{M25P80_EXTID,  1 * 1024 * 1024,  64 * 1024,  256},
//	{M25P40_EXTID,  512 * 1024,       64 * 1024,  256},
//	{M25P10_EXTID,  128 * 1024,       32 * 1024,  128},
};
#define FLASH_CHIP_TABLE_SIZE (sizeof(flash_chip_table) / sizeof(flash_chip_table[0]))

/** Микросхема памяти */
struct flash_chip {
	struct list_head list;
	/** Параметры микросхемы */
	const struct flash_chip_info *info;
	/** Номер микросхемы */
	int target;
	/** Линейное смещение */
	size_t offset;
};

/** Описание PCI устройства СДЗ*/
static struct acle_data {
	/** Адрес отображения памяти ввода/вывода из BAR1 в ядро для команд */
	void __iomem *cmd_mem;
	/** Базовый порт ввода/вывода из BAR1 для команд */
	uint16_t cmd_port;
	/** Адрес отображения памяти ввода/вывода из BAR0 в ядро для данных */
	void __iomem *data_mem;
	/** Буфер для чтения/записи */
	void *iobuf;
	/** Размер NAND в байтах */
	size_t nand_size;
	/** Код завершения предыдущей операции */
	int errno;
	/** Буфер для записываемой страницы */
	void *page;
	/** Список микросхем памяти на плате */
	struct list_head flash_chips;
} acle_data;

#pragma pack(push, 1)
// binary-coded-decimal
struct BCDTime {
    uint8_t bcd_second; // c[0,59]
    uint8_t bcd_minute; // c[0,59]
    uint8_t bcd_hour;   // c[0,23]
};

struct BCDDate {
    uint8_t bcd_day;   // c[1,days_in_month[month]]
    uint8_t bcd_month; // c[1,12]
    uint8_t bcd_year;  // uint8_t, т.к. после 2000г
};

struct BCDDateTime {
   struct BCDTime time;
   uint8_t        bcd_day_of_week; // DOW, не используется?
   struct BCDDate date;
};
#pragma pack(pop)

/** Количество  плат СДЗ*/
static atomic_t board_count; 

const uint8_t days_in_month[12] = {31, 28, 31,
                                   30, 31, 30,
                                   31, 31, 30,
                                   31, 30, 31};

static uint8_t bcd_to_byte(uint8_t byte_in_bcd)
{
    return ((byte_in_bcd >> 4) * 10) + (0xF & byte_in_bcd);
}

static uint8_t byte_to_bcd(uint8_t byte)
{
    if (99 < byte)
        {
            return 0xFF;
        }
    else
        {
            return ((byte / 10) << 4) | (byte % 10);
        }
}

static int is_bcd_time_invalid(const struct BCDTime *input)
{
    uint8_t value = 0;

    if (NULL == input)
        {
            return 1;
        }

    value = bcd_to_byte(input->bcd_second);
    if (59 < value)
        {
            return 2;
        }

    value = bcd_to_byte(input->bcd_minute);
    if (59 < value)
        {
            return 3;
        }

    value = bcd_to_byte(input->bcd_hour);
    if (23 < value)
        {
            return 4;
        }

    return 0;
}

static int is_bcd_date_invalid(const struct BCDDate *input)
{
    uint8_t year = 0, month = 0, day = 0, max_days = 0;

    if (NULL == input)
        {
            return 10;
        }

    year = bcd_to_byte(input->bcd_year);

    month = bcd_to_byte(input->bcd_month);
    if (1 > month || month > 12)
        {
            return 20;
        }
    max_days = days_in_month[month];

    day = bcd_to_byte(input->bcd_day);
    if (1 > day)
        {
            return 30;
        }
    if ((0 == (0x3 & year)) && (2 == month))
        {
            // високосный февраль
            max_days++;
        }
    if (max_days < day)
        {
            return 40;
        }

    return 0;
}

static int is_bcd_datetime_invalid(const struct BCDDateTime *input)
{
    if (NULL == input)
        {
            return 100;
        }
    return is_bcd_date_invalid(&input->date)
         + is_bcd_time_invalid(&input->time);
}

static int bcd_to_acle_rtc(const struct BCDDateTime   *input,
                                 struct acle_rtc_time *output)
{
    int status = 0;

    if ((status = is_bcd_datetime_invalid(input)))
        {
            return status;
        }
    else if (NULL == output)
        {
            return 200;
        }

    output->year   = 2000 + bcd_to_byte(input->date.bcd_year);
    output->month  = bcd_to_byte(input->date.bcd_month);
    output->day    = bcd_to_byte(input->date.bcd_day);
    output->hour   = bcd_to_byte(input->time.bcd_hour);
    output->minute = bcd_to_byte(input->time.bcd_minute);
    output->second = bcd_to_byte(input->time.bcd_second);

    return 0;
}

static int acle_to_bcd_rtc(const struct acle_rtc_time *input,
                                 struct BCDDateTime   *output)
{
    int                status = 0;
    struct BCDDateTime result;

    memset(&result, 0, sizeof(result));

    if (NULL == input)
        {
            return 200;
        }
    else if (NULL == output)
        {
            return 100;
        }

    result.date.bcd_year   = byte_to_bcd(input->year - 2000);
    result.date.bcd_month  = byte_to_bcd(input->month);
    result.date.bcd_day    = byte_to_bcd(input->day);
    result.bcd_day_of_week = 0;
    result.time.bcd_hour   = byte_to_bcd(input->hour);
    result.time.bcd_minute = byte_to_bcd(input->minute);
    result.time.bcd_second = byte_to_bcd(input->second);

    if ((status = is_bcd_datetime_invalid(&result)))
        {
            return 300 + status;
        }

    *output = result;
    return 0;
}

/**
 * \def make_cmd_code(target, cmd, nargs, nres, bufno)
 * Генерирует команду для отправки на контроллер
 * @param [in] target - запрашиваемый физический участок флеш-памяти
 * @param [in] cmd    - запрашиваемая операция
 * @param [in] nargs  - сколько регистров с нулевого было обновлено на pci-шине (туда)
 * @param [in] nres   - сколько регистров с нулевого надо обновить на pci-шине (обратно)
 * @param [in] bufno  - какую область ОЗУ расширения использовать при
 *                      возможной передачи данных
 */
#define make_cmd_code(target, cmd, nargs, nres, bufno) ({ \
	(((nres) << 20) | ((nargs) << 16) | ((cmd) << 8) | ((bufno) << 4) | ((target) << 1) | 1); \
})

/**
 * @brief чтения данных из регистров PCI устройства по заданному смещению
 * @param [in] dev - устройство аппаратная  плата  СДЗ
 * @param [in] offset - смещение до ячейки памяти/регистра, из которой надо считать информацию.
 * @return считанные данные из регистров PCI платы СДЗ
 */
static unsigned int ioreadl(struct acle_data *dev, unsigned int offset)
{
	return (dev->cmd_mem) ? readl(dev->cmd_mem + offset)
		: inl(dev->cmd_port + offset);
}

/**
 * @brief запись  данных в  регистры  PCI устройства по заданному смещению
 * @param [in] dev - устройство аппартаная плата СДЗ
 * @param [in] offset - смещение до ячейки памяти/регистра, в которую происходит запись
 * @param [in] data - записываемы данные в плату
 */

static void iowritel(struct acle_data *dev, unsigned int offset,
	unsigned int data)
{
	if (dev->cmd_mem)
		writel(data, dev->cmd_mem + offset);
	else
		outl(data, dev->cmd_port + offset);
}

/**
 * Отправить команду в устройство
 *
 * @param[in] cmd_mem - базовый адрес памяти ввода/вывода
 * @param[in] cmd_code - код команды (должен быть создан с помощью make_cmd_code)
 * @param[in] delay - задержка в микросекундах до начала цикла опроса устройства
 * о завершении выполнения команды
 * @return 0 в случае успешного выполнения команды, иначе - код состояния устройства
 */
static int exec_cmd(struct acle_data *dev, uint32_t cmd_code, unsigned long delay)
{
	uint32_t res;

	iowritel(dev, REG_CMD, cmd_code);

	if (delay)
		udelay(delay);
	while ((res = (ioreadl(dev, REG_CMD) & 0x00000080)) == 0) {
		//schedule();
	}

	if (res & 0x00000040)
		return ioreadl(dev, REG_CMD);
	return 0;
}

/**
 * Поиск параметров микросхемы памяти в таблице
 *
 * @param id - идентификатор микросхемы
 * @return параметры микросхемы или NULL, если микросхемы нет в таблице
 */
static const struct flash_chip_info *find_flash_chip_info(uint32_t id)
{
	int i;
	for (i = 0; i < FLASH_CHIP_TABLE_SIZE; ++i)
		if (flash_chip_table[i].chip_id == id)
			return &flash_chip_table[i];
	return NULL;
}

/**
 * Инициализация списка микросхем памяти
 *
 * @param dev - параметры устройства
 * @return количество обнаруженных микросхем или -EIO в случае ошибки
 */
static int init_flash_chips(struct acle_data *dev)
{
	int target = TARGET_CF;
	int count = 0;
	size_t total = 0;

	if ((ioreadl(dev, REG_CMD) & 0x00000080) == 0)
		return -EIO;

	INIT_LIST_HEAD(&dev->flash_chips);

	while (target < TARGET_LAST) {
		uint32_t id, fullID;
		const struct flash_chip_info *chip_info;
		uint32_t cmd_code = make_cmd_code(target, LE_COMMAND_CODE_SPI_READ_ID,
			0, 2, SELECT_BUF0);
		int res = exec_cmd(dev, cmd_code, 0);
		if (res) {
			++target;
			continue;
		}
		id = (uint32_t) ioreadl(dev, REG_ARG_0);
    fullID = (uint32_t) ioreadl(dev, REG_ARG_1);

		acle_debug_print("Chip target=%d chip id=0x%x, chip fullid=0x%x\n", target, id, fullID);

		chip_info = find_flash_chip_info(id);
		if (chip_info) {
			struct flash_chip *chip = kmalloc(sizeof(*chip), GFP_KERNEL);
			if (!chip) {
				printk(KERN_WARNING PREFIX "kmalloc failed\n");
				goto out;
			}
			chip->info = chip_info;
			chip->target = target;
			chip->offset = total;
			list_add(&chip->list, &dev->flash_chips);
			total += chip->info->total_size;
			++count;
		}
		++target;
	}
out:
	dev->nand_size = total;
	return count;
}

/**
 * Чтение данных из flash-памяти
 *
 * @param[in] dev - параметры устройства
 * @param[in] offset - линейное смещение в байтах (должно быть выровнено
 * по границе 4 байт)
 * @param[in] count - размер читаемых данных (должен быть выровнен по границе 4 байт
 * и не более MAX_READ_SIZE)
 * @param[out] buf - буфер для сохранения прочитанных данных
 * @return 0 в случае успешного чтения данных или -EIO в случае ошибки
 */
static int read_flash(struct acle_data *dev, size_t offset,
	size_t count, void *buf)
{
	struct flash_chip *chip;
	size_t total = 0;

	if ((ioreadl(dev, REG_CMD) & 0x00000080) == 0)
		return -EIO;

	list_for_each_entry(chip, &dev->flash_chips, list) {
		uint32_t cmd_code;
		size_t _offset = offset - chip->offset;
		size_t _count = min(chip->info->total_size - _offset, count);
		if (_offset >= chip->info->total_size || _count == 0)
			continue;

		iowritel(dev, REG_ARG_0, _offset);
		iowritel(dev, REG_ARG_1, _count / 4);

		cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_SPI_READ_DATA,
			2, 0, SELECT_BUF0);
		if (exec_cmd(dev, cmd_code, 130))
			return -EIO;

		cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_NONE_NO_OPERATION,
			0, 0, SELECT_BUF1);
		if (exec_cmd(dev, cmd_code, 8))
			return -EIO;

		memcpy_fromio(buf + total, dev->data_mem, _count);

		offset += _count;
		count -= _count;
		total += _count;
	}
	return 0;
}

/**
 * @brief Реализация функции lseek для устройства accord_le
 * Функция утсанавливает позицию файла для операцией чтения и записи.
 * @param[in] file - представления файла
 * @param[in] offset - смещение в позиции  файла.
 * @param[in] whence - способ установки позиции. 
 * @return  Установленная позиция. 
 */

static loff_t acle_llseek(struct file *file, loff_t offset, int whence)
{
	loff_t newpos;

	switch (whence) {
	case SEEK_SET:
		if (offset >= acle_data.nand_size)
			return -EINVAL;
		newpos = offset;
		break;
	case SEEK_CUR:
		if (file->f_pos + offset >= acle_data.nand_size)
			return -EINVAL;
		newpos = offset + file->f_pos;
		break;
	default:
		return -EINVAL;
	}

	file->f_pos = newpos;
	return newpos;
}

/**
 * @brief Реализация функции read для устройства accord_le
 * Функция считывает блок данных по заданному смещению
 * @param[in] file - представления файла
 * @param[out] buf - буфер для получения данных.
 * @param[in] count - количество бит  для чтения. 
 * @param[in] offset - смещение, с которого производится чтения.
 * @return  Количество считанных байт.
 */

static ssize_t acle_read(struct file *file, char __user *buf,
	size_t count, loff_t *offset)
{
	int res = 0;
	size_t total = 0;
	/* выравниваем начало и размер по 4-х байтовой границе */
	size_t _count = (count % 4 == 0) ? count : (count + 4 - count % 4);
	loff_t _offset = (*offset < 4) ? 0 : (*offset - *offset % 4);
	loff_t cur = _offset;
	const size_t dcount = _count - count;
	const loff_t doff = *offset - _offset;

	if (acle_data.errno) {
		int tmp = acle_data.errno;
		acle_data.errno = 0;
		return tmp;
	}

	while (total < _count) {
		size_t chunk_size = min(_count - total, MAX_READ_SIZE);
		if (cur >= acle_data.nand_size)
			break;
		res = read_flash(&acle_data, cur, chunk_size, acle_data.iobuf);
		if (res)
			break;
		if (copy_to_user(buf + cur - _offset,
			acle_data.iobuf + (total ? 0 : doff),
			chunk_size - ((chunk_size == MAX_READ_SIZE) ? 0 : dcount) - (total ? 0 : doff))) {
			res = -EFAULT;
			break;
		}

		total += chunk_size;
		cur += chunk_size;
	}

	if (total == 0)
		return res;
	*offset = cur - doff;
	acle_data.errno = res;
	return count;
}

/**
 * Запись данных во flash-память
 *
 * @param[in] dev - параметры устройства
 * @param[in] offset - линейное смещение в байтах (должно быть выровнено
 * по границе 4 байт)
 * @param[in] count - размер записываемых данных (должен быть выровнен
 * по границе 4 байт), не более размера страницы (256 байт)
 * @param[in] buf - буфер с записываемыми данными
 * @return 0 в случае успешного чтения данных или -EIO в случае ошибки
 */
static int write_flash(struct acle_data *dev, size_t offset, size_t count,
	const void *buf)
{
	struct flash_chip *chip;
	size_t total = 0;

	if ((ioreadl(dev, REG_CMD) & 0x00000080) == 0)
		return -EIO;

	list_for_each_entry(chip, &dev->flash_chips, list) {
		uint32_t cmd_code;
		size_t _offset = offset - chip->offset;
		size_t _count = min(chip->info->total_size - _offset, count);
		if (_offset >= chip->info->total_size || _count == 0)
			continue;

		/* выбрали буфер 0 и разрешили доступ к нему с PCI */
		cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_NONE_NO_OPERATION,
			0, 0, PCI_WRITE_EN | SELECT_BUF0);
		if (exec_cmd(dev, cmd_code, 0))
			return -EIO;
		/* записали */
		memcpy_toio(dev->data_mem, buf, count);
		/* Разрешить запись во flash */
		cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_SPI_WRITE_ENABLE,
			0, 0, SELECT_BUF1);
		if (exec_cmd(dev, cmd_code, 0))
			return -EIO;
		/* Записываем */
		/* заносим агрументы */
		iowritel(dev, REG_ARG_0, _offset);
		iowritel(dev, REG_ARG_1, _count / 4);
		/* PCI_WRITE_EN or SELECT_BUF1 - чтобы Альтера имела доступ к буферу 0 */
		cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_SPI_PAGE_PROGRAM,
			2, 0, PCI_WRITE_EN | SELECT_BUF1);
		if (exec_cmd(dev, cmd_code, 0))
			return -EIO;
		/* опрашиваем статус-регистр flash, ждем завершения записи */
		cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_SPI_READ_STATUS,
			0, 1, SELECT_BUF1);
		do {
			if (exec_cmd(dev, cmd_code, 0))
				return -EIO;
		} while (ioreadl(dev, REG_ARG_0) & 0x01000000);

		offset += _count;
		count -= _count;
		total += _count;
	}

	return 0;
}

/**
 * Стирание логического сектора flash-памяти
 *
 * @param[in] dev - параметры устройства
 * @param[in] offset - линейное смещение в байтах (должно быть выровнено
 * по границе логического сектора, т.е. 262144 байта)
 * @return 0 в случае успешного чтения данных или -EIO в случае ошибки
 */
static int erase_flash_sector(struct acle_data *dev, size_t offset)
{
	struct flash_chip *chip;

	list_for_each_entry(chip, &dev->flash_chips, list) {
		uint32_t cmd_code;
		size_t _offset = offset - chip->offset;
		int count = ACLE_SECTOR_SIZE / (chip->info->sector_size );
		if (_offset >= chip->info->total_size)
			continue;

		while (count) {
			/* Разрешить запись во flash */
			cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_SPI_WRITE_ENABLE,
				0, 0, SELECT_BUF1);
			if (exec_cmd(dev, cmd_code, 0))
				return -EIO;
			/* команда стирания */
			iowritel(dev, REG_ARG_0, _offset);
			cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_SPI_SECTOR_ERASE,
				1, 0, PCI_WRITE_EN | SELECT_BUF1);
			if (exec_cmd(dev, cmd_code, 0))
				return -EIO;
			/* опрашиваем статус-регистр flash, ждем завершения записи */
			cmd_code = make_cmd_code(chip->target, LE_COMMAND_CODE_SPI_READ_STATUS,
				0, 1, SELECT_BUF1);
			do {
				if (exec_cmd(dev, cmd_code, 0))
					return -EIO;
			} while (ioreadl(dev, REG_ARG_0) & 0x01000000);

			_offset += (chip->info->sector_size);
			--count;
		}
	}

	return 0;
}


/**
 * @brief  Логин в СДЗ GX x64
 * Логин в плату СДЗ. После успешного логина база данных открывается 
 * в соответствии с правами пользователя.
 * Обрабочик для вызова кода x64
 * @param[in] dev - параметры устройства GX
 * @param[in] req - данные аутентификации пользователя. 
 * @return Результат завершения операции login
 */


static int acle_login64(struct acle_data *dev,  struct acle_login_request_x64 *req)
{
	int i, res;
	uint32_t cmd_code;
	uint32_t userno, user_flags;
	for (i = 0; i < 8; ++i) {
		iowritel(dev, REG_ARG_0 + 4*i, *(uint32_t*)&req->hash[4*i]);
	}
	cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_NONE_LOGIN,
		8, 3, SELECT_BUF0);
	if (exec_cmd(dev, cmd_code, 0))
		return -EIO;
	res = ioreadl(dev, REG_ARG_0);
	acle_debug_print("acle_login64 res = %d\n", res);
	if (res == 0)
		return -EPERM;
	userno = res;
        acle_debug_print("acle_login64 userno=%d", userno);
	user_flags = ioreadl(dev, REG_ARG_1);
        acle_debug_print("acle_login64 userflasg=%x", user_flags);
	ioreadl(dev, REG_ARG_2);

        if (put_user(userno, req->userno) || put_user(user_flags, req->user_flags)) {
		acle_debug_print("acle_login failed to write  userno or user flags to user memory");
		return -EFAULT;
	}
	return 0;
}



/**
 * @brief  Логин в СДЗ GX
 * Логин в плату СДЗ. После успешного логина база данных открывается 
 * в соответствии с правами пользователя.
 * @param[in] dev - параметры устройства GX
 * @param[in] req - данные аутентификации пользователя. 
 * @return Результат завершения операции login
 */

static int login(struct acle_data *dev, struct acle_login_request *req)
{
	int i, res;
	uint32_t cmd_code;
	uint32_t userno, user_flags;
    for (i = 0; i < 8; ++i)
    {
		iowritel(dev, REG_ARG_0 + 4*i, *(uint32_t*)&req->hash[4*i]);
    }
	cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_NONE_LOGIN,
		8, 3, SELECT_BUF0);
	if (exec_cmd(dev, cmd_code, 0))
		return -EIO;
	res = ioreadl(dev, REG_ARG_0);
	acle_debug_print("login res = %d\n", res);
	if (res == 0)
		return -EPERM;
	userno = res;
        acle_debug_print("login userno=%d", userno);
	user_flags = ioreadl(dev, REG_ARG_1);
        acle_debug_print("login userflasg=%x", user_flags);
	ioreadl(dev, REG_ARG_2);

	if (put_user(userno, (uint32_t *)compat_ptr(req->userno)) || put_user(user_flags, (uint32_t *) compat_ptr(req->user_flags))) {
		acle_debug_print("login failed to write  userno or user flags to user memory");
		return -EFAULT;
	}
	return 0;
}

/**
 * @brief  Логоут  из СДЗ платы.
 * Логин в плату СДЗ. После выполнения операции происходит
 * закрытие базы данных.
 * @param[in] dev - параметры устройства GX
 * @return Результат завершения операции logout
 */
static int logout(struct acle_data *dev)
{
	uint32_t cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_NONE_LOGOUT,
		0, 1, SELECT_BUF0);
	if (exec_cmd(dev, cmd_code, 0))
		return -EIO;
#ifdef ACCORDLE_CFG_IGNORE_LOGOUT_ERROR
	ioreadl(dev, REG_ARG_0);
	return 0;
#else // ACCORDLE_CFG_IGNORE_LOGOUT_ERROR
	return ioreadl(dev, REG_ARG_0);
#endif // ACCORDLE_CFG_IGNORE_LOGOUT_ERROR
}

/**
 * @brief  Получение состояние СДЗ 
 * Получаем состояние пользователя, о контроллере СДЗ.
 * @param[in] dev - параметры устройства GX
 * @param[out] state - сотояние платы 
 * @return Результат завершения операции получения состояния
 */

static int get_state(struct acle_data *dev, struct acle_state *state)
{
    uint32_t status   = 0;
    uint32_t ver      = 0;
    uint8_t  ver_hi   = 0;
    uint8_t  ver_lo   = 0;
    uint16_t sw_flags = 0;
    uint32_t cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_NONE_INFO,
        0, 15, SELECT_BUF0);
    if (exec_cmd(dev, cmd_code, 0))
        return -EIO;

    state->flash_id  =  (uint32_t)ioreadl(dev, REG_ARG_7);

    ver =  (uint32_t)ioreadl(dev, REG_ARG_6);
    ver_hi = ver >> 24;
    ver -= (ver_hi << 24);
    ver_lo = ver >> 16;
    ver -= (ver_lo << 16);
    sw_flags = ver;
    state->fw_ver_hi = ver_hi;
    state->fw_ver_lo = ver_lo;
    state->sw_flags = sw_flags;
    state->memtabel.signature[0] = 0xFF;

    state->logged_user_flags = (uint8_t)ioreadl(dev, REG_ARG_5);
    state->logged_userno = (uint8_t)ioreadl(dev, REG_ARG_4);
    state->nusers = (uint16_t)ioreadl(dev, REG_ARG_3);
    state->hw_flags = 0;

    status = (uint32_t)ioreadl(dev, REG_ARG_2);
    if ((status & 1) == 0)
        state->hw_flags |= LE_SERVICE_MODE;
    if ((status & 512) == 0)
        state->hw_flags |= LE_ACCORD_MODE;
    state->serial = ioreadl(dev, REG_ARG_1);
    state->mode = (uint8_t)ioreadl(dev, REG_ARG_0);
    state->watchdog_timer = (uint32_t)ioreadl(dev, REG_ARG_10); // gx 0x20
    state->board_uid = (uint32_t)ioreadl(dev, REG_ARG_8);
    state->flash_id_4 = (uint32_t)ioreadl(dev, REG_ARG_9);
    // возвращается как объём sd-карты минус RESERVED_SDC_SECTORS=(1024^3)/512
    state->sdc_first_sector = (uint32_t)ioreadl(dev, REG_ARG_12); // gx 0x21-3x


    if ((ver_hi < MEMTABLE_HW_VERSION_HI) 
            || ((ver_hi == MEMTABLE_HW_VERSION_HI)&& (ver_lo < MEMTABLE_HW_VERSION_LO)))
    {
        return 0;
    };
    cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_NONE_NO_OPERATION,
            0, 0, SELECT_BUF1);
    if (exec_cmd(dev, cmd_code, 0))
        return -EIO;
    memcpy_fromio(&(state->memtabel), dev->data_mem, sizeof(memtable_t));
    return 0;
}

/**
 * @brief  Устанавливаем делитель ДСЧ 
 * Устанавливаем делитель аппаратного ДСЧ
 * @param[in] dev - параметры устройства GX
 * @param[in] divider -  делитель
 * @return Результат завершения операции установления делителя ДСЧ.
 */


static int init_rng(struct acle_data *dev, int divider)
{
	uint32_t cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_SET_TIMER,
		1, 0, SELECT_BUF0);
	iowritel(dev, REG_ARG_0, divider);
	if (exec_cmd(dev, cmd_code, 0))
		return -EIO;
	return 0;
}

/**
 * @brief Переключаемся между ДСЧ и чисто программным ГСЧ
 * Переключаемся между ДСЧ и программным ГСЧ
 * @param[in] dev               - параметры устройства GX
 * @param[in] use_software_mode - использовать ли программный ГСЧ
 * @return Результат завершения операции переключения режима ДСЧ.
 */
static int set_rng_mode(struct acle_data *dev,
                        int              divider,
                        int              use_sofware_mode)
{
	uint32_t cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_SET_TIMER,
		1, 0, SELECT_BUF0);
	uint32_t flag = RNG_DIVIDER;
	if (use_sofware_mode)
		{
			flag += 0x80000000; // старший бит
		}
#ifdef _ACLE_DEBUG_
	if (use_sofware_mode)
		{
			acle_debug_print("set_rng_mode: requested SOFTWARE\n");
		}
	else
		{
			acle_debug_print("set_rng_mode: requested HARDWARE\n");
		}
#endif // _ACLE_DEBUG_
	iowritel(dev, REG_ARG_0, flag);
	if (exec_cmd(dev, cmd_code, 0))
		return -EIO;
	return 0;
}

/**
 * @brief  Получения случайных чисел из ДСЧ контроллера. 
 * Получаем случайную последовательность их контроллера СДЗ.
 * @param[in] dev - параметры устройства GX
 * @param[in] count -  количество случайных байт, необходимых для считывания.
 * @param[out] buf - буфер  с случайными байтами.
 * @return Результат завершения операции получения ДСЧ.
 */

static int acle_get_random_bytes(struct acle_data *dev, size_t count, void __user *buf)
{
	size_t i = 0;
	/* количество получаемых данных должно быть кратным 4 */
	size_t _count = count + (4 - count % 4);

	while (i < _count) {
		size_t j;
		size_t chunk = min(sizeof(uint32_t) * 15, _count - i);
		uint32_t cmd_code = make_cmd_code(TARGET_NULL, LE_COMMAND_CODE_GET_RANDOM,
			0, chunk / sizeof(uint32_t), SELECT_BUF0);
		if (exec_cmd(dev, cmd_code, 0))
			return -EIO;

		for (j = 0; j < chunk / 4; ++j) {
			uint32_t tmp = ioreadl(dev, REG_ARG_0 + j * 4);
			size_t off = i + j * 4;
			if (count - off < 4) {
				if (copy_to_user(buf + off, &tmp, count - off)) {
					return -EFAULT;
        }
				return 0;
			}
			if (copy_to_user(buf + off, &tmp, 4)) {
				return -EFAULT;
      }
		}

		i += chunk;
	}
	return 0;
}

/**
 * @brief  Запись данных в BUF0  
 * Записываем последовательность байт в BUF0 платы СДЗ.
 * @param[in] dev - параметры устройства GX
 * @param[in] count -  количество случайных байт для записи. 
 * @param[in] buf - буфер  с данными для записи.
 * @return Результат завершения операции записи в BUF0.
 */


static int write_buf0(struct acle_data *dev, size_t count, const void *buf)
{
	/* выбрали буфер 0 и разрешили доступ к нему с PCI */
	uint32_t cmd_code = make_cmd_code(TARGET_NULL,
		LE_COMMAND_CODE_NONE_NO_OPERATION,
		0, 0, PCI_WRITE_EN | SELECT_BUF0);
	if (exec_cmd(dev, cmd_code, 0))
		return -EIO;
	/* записали */
	memcpy_toio(dev->data_mem, buf, count);
	return 0;
}

/**
 * @brief  Чтение  данных из  BUF0  
 * Считывание данных из BUF0 платы СДЗ.
 * @param[in] dev - параметры устройства GX
 * @param[in] count -  количество случайных байт для чтения. 
 * @param[out] buf - буфер для записи считанных данных из BUF0.
 * @return Результат завершения операции чтения из  BUF0.
 */
static int read_buf0(struct acle_data *dev, size_t count, void *buf)
{
	/* выбрали буфер 0 и разрешили доступ к нему с PCI */
	uint32_t cmd_code = make_cmd_code(TARGET_NULL,
		LE_COMMAND_CODE_NONE_NO_OPERATION,
		0, 0, SELECT_BUF0);
	if (exec_cmd(dev, cmd_code, 0))
		return -EIO;
	/* прочитали */
	memcpy_fromio(buf, dev->data_mem, count);
	return 0;
}
static int is_get_tm_id(struct acle_data *dev, void  *buf)
{
    uint32_t res = 0;
    uint32_t cmd_code = make_cmd_code(TARGET_NULL,
            LE_COMMAND_CODE_TM_ID,
            0, 3, SELECT_BUF1);

    if (exec_cmd(dev, cmd_code, 0))
        return -EIO;
    res = ioreadl(dev, REG_ARG_0);
    if (res == 0)
    {
        uint32_t idChunk = ioreadl(dev, REG_ARG_1); 
        memcpy(buf, &idChunk, sizeof(idChunk));
        idChunk = ioreadl(dev, REG_ARG_2); 
        memcpy(buf+sizeof(idChunk), &idChunk, sizeof(idChunk));

    }
    return res;
}

static int tm_read_page(struct acle_data *dev, uint8_t page_no ,void __user *buf)
{
    uint32_t cmd_code = make_cmd_code(TARGET_NULL,
            LE_COMMAND_CODE_TM_RD_PAGE,
            1, 9, SELECT_BUF1);
    uint32_t res = 0;
	iowritel(dev, REG_ARG_0, (uint32_t)page_no);


    if (exec_cmd(dev, cmd_code, 0))
        return -EIO;
    res = ioreadl(dev, REG_ARG_0);
    if ( res == 0)
    {
        uint32_t RL = REG_ARG_1;
        uint32_t offset = 0;
        uint8_t i = 0;
        for (i=0; i<8; ++i)
        {
            uint32_t page_chunk = ioreadl(dev, RL);
            if (copy_to_user(buf + offset, &page_chunk, sizeof(page_chunk)) != 0)
              break;
            offset += sizeof(page_chunk);
            RL += 4;
        };
    
    }
    return res;
}

static int tm_write_page(struct acle_data *dev, uint8_t page_no ,void __user *buf)
{
    uint8_t RL = REG_ARG_1;
    uint8_t i = 0;
    uint32_t offset = 0;


    uint32_t cmd_code = make_cmd_code(TARGET_NULL,
            LE_COMMAND_CODE_TM_WR_PAGE,
            9, 1, SELECT_BUF1);
    iowritel(dev, REG_ARG_0, (uint32_t) page_no);
    
    for (i=0; i<8; i++)
    {
        uint32_t page_chunk;
		if (copy_from_user(&page_chunk, buf + offset, sizeof(page_chunk)) != 0)
            return -EIO;
        iowritel(dev, RL, page_chunk);
        offset += sizeof(page_chunk);
        RL += 4;
    }
    if (exec_cmd(dev, cmd_code, 0))
        return -EIO;
    return 0;
}
static int is_tm_present(struct acle_data *dev)
{
    uint8_t res = 0;
    uint32_t cmd_code = make_cmd_code(TARGET_NULL,
            LE_COMMAND_CODE_TM_PRESENT,
            0, 1, SELECT_BUF1);
    if (exec_cmd(dev, cmd_code, 0))
    {
        return -EIO;
    }
    res =  (uint8_t)ioreadl(dev, REG_ARG_0);
    if ( (uint8_t)(res) != 0xFF)
    {
        return -EIO;
    }
    return 0;
}

static int disable_watchdog_timer(struct acle_data *dev) {
    uint32_t cmd_code = make_cmd_code(TARGET_NULL,
            LE_COMMAND_CODE_WD_RESET, 
            0, 0, SELECT_BUF0);
    if (exec_cmd(dev, cmd_code, 0))
    {
        return -EIO;
    }
    return 0;
	//return ioreadl(dev, REG_ARG_0);
}

static int get_rtc_timer(struct acle_data *dev, struct acle_rtc_time *rtc)
{
    int      is_invalid    = 0;
    uint32_t regs_0_1_2[3] = {0};
    uint32_t cmd_code      = make_cmd_code(TARGET_NULL,
                                           LE_COMMAND_CODE_GET_RTC,
                                           0,
                                           3,
                                           SELECT_BUF0);
    struct acle_rtc_time result;
    struct BCDDateTime   bcd_datetime;

    memset(&result,       0x00, sizeof(result));
    memset(&bcd_datetime, 0x00, sizeof(bcd_datetime));

    if (exec_cmd(dev, cmd_code, 0))
        {
            return -EIO;
        }

    regs_0_1_2[0] = ioreadl(dev, REG_ARG_0);

    if (!regs_0_1_2[0])
        {
            regs_0_1_2[1] = ioreadl(dev, REG_ARG_1);
            regs_0_1_2[2] = ioreadl(dev, REG_ARG_2);
            memcpy(&bcd_datetime,
                   &regs_0_1_2[1],
                   min(sizeof(bcd_datetime), 2*sizeof(regs_0_1_2[1])));

            is_invalid = bcd_to_acle_rtc(&bcd_datetime, &result);
        }

    acle_debug_print("\
get_rtc_time:\n\
    regs_0_1_2 = [%02x %02x %02x %02x\n\
                  %02x %02x %02x %02x\n\
                  %02x %02x %02x %02x]\n\
    bcd = {day=   %2d(0x%02x), month=%2d(0x%02x),year=%2d(0x%02x),\n\
           DOW=   %2d(0x%02x),\n\
           second=%2d(0x%02x),minute=%2d(0x%02x),hour=%2d(0x%02x)}\n\
    acle_rtc = {year=%5d(%04x), month=%2d(0x%02x),   day=%2d(0x%02x),\n\
                   hour=%2d(%02x),  minute=%2d(0x%02x),second=%2d(0x%02x)}\n\
    is_valid = %s\n",
        ((uint8_t*)regs_0_1_2)[0],  ((uint8_t*)regs_0_1_2)[1],
        ((uint8_t*)regs_0_1_2)[2],  ((uint8_t*)regs_0_1_2)[3],
        ((uint8_t*)regs_0_1_2)[4],  ((uint8_t*)regs_0_1_2)[5],
        ((uint8_t*)regs_0_1_2)[6],  ((uint8_t*)regs_0_1_2)[7],
        ((uint8_t*)regs_0_1_2)[8],  ((uint8_t*)regs_0_1_2)[9],
        ((uint8_t*)regs_0_1_2)[10], ((uint8_t*)regs_0_1_2)[11],
        bcd_datetime.date.bcd_day,    bcd_datetime.date.bcd_day,
        bcd_datetime.date.bcd_month,  bcd_datetime.date.bcd_month,
        bcd_datetime.date.bcd_year,   bcd_datetime.date.bcd_year,
        bcd_datetime.bcd_day_of_week, bcd_datetime.bcd_day_of_week,
        bcd_datetime.time.bcd_second, bcd_datetime.time.bcd_second,
        bcd_datetime.time.bcd_minute, bcd_datetime.time.bcd_minute,
        bcd_datetime.time.bcd_hour,   bcd_datetime.time.bcd_hour,
        result.year,   result.year,
        result.month,  result.month,
        result.day,    result.day,
        result.hour,   result.hour,
        result.minute, result.minute,
        result.second, result.second,
        is_invalid ? "FALSE" : "TRUE"
    );

    if (regs_0_1_2[0])
        {
            // STATUS_ADAPTER_HADRWARE_ERROR
            return -EREMOTEIO;
        }
    else if (is_invalid)
        {
            return -EBADMSG;
        }

    *rtc = result;
    return 0;
}

static int set_rtc_timer(struct acle_data *dev, struct acle_rtc_time *rtc)
{
    uint32_t status      = 0;
    int      is_invalid  = 0;
    uint32_t regs_0_1[2] = {0};
    uint32_t cmd_code    = make_cmd_code(TARGET_NULL,
                                         LE_COMMAND_CODE_SET_RTC,
                                         2,
                                         1,
                                         SELECT_BUF0);
    struct BCDDateTime bcd_datetime;

    memset(&bcd_datetime, 0x00, sizeof(bcd_datetime));

    is_invalid = acle_to_bcd_rtc(rtc, &bcd_datetime);

    if (!is_invalid)
        {
            memcpy(regs_0_1,
                   &bcd_datetime,
                   min(sizeof(regs_0_1), sizeof(bcd_datetime)));
            iowritel(dev, REG_ARG_0, regs_0_1[0]);
            iowritel(dev, REG_ARG_1, regs_0_1[1]);

            if (exec_cmd(dev, cmd_code, 0))
                {
                    return -EIO;
                }
            status = ioreadl(dev, REG_ARG_0);
        }

    acle_debug_print("\
set_rtc_time:\n\
    regs_0_1 = [%02x %02x %02x %02x\n\
                %02x %02x %02x %02x]\n\
    bcd = {day=   %2d(0x%02x), month=%2d(0x%02x),year=%2d(0x%02x),\n\
           DOW=   %2d(0x%02x),\n\
           second=%2d(0x%02x),minute=%2d(0x%02x),hour=%2d(0x%02x)}\n\
    acle_rtc = {year=%5d(%04x), month=%2d(0x%02x),   day=%2d(0x%02x),\n\
                   hour=%2d(%02x),  minute=%2d(0x%02x),second=%2d(0x%02x)}\n\
    is_valid = %s\n",
        ((uint8_t*)regs_0_1)[0],  ((uint8_t*)regs_0_1)[1],
        ((uint8_t*)regs_0_1)[2],  ((uint8_t*)regs_0_1)[3],
        ((uint8_t*)regs_0_1)[4],  ((uint8_t*)regs_0_1)[5],
        ((uint8_t*)regs_0_1)[6],  ((uint8_t*)regs_0_1)[7],
        bcd_datetime.date.bcd_day,    bcd_datetime.date.bcd_day,
        bcd_datetime.date.bcd_month,  bcd_datetime.date.bcd_month,
        bcd_datetime.date.bcd_year,   bcd_datetime.date.bcd_year,
        bcd_datetime.bcd_day_of_week, bcd_datetime.bcd_day_of_week,
        bcd_datetime.time.bcd_second, bcd_datetime.time.bcd_second,
        bcd_datetime.time.bcd_minute, bcd_datetime.time.bcd_minute,
        bcd_datetime.time.bcd_hour,   bcd_datetime.time.bcd_hour,
        rtc->year,   rtc->year,
        rtc->month,  rtc->month,
        rtc->day,    rtc->day,
        rtc->hour,   rtc->hour,
        rtc->minute, rtc->minute,
        rtc->second, rtc->second,
        is_invalid ? "FALSE" : "TRUE"
    );

    if (status)
        {
            // STATUS_ADAPTER_HADRWARE_ERROR
            return -EREMOTEIO;
        }
    else if (is_invalid)
        {
            return -EBADMSG;
        }

    return 0;
}

/**
 * @brief Чтение блока данных с sd-карты.
 * Важно: не допускается чтение/запись на границе сектора, то есть
 * `byte_offset_in_sector + buffer_byte_size <= ACLE_MAX_SD_BLOCK_SIZE`.
 * @param [in]  dev                   - устройство, аппартаная плата СДЗ
 * @param [in]  sector_number         - номер сектора
 * @param [in]  byte_offset_in_sector - смещение внутри сектора, кратное размеру int
 * @param [in]  buffer_byte_size      - количество считываемых данных, кратное размеру int
 * @param [out] buffer                - куда прочитать данные
 */
static int read_sd(struct acle_data *dev,
                   unsigned         sector_number,
                   unsigned         byte_offset_in_sector,
                   unsigned         buffer_byte_size,
                   void             *buffer)
{
    int      status   = 0;
    uint32_t cmd_code = make_cmd_code(TARGET_NULL,
                                      LE_COMMAND_CODE_SD_READ,
                                      3,
                                      1,
                                      SELECT_BUF0);

    iowritel(dev, REG_ARG_0, sector_number);
    iowritel(dev, REG_ARG_1, byte_offset_in_sector);
    iowritel(dev, REG_ARG_2, buffer_byte_size);

    acle_debug_print("read_sd:\n");

    if (0 != byte_offset_in_sector % sizeof(int) || 0 != buffer_byte_size % sizeof(int) || ACLE_MAX_SD_BLOCK_SIZE < buffer_byte_size || ACLE_MAX_SD_BLOCK_SIZE < byte_offset_in_sector + buffer_byte_size)
        {
            acle_debug_print("  error: size(=%u) and offset(=%u) require %%%lu == 0, size < %u and size+offset<%u\n", buffer_byte_size, byte_offset_in_sector, sizeof(long), ACLE_MAX_SD_BLOCK_SIZE, ACLE_MAX_SD_BLOCK_SIZE);
            status = -EBADMSG;
        }
    else if (exec_cmd(dev, cmd_code, 0))
        {
            acle_debug_print("  error: main command failed\n");
            status = -EIO;
        }
    else
        {
            status = ioreadl(dev, REG_ARG_0);
            if (status)
                {
                    acle_debug_print("  error: control reg-read failed, status %d\n", status);
                    status = -EREMOTEIO;
                }
            else
                {
                    /* PCI_WRITE_EN or SELECT_BUF1 - чтобы Альтера имела доступ к буферу 0 */
                    cmd_code = make_cmd_code(TARGET_NULL,
                                             LE_COMMAND_CODE_NONE_NO_OPERATION,
                                             0,
                                             0,
                                             SELECT_BUF1);
                    if (exec_cmd(dev, cmd_code, 0))
                        {
                            acle_debug_print("  error: post-control command failed\n");
                            status = -EIO;
                        }
                        memcpy_fromio(buffer, dev->data_mem, buffer_byte_size);
                }
        }

#ifdef _ACLE_DEBUG_
    acle_debug_print("\
  attempt to read %u(0x%x) bytes, sector %u(0x%x), offset %u(0x%x)\n\
  %s (0x%x)\n",
        buffer_byte_size,      buffer_byte_size,
        sector_number,         sector_number,
        byte_offset_in_sector, byte_offset_in_sector,
        status ? "ERROR" : "SUCCESS", status);
    if (!status)
    {
        unsigned i      = 0;
	uint8_t  *bytes = (uint8_t*)buffer;
        char     fmt[]  = "%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n";
	for (i = 0; i < buffer_byte_size; i += 16)
	    {
                if (16 > buffer_byte_size - i)
                    {
                        fmt[5*(buffer_byte_size - i) + (buffer_byte_size - i)/4] = '\0';
                    }
                printk(fmt, bytes[i],    bytes[i+1],  bytes[i+2],  bytes[i+3],
                            bytes[i+4],  bytes[i+5],  bytes[i+6],  bytes[i+7],
                            bytes[i+8],  bytes[i+9],  bytes[i+10], bytes[i+11],
                            bytes[i+12], bytes[i+13], bytes[i+14], bytes[i+15]);
            }
    }
#endif // _ACLE_DEBUG_

    return status;
}

/**
 * @brief Запись блока данных на sd-карту.
 * Важно: не допускается чтение/запись на границе сектора, то есть
 * `byte_offset_in_sector + buffer_byte_size <= ACLE_MAX_SD_BLOCK_SIZE`.
 * @param [in] dev                   - устройство, аппартаная плата СДЗ
 * @param [in] sector_number         - номер сектора
 * @param [in] byte_offset_in_sector - смещение внутри сектора, кратное размеру int
 * @param [in] buffer_byte_size      - количество считываемых данных, кратное размеру int
 * @param [in] buffer                - откуда записать данные
 */
static int write_sd(struct acle_data *dev,
                    unsigned         sector_number,
                    unsigned         byte_offset_in_sector,
                    unsigned         buffer_byte_size,
                    const void       *buffer)
{
    int      status = 0;
    /* выбрали буфер 0 и разрешили доступ к нему с PCI */
    uint32_t cmd_code = make_cmd_code(TARGET_NULL,
                                      LE_COMMAND_CODE_NONE_NO_OPERATION,
                                      0,
                                      0,
                                      PCI_WRITE_EN | SELECT_BUF0);

    acle_debug_print("write_sd:\n");
    if (0 != byte_offset_in_sector % sizeof(int) || 0 != buffer_byte_size % sizeof(int) || ACLE_MAX_SD_BLOCK_SIZE < buffer_byte_size || ACLE_MAX_SD_BLOCK_SIZE < byte_offset_in_sector + buffer_byte_size)
        {
            acle_debug_print("   error: size(=%u) and offset(=%u) require %%%lu == 0, size < %u and size+offset<%u\n", buffer_byte_size, byte_offset_in_sector, sizeof(long), ACLE_MAX_SD_BLOCK_SIZE, ACLE_MAX_SD_BLOCK_SIZE);
            status = -EBADMSG;
        }
    else if (exec_cmd(dev, cmd_code, 0))
        {
            acle_debug_print("  error: pre-control command failed\n");
            status = -EIO;
        }
    else
        {
            iowritel(dev, REG_ARG_0, sector_number);
            iowritel(dev, REG_ARG_1, byte_offset_in_sector);
            iowritel(dev, REG_ARG_2, buffer_byte_size);
            memcpy_toio(dev->data_mem, buffer, buffer_byte_size);

            cmd_code = make_cmd_code(TARGET_NULL,
                                     LE_COMMAND_CODE_SD_WRITE,
                                     3,
                                     1,
                                     PCI_WRITE_EN | SELECT_BUF1);
            if (exec_cmd(dev, cmd_code, 0))
                {
                    acle_debug_print("  error: main command failed\n");
                    status = -EIO;
                }
            else
                {
                    status = ioreadl(dev, REG_ARG_0);
                    if (status)
                        {
                            acle_debug_print("  error: control reg-read failed\n");
                            status = -EREMOTEIO;
                        }
                }
        }

#ifdef _ACLE_DEBUG_
    acle_debug_print("\
  attempt to write %u(0x%x) bytes, sector %u(0x%x), offset %u(0x%x)\n\
  %s (0x%x)\n",
        buffer_byte_size,      buffer_byte_size,
        sector_number,         sector_number,
        byte_offset_in_sector, byte_offset_in_sector,
        status ? "ERROR" : "SUCCESS", status);
    if (!status)
    {
        unsigned i      = 0;
	uint8_t  *bytes = (uint8_t*)buffer;
        char     fmt[]  = "%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n";
	for (i = 0; i < buffer_byte_size; i += 16)
	    {
                if (16 > buffer_byte_size - i)
                    {
                        fmt[5*(buffer_byte_size - i) + (buffer_byte_size - i)/4] = '\0';
                    }
                printk(fmt, bytes[i],    bytes[i+1],  bytes[i+2],  bytes[i+3],
                            bytes[i+4],  bytes[i+5],  bytes[i+6],  bytes[i+7],
                            bytes[i+8],  bytes[i+9],  bytes[i+10], bytes[i+11],
                            bytes[i+12], bytes[i+13], bytes[i+14], bytes[i+15]);
            }
    }
#endif // _ACLE_DEBUG_

    return status;
}


/*
 * Set mode of working with SD cards. Mode has changed by last 3 bits:
 * 000 - nothing
 * 001 - show reader without card
 * 011 - show reader with card on READ mode
 * 111 - show reader with card on RW
 */
static int set_sdc_mode (struct acle_data *dev, unsigned long arg) {
    unsigned char byte = arg & 0x7;
    uint32_t cmd_code_0, cmd_code ;

    if (byte != 0 && byte != 1 && byte != 3 && byte != 7) {
        acle_debug_print ("   wrong argument: %lu\n", arg);
        return -EFAULT;
    }
    
#ifdef SET_SDC_0
    if (byte) {
        iowritel (dev, REG_ARG_0, 0);
        cmd_code_0 = make_cmd_code (TARGET_NULL, ACC_COMMAND_CODE_SET_SDC, 1, 0, SELECT_BUF1);
        if (exec_cmd (dev, cmd_code_0, 0)) {
            acle_debug_print ("   cannot execute command\n");
            return -EIO;
        }
    }
#endif // SET_SDC_0
    iowritel (dev, REG_ARG_0, byte);
    cmd_code = make_cmd_code (TARGET_NULL, ACC_COMMAND_CODE_SET_SDC, 1, 0, SELECT_BUF1);

    if (exec_cmd (dev, cmd_code, 0)) {
        acle_debug_print ("   cannot execute command\n");
        return -EIO;
    }
    return 0;
}

/**
 * @brief  Реализация системного вызова ioctl 
 * Системный вызов ioctl 
 * @param[in] file - представления файла. 
 * @param[in] cmd -  код команды.
 * @param[in,out] arg - список аргументов ioctl
 * @return Результат завершения операции ioctl
 */

static long acle_ioctl(struct file *file, unsigned int cmd, unsigned long _arg)
{
	int res;
    
    void __user   *arg = compat_ptr(_arg);
  acle_debug_print("compat_ioctl\n");
	switch (cmd) {
	case ACLE_CMD_WRITE: {
		struct acle_data_chunk page;
		if (copy_from_user(&page, (void __user *)arg, sizeof(page)) != 0)
			return -EFAULT;
		/* за один раз можно записать не более страницы и размер должен
		 * быть кратным 4 */
		if (page.count > ACLE_PAGE_SIZE || page.count % 4)
			return -EINVAL;
		/* смещение должно быть в пределах размера NAND и выровнено
		 * по границе 4 байт */
		if (page.offset >= acle_data.nand_size || page.offset % 4)
			return -EINVAL;
		if (copy_from_user(acle_data.page, compat_ptr(page.data),
			page.count) != 0)
			return -EFAULT;
		res = write_flash(&acle_data, page.offset, page.count, acle_data.page);
		break;
	}

	case ACLE_CMD_WRITE_TO_BUF0: {
		struct acle_data_chunk chunk;
		void *buf;

		if (copy_from_user(&chunk, (void __user *)arg, sizeof(chunk)) != 0)
			return -EFAULT;
		buf = kmalloc(chunk.count, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;
		if (copy_from_user(buf, compat_ptr(chunk.data), chunk.count) != 0) {
			memset(buf, 0, chunk.count);
			kfree(buf);
			return -EFAULT;
		}

		res = write_buf0(&acle_data, chunk.count, buf);
		memset(buf, 0, chunk.count);
		kfree(buf);
		break;
	}

	case ACLE_CMD_READ_FROM_BUF0: {
		/* размер структуры с параметрами И/А - 108 (определена в libamdz) */
		void *buf = kmalloc(ACLE_IADATA_SIZE, GFP_KERNEL);
		if (!buf) {
			printk(KERN_WARNING PREFIX "kmalloc failed\n");
			return -ENOMEM;
		}

		/* размер структуры с параметрами И/А - 108 (определена в libamdz) */
		res = read_buf0(&acle_data, ACLE_IADATA_SIZE, buf);
		if (res)
			return -EFAULT;

		if (copy_to_user((void *)arg, buf, ACLE_IADATA_SIZE))
			return -EFAULT;

		memset(buf, 0, 108);
		kfree(buf);
		break;
	}

	case ACLE_CMD_ERASE_SECTOR:
		res = erase_flash_sector(&acle_data, _arg * ACLE_SECTOR_SIZE);
		break;

	case ACLE_CMD_LOGIN: {
		struct acle_login_request req;
		if (copy_from_user(&req, (void __user *)arg, sizeof(req)) != 0)
			return -EFAULT;
		res = login(&acle_data, &req);
		break;
	}

	case ACLE_CMD_LOGOUT:
		res = logout(&acle_data);
		break;

	case ACLE_GET_BOARD_COUNT:
        res = put_user(atomic_read(&board_count), (int *)arg);
		break;

	case ACLE_CMD_GET_RANDOM_BYTES: {
		struct acle_rng_request req;
		if (copy_from_user(&req, (void __user *)arg, sizeof(req)) != 0)
			return -EFAULT;
		res = acle_get_random_bytes(&acle_data, req.count, (void __user *)req.buf);
		break;
	}

	case ACLE_CMD_GET_STATE: {
		struct acle_state state;
		res = get_state(&acle_data, &state);
		if (res)
			return res;
		if (copy_to_user((void *)arg, &state, sizeof(state)))
			return -EFAULT;
		break;
	}
  case ACLE_CMD_GET_STATE_KERNEL: {
    struct acle_state state;
    res = get_state(&acle_data, &state);

    if (res) return res;
    memcpy((void *)arg, &state, sizeof(state));
    break;
  }
    case ACLE_CMD_TM_PRESENT: {
        res = is_tm_present(&acle_data);
        if (res)
        {
            return -EFAULT;
        }
        break;
    }
    case ACLE_CMD_TM_GET_ID: {
        uint8_t tm[8];
        memset(tm, 0x00, 8);
        if (copy_to_user((void *)arg, tm, 8) != 0)
          return -EFAULT;
        res = is_get_tm_id(&acle_data, tm);
        if (res)
            return -EFAULT;
        if (copy_to_user((void *)arg, tm, 8))
            return -EFAULT;
        break;
    }
    case ACLE_CMD_TM_READ_PAGE: {
		struct acle_rng_request req;
		if (copy_from_user(&req, (void __user *)arg, sizeof(req)) != 0)
			return -EFAULT;
        res = tm_read_page(&acle_data, req.count, (void __user *) compat_ptr(req.buf));
        break;
    }
    case ACLE_CMD_TM_WRITE_PAGE: {
        struct acle_rng_request req;
		if (copy_from_user(&req, (void __user *)arg, sizeof(req)) != 0)
			return -EFAULT;
        res = tm_write_page(&acle_data, req.count, (void __user *) compat_ptr(req.buf));
        break;
    }
    case ACLE_DISABLE_WATCHDOG_TIMER: {
		res = disable_watchdog_timer(&acle_data);
        break;
    }
    case ACLE_CMD_GET_RTC: {
        struct acle_rtc_time rtc;
        memset(&rtc, 0, sizeof(rtc));

        res = get_rtc_timer(&acle_data, &rtc);
        if (copy_to_user((void*)arg, &rtc, sizeof(rtc))) {
            return -EFAULT;
        }
        break;
    }
    case ACLE_CMD_SET_RTC:
    {
        struct acle_rtc_time rtc;
        memset(&rtc, 0, sizeof(rtc));

        if (copy_from_user(&rtc, (void __user *)arg, sizeof(rtc)) != 0) {
            return -EFAULT;
        }
        res = set_rtc_timer(&acle_data, &rtc);
        break;
    }
    case ACLE_CMD_SD_READ:
    {
        struct acle_sd_chunk page;
        void *buf;
        if (copy_from_user(&page, (void __user *)arg, sizeof(page)) != 0)
            return -EFAULT;
        buf = kmalloc(page.size, GFP_KERNEL);
        if (!buf)
            return -ENOMEM;

        memset(buf, 0, page.size);
        res = read_sd(&acle_data, page.sector_number, page.offset, page.size, buf);
        if (res)
            {
                kfree(buf);
                return -EFAULT;
            }
        else if (copy_to_user(compat_ptr(page.data), buf, page.size))
            {
                kfree(buf);
                return -EFAULT;
            }
        else {
            kfree(buf);
        }
        break;
    }
    case ACLE_CMD_SD_WRITE:
    {
        struct acle_sd_chunk page;
        void *buf;
        if (copy_from_user(&page, (void __user *)arg, sizeof(page)) != 0)
            return -EFAULT;
        buf = kmalloc(page.size, GFP_KERNEL);
        if (!buf)
            return -ENOMEM;
        if (copy_from_user(buf, (void __user *)compat_ptr(page.data), page.size) != 0)
            return -EFAULT;

        res = write_sd(&acle_data, page.sector_number, page.offset, page.size, buf);
        memset(buf, 0, page.size);
        kfree(buf);
        break;
    }
    case ACLE_CMD_SET_RANDOM: {
        res = set_rng_mode(&acle_data, RNG_DIVIDER, _arg);
        break;
    }
    case ACLE_CMD_SET_SDC: {
        res = set_sdc_mode (&acle_data, _arg);
        break;
    }
	default:

	printk(KERN_INFO PREFIX "no tty");
		res = -ENOTTY;
	}
	return res;
}

static long acle_ioctl64(struct file *file, unsigned int cmd, unsigned long _arg) 
{
	int res;
	acle_debug_print("masked cmd=%x\n", IOCTL_CMD_MASK & cmd);

  switch (cmd & IOCTL_CMD_MASK) {
	case (ACLE_CMD_WRITE & IOCTL_CMD_MASK): {
		struct acle_data_chunk_x64 page;
		acle_debug_print("ACLE_CMD_WRITE\n");
		if (copy_from_user(&page, (void __user *)_arg, sizeof(page)) != 0)
			return -EFAULT;
		/* за один раз можно записать не более страницы и размер должен
		 * быть кратным 4 */
		if (page.count > ACLE_PAGE_SIZE || page.count % 4)
			return -EINVAL;
		/* смещение должно быть в пределах размера NAND и выровнено
		 * по границе 4 байт */
		if (page.offset >= acle_data.nand_size || page.offset % 4)
			return -EINVAL;
    // what if we want to run x64 apps from x32 system? 
		if (copy_from_user(acle_data.page, (void __user *)page.data, page.count) != 0)
			return -EFAULT;
		res = write_flash(&acle_data, page.offset, page.count, acle_data.page);
		break;
	}

	case (ACLE_CMD_WRITE_TO_BUF0 & IOCTL_CMD_MASK): {
		struct acle_data_chunk_x64 chunk;
		void *buf;
		acle_debug_print("ACLE_CMD_WRITE_TO_BUF0\n");

		if (copy_from_user(&chunk, (void __user *)_arg, sizeof(chunk)) != 0)
			return -EFAULT;
		buf = kmalloc(chunk.count, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;
		if (copy_from_user(buf, (void __user *)chunk.data, chunk.count) != 0) {
			memset(buf, 0, chunk.count);
			kfree(buf);
			return -EFAULT;
		}

		res = write_buf0(&acle_data, chunk.count, buf);
		memset(buf, 0, chunk.count);
		kfree(buf);
		break;
	}

	case (ACLE_CMD_READ_FROM_BUF0 & IOCTL_CMD_MASK): {
		/* размер структуры с параметрами И/А - 108 (определена в libamdz) */
		void *buf = kmalloc(ACLE_IADATA_SIZE, GFP_KERNEL);
		
		acle_debug_print("ACLE_CMD_READ_FROM_BUF0\n");
		if (!buf) {
			printk(KERN_WARNING PREFIX "kmalloc failed\n");
			return -ENOMEM;
		}

		/* размер структуры с параметрами И/А - 108 (определена в libamdz) */
		res = read_buf0(&acle_data, ACLE_IADATA_SIZE, buf);
		if (res)
			return -EFAULT;

		if (copy_to_user((void __user*)_arg, buf, ACLE_IADATA_SIZE))
			return -EFAULT;

		memset(buf, 0, 108);
		kfree(buf);
		break;
	}

	case (ACLE_CMD_ERASE_SECTOR & IOCTL_CMD_MASK): {
		acle_debug_print("ACLE_CMD_ERASE_SECTOR\n");
		res = erase_flash_sector(&acle_data, _arg * ACLE_SECTOR_SIZE);
		break;
  }

	case (ACLE_CMD_LOGIN & IOCTL_CMD_MASK): {
		//struct acle_login_request req;
		struct acle_login_request_x64 req;
		acle_debug_print("ACLE_CMD_LOGIN\n");
		if (copy_from_user(&req, (void __user *)_arg, sizeof(req)) != 0)
			return -EFAULT;
		res = acle_login64(&acle_data, &req);
		break;
	}

	case (ACLE_CMD_LOGOUT & IOCTL_CMD_MASK): {
    acle_debug_print("ACLE_CMD_LOGOUT\n");
		res = logout(&acle_data);
		break;
  }

	case (ACLE_GET_BOARD_COUNT & IOCTL_CMD_MASK): {
    acle_debug_print("ACLE_GET_BOARD_COUNT\n");
    res = put_user(atomic_read(&board_count), (int *)_arg);
		break;
  }

	case (ACLE_CMD_GET_RANDOM_BYTES & IOCTL_CMD_MASK): {
		struct acle_rng_request_x64 req;
    acle_debug_print("ACLE_CMD_GET_RANDOM_BYTES\n");
		if (copy_from_user(&req, (void __user *)_arg, sizeof(req) ) != 0)
			return -EFAULT;
		res = acle_get_random_bytes(&acle_data, req.count, req.buf);
		break;
	}

	case (ACLE_CMD_GET_STATE & IOCTL_CMD_MASK): {
		struct acle_state state;
    acle_debug_print("ACLE_CMD_GET_STATE\n");
		res = get_state(&acle_data, &state);
		if (res) return res;
   
    if (virt_addr_valid((void *)_arg)) {
      memcpy((void *)_arg, &state, sizeof(state));
    } else {
      acle_debug_print("This is not kernel space address\n");
      if (copy_to_user((void __user *)_arg, &state, sizeof(state)))
			  return -EFAULT; 
    }
		
		break;
	}

  case (ACLE_CMD_GET_STATE_KERNEL & IOCTL_CMD_MASK): {
    struct acle_state state;
    res = get_state(&acle_data, &state);

    if (res) return res;
    if (virt_addr_valid((void *)_arg)) {
      memcpy((void *)_arg, &state, sizeof(state));
    } else {
      acle_debug_print("This is not kernel space address\n");
      if (copy_to_user((void __user *)_arg, &state, sizeof(state)))
			  return -EFAULT; 
    }
    break;
  }

  case (ACLE_CMD_TM_PRESENT & IOCTL_CMD_MASK): {
    acle_debug_print("ACLE_CMD_TM_PRESENT\n");
    res = is_tm_present(&acle_data);
    if (res) {
      return -EFAULT;
    }
    break;
  }
  case (ACLE_CMD_TM_GET_ID & IOCTL_CMD_MASK): {
    uint8_t tm[8];
    acle_debug_print("ACLE_CMD_TM_GET_ID\n");
    memset(tm, 0x00, 8);
    if (copy_to_user((void __user*)_arg, tm, 8) != 0)
      return -EFAULT;
    res = is_get_tm_id(&acle_data, tm);
    if (res)
      return -EFAULT;
    if (copy_to_user((void __user*)_arg, tm, 8))
      return -EFAULT;
    break;
  }
  case (ACLE_CMD_TM_READ_PAGE & IOCTL_CMD_MASK): {
    struct acle_rng_request_x64 req;
    acle_debug_print("ACLE_CMD_TM_READ_PAGE\n");
    if (copy_from_user(&req, (void __user *)_arg, sizeof(req)) != 0)
      return -EFAULT;
    res = tm_read_page(&acle_data, req.count, (void __user *) req.buf);
    break;
  }
  case (ACLE_CMD_TM_WRITE_PAGE & IOCTL_CMD_MASK): {
    struct acle_rng_request_x64 req;
    acle_debug_print("ACLE_CMD_TM_WRITE_PAGE\n");
		if (copy_from_user(&req, (void __user *)_arg, sizeof(req)) != 0)
		  return -EFAULT;
    res = tm_write_page(&acle_data, req.count, (void __user *) req.buf);
    break;
  }
  case (ACLE_DISABLE_WATCHDOG_TIMER & IOCTL_CMD_MASK): {
    acle_debug_print("ACLE_DISABLE_WATCHDOG_TIMER\n");
		res = disable_watchdog_timer(&acle_data);
    break;
  }
  case (ACLE_CMD_GET_RTC & IOCTL_CMD_MASK): {
    struct acle_rtc_time rtc;
    acle_debug_print("ACLE_CMD_GET_RTC\n");
    memset(&rtc, 0, sizeof(rtc));

    res = get_rtc_timer(&acle_data, &rtc);
    if (copy_to_user((void __user*)_arg, &rtc, sizeof(rtc)) != 0) {
      return -EFAULT;
    }
    break;
  }
  case (ACLE_CMD_SET_RTC & IOCTL_CMD_MASK): {
    struct acle_rtc_time rtc;
    acle_debug_print("ACLE_CMD_SET_RTC\n");
    memset(&rtc, 0, sizeof(rtc));

    if (copy_from_user(&rtc, (void __user *)_arg, sizeof(rtc)) != 0) {
      return -EFAULT;
    }
    res = set_rtc_timer(&acle_data, &rtc);
    break;
  }
  case (ACLE_CMD_SD_READ & IOCTL_CMD_MASK): {
    struct acle_sd_chunk_x64 page;
    void *buf;
    acle_debug_print("ACLE_CMD_SD_READ\n");
    if (copy_from_user(&page, (void __user *)_arg, sizeof(page)) != 0)
      return -EFAULT;
    buf = kmalloc(page.size, GFP_KERNEL);
    if (!buf) {
      return -ENOMEM;
    }
    memset(buf, 0, page.size);
    res = read_sd(&acle_data, page.sector_number, page.offset, page.size, buf);
    if (res) {
      kfree(buf);
      return -EFAULT;
    } else if (copy_to_user((void __user *)page.data, buf, page.size)) {
      kfree(buf);
      return -EFAULT;
    } else {
      kfree(buf);
    }
    break;
  }
  case (ACLE_CMD_SD_WRITE & IOCTL_CMD_MASK): {
    struct acle_sd_chunk_x64 page;
    void *buf;
    acle_debug_print("ACLE_CMD_SD_WRITE\n");
    if (copy_from_user(&page, (void __user *)_arg, sizeof(page)) != 0)
      return -EFAULT;
    buf = kmalloc(page.size, GFP_KERNEL);
    if (!buf)
      return -ENOMEM;
    if (copy_from_user(buf, (void __user *)page.data, page.size) != 0)
      return -EFAULT;

    res = write_sd(&acle_data, page.sector_number, page.offset, page.size, buf);
    memset(buf, 0, page.size);
    kfree(buf);
    break;
  }
  case (ACLE_CMD_SET_RANDOM & IOCTL_CMD_MASK): {
    acle_debug_print("ACLE_CMD_SET_RANDOM\n");
    res = set_rng_mode(&acle_data, RNG_DIVIDER, _arg);
    break;
  }
  case (ACLE_CMD_SET_SDC & IOCTL_CMD_MASK): {
    acle_debug_print("ACLE_CMD_SET_SDC\n");
    res = set_sdc_mode (&acle_data, _arg);
    break;
  }
	default: {
    printk(KERN_INFO PREFIX "no tty");
		res = -ENOTTY;
  }
	}

	return res;
}
/**
 * Операции  работы с устройством accord_le
 */
static struct file_operations acle_fops = {
	.owner = THIS_MODULE, ///< Указатель на данный модуль
	.llseek = acle_llseek, ///< Реализация вызова llseek
	.read = acle_read, ///< Реализация системного вызова read
	.unlocked_ioctl = acle_ioctl64, ///< Реализация системного вызова ioctl
    .compat_ioctl = acle_ioctl,
};

/**
 * Операция описания символьного устройства
 */
static struct miscdevice acle_dev = {
	.minor = MISC_DYNAMIC_MINOR, ///< minor номер устройства
	.name = DRIVER_NAME, ///< имя драйвера 
	.fops = &acle_fops, ///< список файловых операций.
};

/**
 * Список VID/DID PCI устройств плат СДЗ
 */
static struct pci_device_id acle_pci_id_table[] = {
	{PCI_DEVICE(ACCORDLE_VID, ACCORDLE_DID)},
	{PCI_DEVICE(ACCORDLE_VID, ACCORDGX_DID)},
	{PCI_DEVICE(ACCORDLE_VID, ACCORDGXM_DID)},
	{PCI_DEVICE(ACCORDLE_VID, ACCORDGXMH_DID)},
	{PCI_DEVICE(ACCORDLE_VID, ACCORDGX2AE_DID)},
	{PCI_DEVICE(ACCORDLE_VID, ACCORDGX2MH_DID)},
	{PCI_DEVICE(ACCORDLE_VID, ACCORDGX5AE_DID)},
	{PCI_DEVICE(ACCORDLE_VID, RESERVE_DID)},
	{PCI_DEVICE(ACCORDLE_VID, RESERVE_DID2)},
	{PCI_DEVICE(ACCORDSP_VID2, ACCORDSP_DID2)},
	{0, }
};
MODULE_DEVICE_TABLE(pci, acle_pci_id_table);

/**
 * @brief Регистрация PCI устройства СДЗ
 * @param[in] dev - PCI устройства 
 * @param[in] ip - PCI идентификатор
 * @return Результат завершения операции. 
 */

static int acle_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int res;

	if (id->vendor == ACCORDLE_VID && id->device == ACCORDLE_DID) {
		printk(KERN_INFO PREFIX "found Accord-LE v1.0 PCI-E controller\n");
	} else if (id->vendor == ACCORDLE_VID && id->device == ACCORDGX_DID) {
		printk(KERN_INFO PREFIX "found Accord-GX v1.0 PCI-E controller\n");
	} else if (id->vendor == ACCORDLE_VID && id->device == ACCORDGXM_DID) {
		printk(KERN_INFO PREFIX "found Accord-GX v1.0 "
			"mini PCI-E controller\n");
	} else if (id->vendor == ACCORDLE_VID && id->device == ACCORDGXMH_DID) {
		printk(KERN_INFO PREFIX "found Accord-GX v1.0 "
			"mini PCI-E half height controller\n");
	} else if (id->vendor == ACCORDLE_VID && id->device == ACCORDGX2AE_DID) {
                printk(KERN_INFO PREFIX "found Accord-GX v2.0 M.2 controller\n");
    } else if (id->vendor == ACCORDLE_VID && id->device == ACCORDGX2MH_DID) {
                printk(KERN_INFO PREFIX "found Accord-GX v2.0 "
                        "mini PCI-E half-size controller\n");
    } else if (id->vendor == ACCORDLE_VID && id->device == ACCORDGX5AE_DID) {
        printk(KERN_INFO PREFIX "found Accord-GX v5.0 "
                "M.2 controller\n");
    } else if (id->vendor == ACCORDLE_VID && id->device == RESERVE_DID) {
        printk(KERN_INFO PREFIX "found Accord-GX:\n");
    } else if (id->vendor == ACCORDLE_VID && id->device == RESERVE_DID2) {
        printk(KERN_INFO PREFIX "found Accord-GX:\n");
    } else if (id->vendor == ACCORDSP_VID2 && id->device == ACCORDSP_DID2) {
        printk(KERN_INFO PREFIX "found Accord-SP:\n");
    }
    else {
		printk(KERN_WARNING PREFIX "unsupported device: vendor %d,"
			" device %d\n", id->vendor, id->device);
		return -ENXIO;
	}

	/* драйвер поддерживает работу только с одной платой одновременно */
	if (atomic_read(&board_count)) {
		printk(KERN_WARNING PREFIX "this driver works with only one "
			"board at the time\n");
		return -ENXIO;
	}

	res = pci_enable_device(dev);
	if (res) {
		printk(KERN_WARNING PREFIX "failed to enable device: %d\n", res);
		return res;
	}

	if (pci_resource_flags(dev, 0) & IORESOURCE_MEM) {
		if (!request_mem_region(pci_resource_start(dev, 0),
			pci_resource_len(dev, 0), DRIVER_NAME)) {
			printk(KERN_ERR PREFIX "data IO memory busy\n");
			res = -EBUSY;
			goto disable_dev;
		}
		acle_data.data_mem = pci_iomap(dev, 0, pci_resource_len(dev, 0));
		if (!acle_data.data_mem) {
			printk(KERN_ERR PREFIX "failed to map data IO memory\n");
			res = -EIO;
			goto release_data_mem;
		}
	} else {
		printk(KERN_ERR PREFIX "BAR0 doesn't contain IO memory space address\n");
		res = -EIO;
		goto disable_dev;
	}

	/* определяем, как посылать команды плате: через память или регистры */
	if (pci_resource_flags(dev, 1) & IORESOURCE_MEM) {
		if (!request_mem_region(pci_resource_start(dev, 1),
			pci_resource_len(dev, 1), DRIVER_NAME)) {
			printk(KERN_ERR PREFIX "cmd IO memory busy\n");
			res = -EBUSY;
			goto unmap_data_mem;
		}
		acle_data.cmd_mem = pci_iomap(dev, 1, pci_resource_len(dev, 1));
		if (!acle_data.cmd_mem) {
			printk(KERN_ERR PREFIX "failed to map cmd IO memory\n");
			res = -EIO;
			goto release_cmd_io_region;
		}
	} else if (pci_resource_flags(dev, 1) & IORESOURCE_IO) {
		if (!request_region(pci_resource_start(dev, 1),
			pci_resource_len(dev, 1), DRIVER_NAME)) {
			printk(KERN_ERR PREFIX "IO ports busy\n");
			res = -EBUSY;
			goto unmap_data_mem;
		}
		acle_data.cmd_port = pci_resource_start(dev, 1);
	} else {
		printk(KERN_ERR PREFIX "BAR1 doesn't contain neither IO memory"
			"space address nor IO port address\n");
		res = -EIO;
		goto unmap_data_mem;
	}

	res = init_flash_chips(&acle_data);


	if (res <= 0) {
		if (res == 0)
			printk(KERN_ERR PREFIX "supported memory chips not found"
				" on the board\n");
		else
			printk(KERN_ERR PREFIX "failed to detect memory chips: %d\n", res);
		goto unmap_cmd_io_region;
	}

	acle_data.iobuf = kzalloc(MAX_READ_SIZE, GFP_KERNEL);
	if (!acle_data.iobuf) {
		printk(KERN_ERR PREFIX "failed to allocate memory for IO-buffer\n");
		res = -ENOMEM;
		goto unmap_cmd_io_region;
	}

	acle_data.page = kzalloc(ACLE_PAGE_SIZE, GFP_KERNEL);
	if (!acle_data.page) {
		printk(KERN_ERR PREFIX "failed to allocate memory for page buffer\n");
		res = -ENOMEM;
		goto free_iobuf;
	}

	if (init_rng(&acle_data, RNG_DIVIDER))
		printk(KERN_WARNING PREFIX "failed to initialize RNG\n");

	if (acle_data.cmd_mem)
		printk(KERN_INFO PREFIX "device initialized, data IO mem "
			"at 0x%8.8llx, cmd IO mem at 0x%8.8llx\n",
			pci_resource_start(dev, 0), pci_resource_start(dev, 1));
	else
		printk(KERN_INFO PREFIX "device initialized, data IO mem "
			"at 0x%8.8llx, IO ports at 0x%4llx\n",
			pci_resource_start(dev, 0), pci_resource_start(dev, 1));

	atomic_inc(&board_count);
  acle_debug_print("Kernel module version \"" DRIVER_VERSION "\" is initialized\n");
	return 0;
free_iobuf:
	kfree(acle_data.iobuf);
unmap_cmd_io_region:
	if (acle_data.cmd_mem)
		pci_iounmap(dev, acle_data.cmd_mem);
release_cmd_io_region:
	if (acle_data.cmd_mem)
		release_mem_region(pci_resource_start(dev, 1),
			pci_resource_len(dev, 1));
	else
		release_region(pci_resource_start(dev, 1),
			pci_resource_len(dev, 1));
unmap_data_mem:
	pci_iounmap(dev, acle_data.data_mem);
release_data_mem:
	release_mem_region(pci_resource_start(dev, 0), pci_resource_len(dev, 0));
disable_dev:
	pci_disable_device(dev);
	return res;
}

/**
 * @brief Функция удаления устройства
 * @param[in] dev - PCI устройства. 
 */
static void acle_remove(struct pci_dev *dev)
{
	atomic_dec(&board_count);
	kfree(acle_data.page);
	kfree(acle_data.iobuf);
	if (acle_data.cmd_mem) {
		pci_iounmap(dev, acle_data.cmd_mem);
		release_mem_region(pci_resource_start(dev, 1),
			pci_resource_len(dev, 1));
	} else {
		release_region(pci_resource_start(dev, 1),
			pci_resource_len(dev, 1));
	}
	pci_iounmap(dev, acle_data.data_mem);
	release_mem_region(pci_resource_start(dev, 0), pci_resource_len(dev, 0));
	pci_disable_device(dev);
}


/**
 * Структура PCI устройства 
 */
static struct pci_driver acle_pci_drv = {
	.name = DRIVER_NAME, ///< Имя PCI устройства
	.id_table = acle_pci_id_table, ///< масии PIC идентификаторов VID/DID
	.probe = acle_probe, ///< функция обратного вызова, используемая для инициализации устройства 
	.remove = acle_remove, ///< функция обратного вызова при удалении устройства
};

/**
 *@brief Функция инициализации модуля ядра
 */
static int __init acle_init(void)
{
	int res = misc_register(&acle_dev);
	if (res) {
		printk(KERN_ERR "failed to register misc device: %d\n", res);
		return res;
	}
	res = pci_register_driver(&acle_pci_drv);
	if (res) {
		printk(KERN_ERR "failed to register pci driver: %d\n", res);
		misc_deregister(&acle_dev);
		return res;
	}
	return 0;
}

/**
 * @brief Функция деинициализации модуля ядра
 */
static void __exit acle_exit(void)
{
	pci_unregister_driver(&acle_pci_drv);
	misc_deregister(&acle_dev);
}

module_init(acle_init)
module_exit(acle_exit)

MODULE_AUTHOR("OKB SAPR JSC");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
