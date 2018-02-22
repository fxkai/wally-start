#ifndef _WALLYSTART_H
#define _WALLYSTART_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
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

#define VERSION "0.3"
#define BASE "."
#define FONT "/etc/wallyd.d/fonts/Cairo-Regular.ttf"
#define START WALLYD_CONFDIR"/wallystart.conf"
#define SDL_CMD_EVENT    (SDL_USEREVENT + 3)

SDL_Texture *t1 = NULL;
SDL_Texture *t2 = NULL;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screenSurface = NULL;
TTF_Font *logFont = NULL;
TTF_Font *showFont = NULL;
SDL_Event event;
SDL_Color black = {0,0,0,255};
SDL_Color white = {255,255,255,255};
bool quit = false;
bool niceing = false;
bool startupDone = false;
int rot = 0, h = 0, w = 0;
pthread_t log_thr;
char *logStr = NULL;
char *cmdStr = NULL;
char *startupScript = "/etc/wallyd.d";
void* globalSLG;
char *color;
int bindPort = 1109;
int logFontSize = 16;
// If answer == NULL, the listener will echo back the input
// otherwise answer is sent
char *answer = "OK\r\n";

char *showText = NULL;
char *showColor;
int showTime = 5;
int showSize = 32;
int showFontSize = 0;
int errno = 0;
SDL_Rect showLocation;

bool loadSDL();
bool dumpSDLInfo();
TTF_Font *loadFont(char *file, int size);
bool fadeImage(SDL_Texture *text, int rot, bool reverse, long delay);
bool showTexture(SDL_Texture *text, int rot);
SDL_Texture* loadImage(char *name);
void closeSDL();
bool dumpModes(void);
SDL_Texture* renderLog(char *strTmp,int *w, int *h);
void* logListener(void *);
void processStartupScript(char *file);
bool processCommand(char *buf);
char *repl_str(const char *str, const char *from, const char *to);
void hexToColor(char * colStr, SDL_Color *c);
void sig_handler(int);
SDL_Texture* renderText(char *, int, char *, int *, int *);
int getNumOrPercentEx(char *str, int relativeTo, int *value, int base);

#endif
