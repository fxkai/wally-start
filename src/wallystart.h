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
#ifdef RASPBERRY
#include "bcm_host.h"
#endif

#define TEXT_SLOTS 255
#define VERSION "0.3"
#define BASE "."
#define FONT "/etc/wallyd.d/fonts/Cairo-Regular.ttf"
#define START WALLYD_CONFDIR"/wallystart.conf"
#define SDL_CMD_EVENT    (SDL_USEREVENT + 3)

int EventFilter(void *userdata, SDL_Event *event);
void initTexts(void);
void renderTexts(void);
bool setupText(int id, int x, int y, int size, char *color, long timeout, char *str);
bool loadSDL();
bool dumpSDLInfo();
TTF_Font *loadFont(char *file, int size);
bool fadeImage(SDL_Texture *text, int rot, bool reverse, long delay);
bool update(SDL_Texture *text, int rot);
SDL_Texture* loadImage(char *name);
void closeSDL();
bool dumpModes(void);
SDL_Texture* renderLog(char *strTmp,int *w, int *h);
void* logListener(void *);
void* processScript(void *file);
bool processCommand(char *buf);
char *repl_str(const char *str, const char *from, const char *to);
void hexToColor(char * colStr, SDL_Color *c);
void sig_handler(int);
SDL_Texture* renderText(char *, int, char *, int *, int *);
int getNumOrPercentEx(char *str, int relativeTo, int *value, int base);
bool fadeImage(SDL_Texture *text, int rot, bool reverse, long delay);
bool fadeOver(SDL_Texture *t1, SDL_Texture *t2,int rot, long delay);
void clearText(int id);

typedef struct texts {
  int x;
  int y;
  int w;
  int h;
  int size;
  SDL_Color color;
  SDL_Texture *tex;
  char *str;
  long timeout;
  bool active;
} texts;

#ifdef WALLYSTART_VARS
SDL_Texture *t1 = NULL;
SDL_Texture *t2 = NULL;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screenSurface = NULL;
TTF_Font *showFont = NULL;
SDL_Event event;
SDL_Color black = {0,0,0,255};
SDL_Color white = {255,255,255,255};
bool quit = false;
bool niceing = false;
bool repaint = false;
bool eventLoop = false;
int rot = 0, h = 0, w = 0;
pthread_t log_thr;
pthread_t startup_thr;
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
int errno;
SDL_Rect showLocation;

struct texts *textFields[TEXT_SLOTS];
#else
extern SDL_Texture *t1;
extern SDL_Texture *t2;
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern SDL_Surface* screenSurface;
extern TTF_Font *showFont;
extern bool quit;
extern bool repaint;
extern int rot, h, w;
extern char *cmdStr;
extern char *startupScript;
extern char *color;
extern char *answer;
extern char *showText;
extern char *showColor;
extern int showTime;
extern int showSize;
extern int showFontSize;
extern SDL_Rect showLocation;
extern struct texts *textFields[TEXT_SLOTS];
extern int errno;
#endif // WALLYSTART_VARS

#endif
