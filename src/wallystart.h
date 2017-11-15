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

#define VERSION "0.21"
#define BASE "."
#define FONT "/etc/wallyd.d/fonts/Cairo-Regular.ttf"
#define START WALLYD_CONFDIR"/wallystart.conf"

#define SDL_CMD_EVENT    (SDL_USEREVENT + 3)

extern char *logStr;
extern char *cmdStr;
extern int bindPort;
extern bool startupDone;

bool loadSDL();
bool dumpSDLInfo();
bool loadFont(char *file, int size);
bool fadeImage(SDL_Texture *text, int rot, bool reverse, long delay);
bool showTexture(SDL_Texture *text, int rot);
SDL_Texture* loadImage(char *name);
void closeSDL();
bool dumpModes(void);
SDL_Texture* renderLog(char *strTmp,int *w, int *h);
void* logListener(void *);
int sgetline(int fd, char ** out);
void processStartupScript(char *file);
bool processCommand(char *buf);
char *repl_str(const char *str, const char *from, const char *to);
void hexToColor(char * colStr, SDL_Color *c);
