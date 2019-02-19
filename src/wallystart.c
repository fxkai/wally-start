#include "wallystart.h"

bool blockCommands = false;
long updateCounter = 0;

int main(int argc, char *argv[])
{
    char *start = START;
    color = strdup("ffffff");

    slog_init(NULL, WALLYD_CONFDIR "/wallyd.conf", DEFAULT_LOG_LEVEL, 0, LOG_ALL, LOG_ALL, true);
    slog(INFO, LOG_CORE, "%s (V" VERSION ")", argv[0]);

    if (argc > 1)
        start = argv[1];

    if(!initGFX())
        exit(1);

    SDL_SetEventFilter(EventFilter, NULL);

    if(!initThreadsAndHandlers(start))
        exit(1);

    while (!quit && SDL_WaitEvent(&event) != 0) {
        eventLoop = true;
        if (event.type == SDL_UPD_EVENT) {
            // keep the queue short
            //updateCounter++;
            //if(updateCounter % 3)
                SDL_FlushEvent(SDL_UPD_EVENT);
            update(event.user.code);
            continue;
        }
        if (event.type == SDL_LOADIMAGE_EVENT) {
            slog(DEBUG, LOG_CORE, "Loading image %s to slot %d", event.user.data1, event.user.code);
            textures[event.user.code]->tex = loadImage(event.user.data1);
            free(event.user.data1);
            continue;
        }
        // TODO : alloc and destroy Textures via message
        if (event.type == SDL_ALLOC_EVENT) {
            continue;
        }
        // TODO : alloc and destroy Textures via message
        if (event.type == SDL_DESTROY_EVENT) {
            slog(DEBUG, LOG_CORE, "Destroy texture %d.", event.user.code);
            SDL_DestroyTexture(event.user.data1);
            update(-1);
            continue;
        }
        if (event.type == SDL_CMD_EVENT) {
            slog(DEBUG, LOG_CORE, "CMD event %s.", event.user.data1);
            // processCommand(event.user.data1);
            // free(event.user.data1);
            continue;
        }
        // update(0);
        slog(DEBUG, LOG_CORE, "Uncaught generic SDL event %d.", event.type);
    }

    cleanupGFX();

    return 0;
}

// bool update(SDL_Texture *tex)
bool update(int texId)
{
    int i;

    // New mechanism (loop over all dirty textures)
    if(texId == -1) {
        for(i = 0; i < TEXTURE_SLOTS; i++) {
            if(textures[i]->dirty && textures[i]->active) {
                update(i);
            }
        }
        return true;
    }

    if(texId > TEXTURE_SLOTS) {
        slog(ERROR, LOG_CORE, "Update failure for slot %d (not existand)", texId);
        return 0;
    }

    SDL_Texture *tex = textures[texId]->tex;
    int alpha = textures[texId]->alpha;
    SDL_Rect fullSize = {0, 0, w, h};

    // slog(TRACE,LOG_CORE,"Update %d (alpha:%d)", texId, alpha);

    if(textures[texId]->fadein) {
        SDL_SetTextureColorMod(tex, alpha, alpha, alpha);
        // slog(TRACE,LOG_CORE,"Fadein/out");
    }
    if(textures[texId]->fadeover) {
        // slog(TRACE,LOG_CORE,"Fadeover %d", texId);
        int srcId = textures[texId]->fadesrc;
        SDL_Texture *src = textures[srcId]->tex;
        tex = fadeOver(src, tex, textures[2]->tex, textures[texId]->alpha);
    }

    if (!rot) {
        SDL_RenderCopy(renderer, tex, NULL, &fullSize);
    } else {
        SDL_RenderCopyEx(renderer, tex, NULL, NULL, rot, NULL, SDL_FLIP_NONE);
    }

    textures[texId]->dirty = false;

    renderTexts();
    SDL_RenderPresent(renderer);

    return true;
}

