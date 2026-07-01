/** @file read_buf.h API для восстановления данных из массива памяти. */
#ifndef __IC_LIBS__MISC__READ_BUF_H
#define __IC_LIBS__MISC__READ_BUF_H

#include <ic-libs/ic_err.h>

#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Дескриптор буфера для чтения. */
struct read_buf {
	unsigned char *buf;
	size_t capacity;
	size_t read_pos;
};

void rb_init(struct read_buf *rbuf, unsigned char *buf, size_t capacity);
void rb_finalize(struct read_buf *rbuf, int dealloc);
int rb_read(struct read_buf *rbuf, void *buf, size_t len);
int rb_get_ptr(struct read_buf *rbuf, size_t len, const void **pptr);

/**
 * Восстанавливает из буфера байт.
 *
 * @param[in] rbuf дескриптор буфера.
 * @param[out] n   адрес для возврата прочитанного значения.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int rb_read_byte(struct read_buf *rbuf, unsigned char *n)
{
	return rb_read(rbuf, n, sizeof(*n));
}

/**
 * Восстанавливает из буфера значение типа uint16_t.
 *
 * @param[in] rbuf дескриптор буфера.
 * @param[out] n   адрес для возврата прочитанного значения.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int rb_read_uint16(struct read_buf *rbuf, uint16_t *n)
{
	return rb_read(rbuf, n, sizeof(*n));
}

/**
 * Восстанавливает из буфера значение типа uint32_t.
 *
 * @param[in] rbuf дескриптор буфера.
 * @param[out] n   адрес для возврата прочитанного значения.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int rb_read_uint32(struct read_buf *rbuf, uint32_t *n)
{
	return rb_read(rbuf, n, sizeof(*n));
}

/**
 * Восстанавливает из буфера значение типа uint64_t.
 *
 * @param[in] rbuf дескриптор буфера.
 * @param[out] n   адрес для возврата прочитанного значения.
 *
 * @return При успешном выполнении возвращает IC_OK, в противном случае
 *         код ошибки.
 */
static inline int rb_read_uint64(struct read_buf *rbuf, uint64_t *n)
{
	return rb_read(rbuf, n, sizeof(*n));
}

/**
 * Возвращает количество байт доступных в буфере для чтения.
 * @param[in] rbuf дескриптор буфера.
 *
 * @return количество байт доступных в буфере для чтения.
 */
static inline size_t rb_avail(struct read_buf *rbuf)
{
	return rbuf->capacity - rbuf->read_pos;
}

int rb_read_str(struct read_buf *rbuf, const char **pstr, uint16_t *plen);
int rb_read_and_alloc_zstr(struct read_buf *rbuf, char **pzstr);

int rb_read_u16str(struct read_buf *rbuf, const uint16_t **pstr, uint16_t *plen);
int rb_read_and_alloc_u16zstr(struct read_buf *rbuf, uint16_t **pzstr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __IC_LIBS__MISC__READ_BUF_H */
