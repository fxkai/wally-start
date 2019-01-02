#define WALLYSTART_VARS

#include "wallystart.h"

int main(int argc, char *argv[])
{
    char *start = START;
    color = strdup("ffffff");

    slog_init(NULL, WALLYD_CONFDIR "/wallyd.conf", DEFAULT_LOG_LEVEL, 0, LOG_ALL, LOG_ALL, true);

    slog(INFO, LOG_CORE, "%s (V" VERSION ")", argv[0]);

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        slog(ERROR, LOG_CORE, "Could not catch signal.");
        exit(1);
    }

    if (argc > 1) {
        start = argv[1];
    }

    initGFX();
    slog(INFO, LOG_CORE, "Screen size : %dx%d", w, h);

    // preFilter all Window events
    SDL_SetEventFilter(EventFilter, NULL);

    // create TCP listener
    if (pthread_create(&log_thr, NULL, &logListener, NULL) != 0) {
        slog(ERROR, LOG_CORE, "Failed to create listener thread!");
        exit(1);
    }

    // detach startup script reader
    if (pthread_create(&startup_thr, NULL, &processScript, start) != 0) {
        slog(ERROR, LOG_CORE, "Failed to create startup scipt thread!");
        exit(1);
    }

    // start a fader thread
    if (pthread_create(&fader_thr, NULL, &faderThread, NULL) != 0) {
        slog(ERROR, LOG_CORE, "Failed to create fader thread!");
        exit(1);
    }

    // start a timer thread
    if (pthread_create(&timer_thr, NULL, &timerThread, NULL) != 0) {
        slog(ERROR, LOG_CORE, "Failed to create timer thread!");
        exit(1);
    }

    while (!quit && SDL_WaitEvent(&event) != 0) {
        eventLoop = true;
        if (event.type == SDL_UPD_EVENT) {
            update(textures[0]->tex);
            continue;
        }
        if (event.type == SDL_CMD_EVENT) {
            slog(DEBUG, LOG_CORE, "New CMD event %s.", event.user.data1);
            processCommand(event.user.data1);
            free(event.user.data1);
            continue;
        }
        update(textures[0]->tex);
        slog(DEBUG, LOG_CORE, "Uncaught SDL event (%d / %d).", event.type, quit);
    }

    cleanupGFX();

    return 0;
}

bool update(SDL_Texture *tex)
{
    SDL_Rect fullSize = {0, 0, w, h};

    if (!rot) {
        SDL_RenderCopy(renderer, tex, NULL, &fullSize);
    } else {
        SDL_RenderCopyEx(renderer, tex, NULL, NULL, rot, NULL, SDL_FLIP_NONE);
    }

    renderTexts();
    SDL_RenderPresent(renderer);

    return true;
}

bool processCommand(char *buf)
{
    int logSize = h / 56;
    int i;
    int validCmd = 0;
    bool nextLine = true;
    char *lineBreak;
    char *lineCopy = NULL;
    char *cmd = strtok_r(buf, "\n", &lineBreak);
    while (nextLine)
    {
        // TODO : Keep track of this and clean it up!
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
                    textures[0]->tex = loadImage(file);
                    textures[0]->active = true;
                    textures[0]->fadein = 255;
                    textures[0]->fadedelay = (struct timespec){0, delay};
                    if (!textures[0]->tex) {
                        slog(ERROR, LOG_CORE, "Failed to load image %s.", file);
                    } else {
                        slog(DEBUG, LOG_CORE, "Loaded image texture %s.", file);
                        fadeImage(textures[0]->tex, false, delay * 1000);
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
                    textures[1]->tex = loadImage(file);
                    fadeOver(textures[0]->tex, textures[1]->tex, delay * 1000);
                    SDL_DestroyTexture(textures[0]->tex);
                    textures[0]->tex = textures[1]->tex;
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
                    textures[0]->tex = loadImage(fileA);
                    textures[1]->tex = loadImage(fileB);
                    for (i = 0; i < loop; i++) {
                        fadeOver(textures[0]->tex, textures[1]->tex, delay * 1000);
                        fadeOver(textures[1]->tex, textures[0]->tex, delay * 1000);
                    }
                    fadeOver(textures[0]->tex, textures[1]->tex, delay * 1000);
                    SDL_DestroyTexture(textures[0]->tex);
                    textures[0]->tex = textures[1]->tex;
                    textures[0]->active = true;
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
                    fadeImage(textures[0]->tex, true, delay * 1000);
                } else {
                    slog(DEBUG, LOG_CORE, "fadeout <delay>");
                }
                SDL_DestroyTexture(textures[0]->tex);
                textures[0]->active = false;
            } else if (strcmp(myCmd, "clearlog") == 0) {
                clearText(0);
            } else if (strcmp(myCmd, "run") == 0) {
                processScript(strdup(cmd + 4));
            } else if (strcmp(myCmd, "cleartext") == 0) {
                char *idStr = strsep(&lineCopy, " ");
                clearText(atoi(idStr));
            } else if (strcmp(myCmd, "log") == 0) {
                setupText(0, 1, h + 6 - ( 2 * logSize), logSize, strdup(color), 10, strdup(cmd + 4));
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
                update(textures[0]->tex);
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
        sleep(3);
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

void* faderThread(void *p) {
    int i;
    SDL_Event sdlevent;
    bool skipSleep = false;;
    while(!quit) {
        for (i = 0; i < TEXTURE_SLOTS; i++) {
            if(textures[i]->fadein > 0) {
                slog(INFO, LOG_CORE, "Found fadein %d", textures[i]->fadein);
                textures[i]->fadein -= 1;
                sdlevent.type = SDL_UPD_EVENT;
                SDL_PushEvent(&sdlevent);
                skipSleep = true;
            }
        }
        if(!skipSleep) {
            sleep(1);
        } else {
            skipSleep = false;
        }
    }
    return NULL;
}
// TODO : check if thread safety is needed
void* timerThread(void *p) {
    int i;
    SDL_Event sdlevent;
    while(!quit) {
        for (i = 0; i < TEXT_SLOTS; i++) {
            if(textFields[i]->active && textFields[i]->timeout > 0) {
                if(textFields[i]->timeout == 1) {
                    textFields[i]->active = false;
                    textFields[i]->destroy = true;
                    slog(INFO, LOG_CORE, "Notified text slot %d to be destroyed", i);
                    sdlevent.type = SDL_UPD_EVENT;
                    SDL_PushEvent(&sdlevent);
                }
                textFields[i]->timeout -= 1;
            }
        }
        sleep(1);
    }
    return NULL;
}
