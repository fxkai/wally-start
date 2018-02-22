#ifndef _WALLYLISTENER_H
#define _WALLYLISTENER_H

#include <SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <signal.h>
#include "slog.h"
#include "autoversion.h"

#define SDL_CMD_EVENT    (SDL_USEREVENT + 3)

extern char *logStr;
extern char *answer;
extern char *cmdStr;
extern bool startupDone;
extern int bindPort;

char *repl_str(const char *str, const char *from, const char *to);
int sgetline(int fd, char ** out);

#endif
