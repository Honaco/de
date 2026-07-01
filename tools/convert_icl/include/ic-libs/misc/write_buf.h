/** @file write_buf.h API для записи данных в массив памяти. */
#ifndef __IC_LIBS__MISC__WRITE_BUF_H
#define __IC_LIBS__MISC__WRITE_BUF_H

#include <ic-libs/ic_err.h>

#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Дескриптор буфера для записи. */
struct write_buf {
	unsigned char *buf;
	size_t capacity;
	size_t write_pos;
};

int wb_init(struct write_buf *wbuf, size_t capacity);
unsigned char *wb_finalize(struct write_buf *wbuf, int dealloc);
int wb_write(struct write_buf *wbuf, const void *buf, size_t len);

/**
 * Записывает байт в буфер.
 *
 * @param[in] wbuf дескриптор буфера для записи.
 * @param[in] byte записываемый байт.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int wb_write_byte(struct write_buf *wbuf, unsigned char byte)
{
	return wb_write(wbuf, &byte, sizeof(byte));
}

/**
 * Записывает в буфер значение типа uint16_t.
 *
 * @param[in] wbuf дескриптор буфера для записи.
 * @param[in] byte записываемой значение.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int wb_write_uint16(struct write_buf *wbuf, uint16_t n)
{
	return wb_write(wbuf, &n, sizeof(n));
}

/**
 * Записывает в буфер значение типа uint32_t.
 *
 * @param[in] wbuf дескриптор буфера для записи.
 * @param[in] n    записываемой значение.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int wb_write_uint32(struct write_buf *wbuf, uint32_t n)
{
	return wb_write(wbuf, &n, sizeof(n));
}

/**
 * Записывает в буфер значение типа uint64_t.
 *
 * @param[in] wbuf дескриптор буфера для записи.
 * @param[in] n    записываемой значение.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int wb_write_uint64(struct write_buf *wbuf, uint64_t n)
{
	return wb_write(wbuf, &n, sizeof(n));
}

int wb_write_str(struct write_buf *wbuf, const char *str);
int wb_write_u16str(struct write_buf *wbuf, const uint16_t *str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IC_LIBS__MISC__WRITE_BUF_H */
