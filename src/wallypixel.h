#ifndef _WALLYPIXEL_
#define _WALLYPIXEL_
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#define WALLYSTART 1
#include "slog.h"
#include "autoversion.h"
#ifdef LINUX
#include <bcm2835.h>
#include <linux/types.h>
#else

#endif
#include <stdint.h>
#include <iostream>
#include <string>
#include <signal.h> // some kind of magic I guess
#include <unistd.h>  // usleep
#include <stdlib.h>
#include "pixel.h"
#include "clinkt.h"

#ifndef LINUX
#define BCM2835_GPIO_FSEL_OUTP 0b001
#define LOW  0x0
#define HIGH  0x1
#endif

#define VERSION "0.16"

extern char *logStr;
extern int bindPort;

extern "C" {
    void bcm2835_spi_end(void);
    void bcm2835_close(void);
    bool bcm2835_init(void);
    void bcm2835_gpio_fsel(int a, int b);
    void bcm2835_gpio_write(int a, int state);
}
extern "C" {
    void* logListener(void *);
    int sgetline(int fd, char ** out);
    void processStartupScript(char *file);
    bool processCommand(char *buf);
    char *repl_str(const char *str, const char *from, const char *to);
}
#endif
