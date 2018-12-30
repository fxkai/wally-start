#define WALLYSTART_VARS

#include "wallystart.h"

int main(int argc, char *argv[])
{
    char *start = START;
    color = strdup("ffffff");

    slog_init(NULL, WALLYD_CONFDIR "/wallyd.conf", DEFAULT_LOG_LEVEL, 0, LOG_ALL, LOG_ALL, true);

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        slog(ERROR, LOG_CORE, "Could not catch signal.");
    }

    if (argc > 1) {
        start = argv[1];
    }

    slog(INFO, LOG_CORE, "%s (V" VERSION ")", argv[0]);

#ifdef RASPBERRY
    bcm_host_init();
    slog(INFO, LOG_TEXTURE, "Initializing broadcom hardware");
#endif
#ifndef DARWIN
    slog(INFO, LOG_TEXTURE, "Enable SDL2 verbose logging");
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#endif

    initTexts();
    dumpModes();

    if (!loadSDL()) {
        slog(ERROR, LOG_CORE, "Failed to initialize SDL. Exit");
        exit(1);
    }

    dumpSDLInfo();

    // preFilter all Window events
    SDL_SetEventFilter(EventFilter, NULL);

    // create TCP listener
    if (pthread_create(&log_thr, NULL, &logListener, NULL) != 0) {
        slog(ERROR, LOG_CORE, "Failed to create listener thread!");
        exit(1);
    }

    slog(INFO, LOG_CORE, "Screen size : %dx%d", w, h);

    if (pthread_create(&startup_thr, NULL, &processScript, start) != 0) {
        slog(ERROR, LOG_CORE, "Failed to create startup thread!");
        exit(1);
    }

    while (!quit && SDL_WaitEvent(&event) != 0) {
        eventLoop = true;
        if (event.type == SDL_CMD_EVENT) {
            slog(DEBUG, LOG_CORE, "New CMD event %s.", event.user.data1);
            processCommand(event.user.data1);
            free(event.user.data1);
            continue;
        }
        update(t1, rot);
        slog(DEBUG, LOG_CORE, "Uncaught SDL event (%d / %d).", event.type, quit);
    }

    SDL_DestroyTexture(t1);
    if (t2) {
        SDL_DestroyTexture(t2);
    }
    closeSDL();
    return 0;
}

bool update(SDL_Texture *tex1, int rot)
{
    //SDL_Rect r = { h-16, 0, h, w };
    SDL_Rect rShow = {0, 0, showFontSize, w};
    SDL_Rect fullSize = {0, 0, w, h};
    int tw, th;
    SDL_Texture *tex3;

    if (rot == 0) {
        SDL_RenderCopy(renderer, tex1, NULL, &fullSize);
    } else {
        SDL_RenderCopyEx(renderer, tex1, NULL, NULL, rot, NULL, SDL_FLIP_NONE);
    }
    renderTexts();
    SDL_RenderPresent(renderer);

    return true;
}

