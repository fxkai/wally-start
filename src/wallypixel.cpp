#include "wallypixel.h"

bool quit = false;
bool niceing = false;
int rot = 0, h = 0, w = 0;
pthread_t log_thr;
char *logStr = NULL;
//char *startupScript = "/etc/wallyd.d";
void* globalSLG;
int bindPort = 1105;
// If answer == NULL, the listener will echo back the input
// otherwise answer is sent
char answer;
PixelList Blinkt;

int main( int argc, char* argv[] )
{
    int argadd = 0;
    Pixel myPixel = Pixel(255,255,255);
    uint32_t red =   0xFF000003;               // hex codes fit in neatly, of course
    uint32_t green = 0x00FF003;
    uint32_t blue =  0x0000FF03;
    uint32_t white = 0xFFFFFF03;

    setPixel(Blinkt, blue);
    setPixel(Blinkt, green, 1);
    setPixel(Blinkt, red, 2);

    slog_init(NULL, WALLYD_CONFDIR"/wallyd.conf", DEFAULT_LOG_LEVEL, 0, LOG_ALL, LOG_ALL , true);

    slog(INFO,LOG_CORE,"%s (V" VERSION ")" ,argv[0]);
    rot = 0;

    if (start()){
        slog(ERROR,LOG_CORE, "Unable to start: bcm not initialising?");
        return 1;
    }
    slog(INFO,LOG_TEXTURE,"Initialized broadcom hardware");

    if(pthread_create(&log_thr, NULL, &logListener, NULL) != 0){
       slog(ERROR,LOG_CORE,"Failed to create listener thread!");
       return 1;
    }
    slog(INFO,LOG_TEXTURE,"Pixelserver listening on port %d",bindPort);

    while(!quit)
    {
           usleep(200000);
           Blinkt.show();
           slog(DEBUG,LOG_CORE,"show pixels, %s",logStr);
           if(logStr) {
                processCommand(strdup(logStr));
                free(logStr);
                logStr = NULL;
           }
    }
    return 0;
}

bool processCommand(char *buf)
{
    int ret;
    int i;
    int validCmd = 0;
    bool nextLine = true;
    char *lineBreak, *spaceBreak;
    char *lineCopy = NULL;
    char *cmd = strtok_r(buf,"\n",&lineBreak);
    while( nextLine ){
        // TODO : Keep track of this and clean it up!
        unsigned long cmdLen = strlen(cmd);
        lineCopy = repl_str(cmd, "$CONF", WALLYD_CONFDIR);
        void *linePtr = lineCopy;
        if(cmd[0] != '#') {
            validCmd++;
            char *myCmd = strsep(&lineCopy, " ");
            if(strcmp(myCmd,"nice") == 0){
              niceing = true;
            }
            else if(strcmp(myCmd,"setPixel") == 0){
                char *pixel       = strsep(&lineCopy, " ");
                char *color       = strsep(&lineCopy, " ");
                char *brightness  = strsep(&lineCopy, " ");
                long pixNum = atol(pixel);
                uint32_t col = (strtol(color,NULL,16) << 8) + atol(brightness);
                slog(DEBUG, LOG_CORE, "setPixel(%x, %x)", col, pixNum);
                setPixel(Blinkt, col, pixNum);
            }
            else if(strcmp(myCmd,"clearlog") == 0){
                free(logStr);
                char *logStr = strdup(" ");
                slog(DEBUG,LOG_CORE,"clearlog");
            }
            else if(strcmp(myCmd,"log") == 0){
                if(logStr) free(logStr);
                logStr = strdup(cmd+4);
                slog(DEBUG,LOG_CORE,"Set log to %s", logStr);
            }
            else if(strcmp(myCmd,"rot") == 0){
                char *rotStr = strsep(&lineCopy, " ");
                rot = atoi(rotStr);
                slog(DEBUG,LOG_CORE,"Set rotation to %u", rot);
            }
            else if(strcmp(myCmd,"sleep") == 0){
                char *sleepStr = strsep(&lineCopy, " ");
                int sl = atoi(sleepStr);
                slog(DEBUG,LOG_CORE,"Sleeping %u sec", sl);
                sleep(sl);
            }
            else {
                slog(WARN,LOG_CORE,"Command not valid : %s", cmd);
                validCmd--;
            }
        } else {
            slog(DEBUG,LOG_CORE,"Ignoring comment line");
        }
        free(linePtr);
        cmd = strtok_r(NULL,"\n",&lineBreak);
        if(cmd == NULL) nextLine=false;
    }
    slog(DEBUG,LOG_CORE,"Command stack executed.");
    return validCmd;
}

#ifndef LINUX
void bcm2835_spi_end(){ 
        slog(DEBUG,LOG_CORE,"bcm2835: spi_end()");
}
void bcm2835_close() { 
        slog(DEBUG,LOG_CORE,"bcm2835: close()");
}
bool bcm2835_init() { 
        slog(DEBUG,LOG_CORE,"bcm2835: init() -> returning true");
        return true;
}
void bcm2835_gpio_fsel(int a, int b) {
        slog(DEBUG,LOG_CORE,"bcm2835: gpio_fsel(%d,%d)",a,b);
}
void bcm2835_gpio_write(int a, int state) {
//        slog(DEBUG,LOG_CORE,"bcm2835: gpio_write(%d,%d)",a,state);
}

#endif