void* faderThread(void *p) {
    SDL_Event sdlevent;
    int i;
    bool skipSleep = false;

    while(!quit) {
        for (i = 0; i < TEXTURE_SLOTS; i++) {
            SDL_zero(sdlevent);
            if(textures[i]->fadein == 1) {
                slog(DEBUG, LOG_CORE, "Found last fadein %d. Clearing.", textures[i]->fadein);
                resetTexture(i);
                textures[i]->alpha = 255;
                sdlevent.type = SDL_UPD_EVENT;
                sdlevent.user.code = i;
                SDL_PushEvent(&sdlevent);
                blockCommands = false;
            }
            if(textures[i]->fadein > 0) {
                // slog(DEBUG, LOG_CORE, "Found fadein %d (delay %d)", textures[i]->fadein, textures[i]->delay);
                textures[i]->fadein -= 1;
                textures[i]->dirty = 1;
                textures[i]->alpha = 255 - textures[i]->fadein;
                sdlevent.type = SDL_UPD_EVENT;
                sdlevent.user.code = i;
                SDL_PushEvent(&sdlevent);
                skipSleep = true;
                blockCommands = true;
                usleep(textures[i]->delay);
            }

            if(textures[i]->fadeloop > 0 && textures[i]->fadeover < 2) {
                slog(DEBUG,LOG_CORE, "toggle fadeloop %d (%d)",i, textures[i]->fadeloop);
                int src = textures[i]->fadesrc;
                resetTexture(src);
                textures[src]->fadeover = 255;
                textures[src]->fadeloop = textures[i]->fadeloop - 1;
                textures[src]->fadesrc = i;
                textures[src]->active = true;
                textures[src]->delay = textures[i]->delay;
                resetTexture(i);
                blockCommands = true;
                skipSleep = true;
            }
            // Finish fadeover
            if(textures[i]->fadeover == 1) {
                slog(DEBUG,LOG_CORE, "Clean up fadeover");
                int src = textures[i]->fadesrc;
                slog(DEBUG,LOG_CORE, "Send destroy event for texture %d", src);
                sdlevent.type = SDL_DESTROY_EVENT;
                sdlevent.user.code = src;
                sdlevent.user.data1 = textures[src]->tex;
                SDL_PushEvent(&sdlevent);
                slog(DEBUG,LOG_CORE, "Move texture %d to %d", i, src);
                copyTexture(i, src);
                resetTexture(src);
                resetTexture(i);
                textures[i]->active = false;
                textures[src]->active = true;
                // Update all dirty texs
                sdlevent.type = SDL_UPD_EVENT;
                sdlevent.user.code = i;
                SDL_PushEvent(&sdlevent);
                skipSleep = false;
                blockCommands = false;
            }
            if(textures[i]->fadeover > 0) {
                // slog(DEBUG, LOG_CORE, "Found fadeover from %d to %d : %d (delay %d)", textures[i]->fadesrc, i, textures[i]->fadeover, textures[i]->delay);
                textures[i]->fadeover -= 1;
                textures[i]->dirty = 1;
                textures[i]->alpha = textures[i]->fadeover;
                sdlevent.type = SDL_UPD_EVENT;
                sdlevent.user.code = i;
                SDL_PushEvent(&sdlevent);
                skipSleep = true;
                blockCommands = true;
                usleep(textures[i]->delay);
            }
 
        }
        if(!skipSleep)
            usleep(100000);
        else
            skipSleep = false;
    }
    return NULL;
}

