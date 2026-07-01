#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "accord-le-ioctl.h"

/* Контекст тестирования */

#define ZTSR_NUM_BITS 106000L
#define ZTSR_NUM_BYTES (ZTSR_NUM_BITS >> 3)
#define ZTSR_HYST_SIZE 62

const uint8_t HYINDEX[5] = {0,2,6,14,30};

const uint16_t NMinZ[5] = {51940, 25440, 12190, 5565, 2253};
const uint16_t NMaxZ[5] = {54060, 27560, 14310, 7685, 4373};

const uint8_t MASKS[5] = {0x01,0x03,0x07,0x0F,0x1F};

/* Работа с аргументами командной строки */

typedef struct {
    char *random_dump_desired_filename;
    int  enable_random_dump;
} Args;

static void parse_args(int argc, char *argv[], Args *out_args)
{
    Args args;
    memset(&args, 0, sizeof(args));
    int i = 1;
    for(i = 1; i < argc; i++)
        {
            if(!strcmp("-h", argv[i])
               || !strcmp("--help", argv[i])
               || !strcmp("help", argv[i]))
                {
                    puts("\
Test amdz random generator. Always dump random bytes (binary) if the test wasn't passed.\n\
Usage:\n\
    acle-test-random-generator [-o [desired_filename]]\n\
Options:\n\
    -h, --help, help      - show this help\n\
    -o [desired_filename] - dump random bytes whether test was passed or not,\n\
                            use `desired_filename` as output file name if\n\
                            mentioned (default filename is 'rand-yyyy-mm-dd-hh-mm-ss')");
                    exit(0);
                }
            else if(!strcmp("-o", argv[i]))
                {
                    args.enable_random_dump = 1;
                }
            else if(!strcmp("-o", argv[i - 1]))
                {
                    args.random_dump_desired_filename = argv[i];
                }
        }
    *out_args = args;
}

int is_amdz_random_generator_ok(int      dev_fd,
                                uint8_t  *out_rnd_buf,
                                unsigned *out_rnd_buf_size)
{
    // взято с libamdz/src/device.c:amdz_test_random_generator()
    int res = 0;
    int i   = 0;

    uint8_t  j = 0, k = 0, iN = 0, Bit = 0, B = 0;
    uint8_t  UCN[ZTSR_HYST_SIZE * sizeof(uint64_t*)] = {0};
    uint8_t  *pUCN = NULL;
    uint64_t *pN = NULL;
    uint8_t  buf[1] = {0};
    uint8_t  amdzRndBuf[ZTSR_NUM_BYTES+60] = {0};

    struct acle_rng_request req;

    pUCN = UCN;
    pN = (uint64_t*)pUCN;

    /* Очистка рабочего накопителя */
    B = 0;
    /* Очистка гистограммы */
    memset(pUCN, 0, ZTSR_HYST_SIZE * sizeof(long*));

    for(i = 0; i < ZTSR_NUM_BYTES; i += 60)
        {
            res = 1;
            while(res)
                {
                    req.buf   = amdzRndBuf + i;
                    req.count = 60;
                    res = ioctl(dev_fd, ACLE_CMD_GET_RANDOM_BYTES, &req);
                }
        }

    for(i = 0; i < ZTSR_NUM_BYTES; i++)
        {
            buf[0] = amdzRndBuf[i];
            for(j = 0; j < 8; j++)
                {
                    /* Получаем бит от ГСЧ */
                    Bit = buf[0] & (1<<j);

                    /* Продвигаем бит в накопитель со стороны младших разрядов */
                        B <<= 1; 
                    if(Bit)
                        B |= 0x01;

                    /* Перебираем все типы отрезков */
                    for(k = 0; k < 5; k++)
                        {
                            /* Инкрементируем число в соответствующем члене гистограммы */
                            /* Находим смещение до нужного элемента гистограммы */
                            iN = HYINDEX[k] + (B & MASKS[k]);
                            /* Инкремент */
                            (*(pN + iN))++;
                        }
                }

        }

    /* Гистограмма построена - теперь ее анализ */
    for(k = 0; k < 5; k++)
        {             /* Для всех классов отрезков */
            for(j=0; j < (2 << k); j++)
                { /* Для всех возможных значений отрезков этого класса */
                    if(((*pN) < NMinZ[k]) || ((*pN) > NMaxZ[k]))
                        {
                          return 0;
                        }
                    pN++;
                }
        }

    /* Сохранить сгенерированный массив случайных чисел */
    if (*out_rnd_buf_size > sizeof(amdzRndBuf))
        {
            *out_rnd_buf_size = sizeof(amdzRndBuf);
        }
    if (*out_rnd_buf_size <= sizeof(amdzRndBuf))
        {
            memcpy(out_rnd_buf, amdzRndBuf, *out_rnd_buf_size);
        }
    return 1;
}

int main(int argc, char *argv[])
{
    uint8_t  buf[100000] = {};
    unsigned buf_size    = sizeof(buf);
    Args          args;
    int dev_fd = -1, res = -1;

    parse_args(argc, argv, &args);
    /* Открываем АМДЗ */
    dev_fd = open("/dev/accord-le", O_RDWR);
    if (dev_fd == -1)
        {
            fprintf(stderr, "failed to open device: %m\n");
            goto out;
        }

    int test = is_amdz_random_generator_ok(dev_fd, buf, &buf_size);
    if (test)
        printf ("\nThe test has been passed\n");
    else
        printf ("\nThe test has been failed\n");

    /* сохраняем дамп случайных чисел */
    if (!test || args.enable_random_dump)
        {
            char   now_str[32] = "rand-yyyy-mm-dd-hh-mm-ss.bin";
            time_t now         = time(NULL);
            FILE   *file       = NULL;
            strftime(now_str + 5, sizeof(now_str) - 5, "%Y-%m-%d-%H-%M-%S", localtime(&now));
            now_str[24] = '.'; // точка затирается завершающим нулём в strftime
            if(!args.random_dump_desired_filename)
                {
                    args.random_dump_desired_filename = now_str;
                }
            file = fopen(args.random_dump_desired_filename, "w");
            if(!file)
                {
                    printf("Warning: failed to open file '%s' for randoms dump\n",
                           args.random_dump_desired_filename);
                }
            else
                {
                    fwrite(buf, 1, buf_size, file);
                    fclose(file);
                    printf("\n Dump saved as '%s'\n", args.random_dump_desired_filename);
                }
        }

out:
    /* деинициализируем библиотеку */
    close(dev_fd);
    return res;
}