bool processCommand(char *buf)
{
    int ret;
    int i;
    int validCmd = 0;
    bool nextLine = true;
    char *lineBreak, *spaceBreak;
    char *lineCopy = NULL;
    char *cmd = strtok_r(buf, "\n", &lineBreak);
    int logSize = h / 56;
    while (nextLine)
    {
        // TODO : Keep track of this and clean it up!
        unsigned long cmdLen = strlen(cmd);
        lineCopy = repl_str(cmd, "$CONF", WALLYD_CONFDIR);
        void *linePtr = lineCopy;
        if (cmd[0] != '#') {
            validCmd++;
            if (strncmp(lineCopy, "quit", 4) == 0) {
                kill(getpid(), SIGINT);
            }
            char *myCmd = strsep(&lineCopy, " ");
            if (strcmp(myCmd, "nice") == 0) {
                niceing = true;
            }
            if (strcmp(myCmd, "fadein") == 0) {
                char *delayStr = strsep(&lineCopy, " ");
                char *file = strsep(&lineCopy, " ");
                long delay = atol(delayStr);
                slog(DEBUG, LOG_CORE, "Fadein %s with delay %u", file, delay);
                if (file && delay) {
                    t1 = loadImage(file);
                    if (!t1) {
                        slog(ERROR, LOG_CORE, "Failed to load image %s.", file);
                    } else {
                        slog(DEBUG, LOG_CORE, "Loaded image texture %s.", file);
                        fadeImage(t1, rot, false, delay * 1000);
                    }
                } else {
                    slog(DEBUG, LOG_CORE, "fadein <delay> <file>");
                }
            }
            else if (strcmp(myCmd, "fadeover") == 0) {
                char *delayStr = strsep(&lineCopy, " ");
                char *file = strsep(&lineCopy, " ");
                long delay = atol(delayStr);
                slog(DEBUG, LOG_CORE, "Fadeover %s with delay %u", file, delay);
                if (file && delay) {
                    t2 = loadImage(file);
                    fadeOver(t1, t2, rot, delay * 1000);
                    SDL_DestroyTexture(t1);
                    t1 = t2;
                } else {
                    slog(DEBUG, LOG_CORE, "fadeover <delay> <file>");
                }
            } else if (strcmp(myCmd, "fadeloop") == 0) {
                char *loopStr = strsep(&lineCopy, " ");
                char *delayStr = strsep(&lineCopy, " ");
                char *fileA = strsep(&lineCopy, " ");
                char *fileB = strsep(&lineCopy, " ");
                long delay = atol(delayStr);
                long loop = atol(loopStr);
                slog(DEBUG, LOG_CORE, "Fadeloop %d times from  %s to %s with delay %u", loop, fileA, fileB, delay);
                if (fileA && fileB && loop && delay) {
                    t1 = loadImage(fileA);
                    t2 = loadImage(fileB);
                    for (i = 0; i < loop; i++) {
                        fadeOver(t1, t2, rot, delay * 1000);
                        fadeOver(t2, t1, rot, delay * 1000);
                    }
                    fadeOver(t1, t2, rot, delay * 1000);
                    SDL_DestroyTexture(t1);
                    t1 = t2;
                } else {
                    slog(DEBUG, LOG_CORE, "fadeloop <num> <delay> <fileA> <fileB>");
                }
            } else if (strcmp(myCmd, "fadeout") == 0) {
                char *delayStr = strsep(&lineCopy, " ");
                long delay = 4500000;
                if (delayStr != NULL) {
                    delay = atol(delayStr);
                }
                slog(DEBUG, LOG_CORE, "Fadeout with delay %u", delay);
                if (delay) {
                    fadeImage(t1, rot, true, delay * 1000);
                } else {
                    slog(DEBUG, LOG_CORE, "fadeout <delay>");
                }
                SDL_DestroyTexture(t1);
            } else if (strcmp(myCmd, "clearlog") == 0) {
                clearText(0);
            } else if (strcmp(myCmd, "log") == 0) {
                setupText(0, 1, h + 6 - ( 2 * logSize), logSize, strdup(color), 0, strdup(cmd + 4));
            } else if (strcmp(myCmd, "run") == 0) {
                processScript(strdup(cmd + 4));
            } else if (strcmp(myCmd, "cleartext") == 0) {
                char *idStr = strsep(&lineCopy, " ");
                clearText(atoi(idStr));
            } else if (strcmp(myCmd, "text") == 0) {
                char *idStr = strsep(&lineCopy, " ");
                char *xStr = strsep(&lineCopy, " ");
                char *yStr = strsep(&lineCopy, " ");
                char *szStr = strsep(&lineCopy, " ");
                char *colStr = strsep(&lineCopy, " ");
                char *timeStr = strsep(&lineCopy, " ");
                int tx = 0;
                int ty = 0;
                int tsize = 0;
                showText = strdup(lineCopy);
                if (!showText) {
                    slog(ERROR, LOG_CORE, "text <id 1-255> <x> <y> <size> <color> <duration> <textstring>");
                    free(lineCopy);
                    free(showText);
                    return false;
                }
                getNumOrPercentEx(xStr, w, &tx, 10);
                getNumOrPercentEx(yStr, h, &ty, 10);
                getNumOrPercentEx(szStr, h, &tsize, 10);
                setupText(atoi(idStr), tx, ty, tsize, strdup(colStr), atoi(timeStr), showText);
                update(t1, 0);
            } else if (strcmp(myCmd, "rot") == 0)
            {
                char *rotStr = strsep(&lineCopy, " ");
                rot = atoi(rotStr);
                slog(DEBUG, LOG_CORE, "Set rotation to %u", rot);
            } else if (strcmp(myCmd, "color") == 0)
            {
                free(color);
                color = strdup(strsep(&lineCopy, " "));
                slog(DEBUG, LOG_CORE, "Set color to %s", color);
            } else if (strcmp(myCmd, "sleep") == 0)
            {
                char *sleepStr = strsep(&lineCopy, " ");
                int sl = atoi(sleepStr);
                slog(DEBUG, LOG_CORE, "Sleeping %u sec", sl);
                sleep(sl);
            } else {
                slog(WARN, LOG_CORE, "Command not valid : %s", cmd);
                validCmd--;
            }
        } else {
            slog(DEBUG, LOG_CORE, "Ignoring comment line");
        }
        free(linePtr);
        cmd = strtok_r(NULL, "\n", &lineBreak);
        if (cmd == NULL)
            nextLine = false;
    }
    slog(DEBUG, LOG_CORE, "Command stack executed.");
    return validCmd;
}

void *processScript(void *file)
{
    while (!eventLoop) 
        sleep(1);
    SDL_Event sdlevent;
    slog(DEBUG, LOG_CORE, "Reading script : %s", file);
    long fsize = 0;
    char *cmds = NULL;

    FILE *f = fopen(file, "rb");
    if (!f) {
        slog(DEBUG, LOG_CORE, "File not found. Not running anything.");
        return (void*)0;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    cmds = malloc(fsize + 1);
    fread(cmds, fsize, 1, f);
    fclose(f);

    cmds[fsize] = 0;
    slog(DEBUG, LOG_CORE, "Processing %d bytes from startupScript", fsize);
    sdlevent.type = SDL_CMD_EVENT;
    sdlevent.user.data1 = strdup(cmds);
    SDL_PushEvent(&sdlevent);
    free(cmds);
    return (void*)0;
}