bool processCommand(char *buf)
{
    int logSize = h / 56;
    int validCmd = 0;
    bool nextLine = true;
    char *lineBreak;
    char *lineCopy = NULL;
    char *cmd = strtok_r(buf, "\n", &lineBreak);
    SDL_Event sdlevent;
    while (nextLine)
    {
        while(blockCommands){
            usleep(100000);
            //slog(TRACE, LOG_CORE, "CMD blocking.");
        }
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
                    // TODO : is that thread safe?
                    // textures[0]->tex = loadImage(strdup(file));
                    SDL_zero(sdlevent);
                    sdlevent.type = SDL_LOADIMAGE_EVENT;
                    sdlevent.user.code = 0;
                    sdlevent.user.data1 = strdup(file);
                    SDL_PushEvent(&sdlevent);
                    textures[0]->active = true;
                    textures[0]->fadein = 255;
                    textures[0]->delay = delay;
                    blockCommands = true;
                } else {
                    slog(DEBUG, LOG_CORE, "fadein <delay> <file>");
                }
            } else if (strcmp(myCmd, "fadeout") == 0) {
                char *delayStr = strsep(&lineCopy, " ");
                long delay = 45000;
                if (delayStr != NULL) {
                    delay = atol(delayStr);
                }
                if (delay) {
                    slog(DEBUG, LOG_CORE, "Fadeout with delay %u", delay);
                    textures[0]->active = true;
                    textures[0]->fadeout = 255;
                    textures[0]->delay = delay;
                    blockCommands = true;
                    // TODO : destroy texture
                } else {
                    slog(DEBUG, LOG_CORE, "fadeout <delay>");
                }
            }
            else if (strcmp(myCmd, "fadeover") == 0) {
                char *delayStr = strsep(&lineCopy, " ");
                char *file = strsep(&lineCopy, " ");
                long delay = atol(delayStr);
                slog(DEBUG, LOG_CORE, "Fadeover %s with delay %u", file, delay);
                if (file && delay) {
                    // TODO : is that thread safe?
                    SDL_zero(sdlevent);
                    sdlevent.type = SDL_LOADIMAGE_EVENT;
                    sdlevent.user.code = 1;
                    sdlevent.user.data1 = strdup(file);
                    SDL_PushEvent(&sdlevent);
                    textures[1]->fadeover = 255;
                    textures[1]->fadesrc = 0;
                    textures[1]->delay = delay;
                    textures[1]->active = true;
                    blockCommands = true;
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
                slog(DEBUG, LOG_CORE, "Fadeloop %d times from %s to %s with delay %u", loop, fileA, fileB, delay);
                if (fileA && fileB && loop && delay) {
                    sdlevent.type = SDL_LOADIMAGE_EVENT;
                    sdlevent.user.code = 0;
                    sdlevent.user.data1 = strdup(fileB);
                    SDL_PushEvent(&sdlevent);
                    sdlevent.type = SDL_LOADIMAGE_EVENT;
                    sdlevent.user.code = 1;
                    sdlevent.user.data1 = strdup(fileA);
                    SDL_PushEvent(&sdlevent);
                    textures[0]->fadeloop = loop;
                    textures[0]->fadeover = 255;
                    textures[0]->fadesrc = 1;
                    textures[0]->delay = delay;
                    textures[0]->active = true;
                    blockCommands = true;
                    // TODO : destroy textures
                } else {
                    slog(DEBUG, LOG_CORE, "fadeloop <num> <delay> <fileA> <fileB>");
                }
           } else if (strcmp(myCmd, "clearlog") == 0) {
                clearText(0);
            } else if (strcmp(myCmd, "run") == 0) {
                processScript(strdup(cmd + 4));
            } else if (strcmp(myCmd, "cleartext") == 0) {
                char *idStr = strsep(&lineCopy, " ");
                clearText(atoi(idStr));
            } else if (strcmp(myCmd, "log") == 0) {
                setupText(0, 1, h + 6 - ( 2 * logSize), logSize, strdup(color), 10, strdup(cmd + 4));
                update(0);
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
                update(0);
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
    // Wait for eventloop to be initialized
    while (!eventLoop) 
        sleep(1);

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
    slog(DEBUG, LOG_CORE, "Processing %d bytes from script", fsize);

    processCommand(cmds);
    free(cmds);

    return NULL;
}

// TODO : check if thread safety is needed acessing the textures array
void* timerThread(void *p) {
    SDL_Event sdlevent;
    int i;
    while(!quit) {
        for (i = 0; i < TEXT_SLOTS; i++) {
            if(textFields[i]->active && textFields[i]->timeout > 0) {
                SDL_zero(sdlevent);
                if(textFields[i]->timeout == 1) {
                    textFields[i]->active = false;
                    textFields[i]->destroy = true;
                    slog(INFO, LOG_CORE, "Notified text slot %d to be destroyed", i);
                    sdlevent.type = SDL_UPD_EVENT;
                    sdlevent.user.code = -1;
                    SDL_PushEvent(&sdlevent);
                }
                textFields[i]->timeout -= 1;
            }
        }
        sleep(1);
    }
    return NULL;
}
