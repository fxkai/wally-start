#include "wallystart.h"

SDL_Texture *t1 = NULL;
SDL_Texture *t2 = NULL;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screenSurface = NULL;
TTF_Font *font = NULL;
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
int fontSize = 16;
// If answer == NULL, the listener will echo back the input
// otherwise answer is sent
char *answer = "OK\r\n";
void sig_handler(int);

int main( int argc, char* argv[] )
{
    SDL_Texture *t3 = NULL;
    int argadd = 0;
    char *start;
    logStr = strdup(" ");
    color = strdup("0xffffff");
    struct timespec eventDelay = { 0, 1000000};
    startupDone = false;

    slog_init(NULL, WALLYD_CONFDIR"/wallyd.conf", DEFAULT_LOG_LEVEL, 0, LOG_ALL, LOG_ALL , true);

    if (signal(SIGINT, sig_handler) == SIG_ERR){
        slog(ERROR,LOG_CORE, "Could not catch signal.");
    }


    if(argc > 1){
        start = argv[1]; 
    } else {
        start = START;
    }

    slog(INFO,LOG_CORE,"%s (V"VERSION")" ,argv[0]);
    rot = 0;

#ifdef RASPBERRY
    bcm_host_init();
    slog(INFO,LOG_TEXTURE,"Initializing broadcom hardware");
#endif
#ifndef DARWIN
    slog(INFO,LOG_TEXTURE,"Enable SDL2 verbose logging");
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
#endif

    dumpModes();

    if(!loadSDL()){
        slog(ERROR,LOG_CORE,"Failed to initialize SDL. Exit");
    }

    dumpSDLInfo();

    if( h > 0) {
      fontSize = h / 56;
    }
    if(!loadFont(BASE""FONT, fontSize)){
        exit(1);
    }
    if(pthread_create(&log_thr, NULL, &logListener, NULL) != 0){
       slog(ERROR,LOG_CORE,"Failed to create listener thread!");
       exit(1);
    }
    slog(INFO,LOG_CORE,"Screen size : %dx%d / Font size : %d",w,h,fontSize);

    processStartupScript(start);

    startupDone = true;

    while(!quit)
    {
       while(SDL_WaitEvent(&event) != 0)
       {
           if(event.type == SDL_CMD_EVENT)
           {
               slog(DEBUG,LOG_CORE,"New CMD event %s.",cmdStr);
               processCommand(cmdStr);
               free(cmdStr);
           }
           if(event.type == SDL_MOUSEBUTTONDOWN || 
               event.type == SDL_APP_TERMINATING || 
           //    event.type == SDL_KEYDOWN || 
               event.type == SDL_QUIT) {
                quit = true;
           }
           usleep(100000);
           // nanosleep(&eventDelay,NULL);
           showTexture(t1, rot);
           // slog(DEBUG,LOG_CORE,"%p %p",text,renderer);
       }
    }
    SDL_DestroyTexture(t1);
    if(t2) {
        SDL_DestroyTexture(t2);
    }
    closeSDL();
    return 0;
}

bool dumpSDLInfo()
{
	int i;
        printf("\nCheck SDL Window enabled flags:\n"); 
        int flags = SDL_GetWindowFlags(window);
        printf("    SDL_WINDOW_FULLSCREEN    [%c]\n", (flags & SDL_WINDOW_FULLSCREEN) ? 'X' : ' '); 
        printf("    SDL_WINDOW_OPENGL        [%c]\n", (flags & SDL_WINDOW_OPENGL) ? 'X' : ' '); 
        printf("    SDL_WINDOW_SHOWN         [%c]\n", (flags & SDL_WINDOW_SHOWN) ? 'X' : ' '); 
        printf("    SDL_WINDOW_HIDDEN        [%c]\n", (flags & SDL_WINDOW_HIDDEN) ? 'X' : ' '); 
        printf("    SDL_WINDOW_BORDERLESS    [%c]\n", (flags & SDL_WINDOW_BORDERLESS) ? 'X' : ' '); 
        printf("    SDL_WINDOW_RESIZABLE     [%c]\n", (flags & SDL_WINDOW_RESIZABLE) ? 'X' : ' '); 
        printf("    SDL_WINDOW_MINIMIZED     [%c]\n", (flags & SDL_WINDOW_MINIMIZED) ? 'X' : ' '); 
        printf("    SDL_WINDOW_MAXIMIZED     [%c]\n", (flags & SDL_WINDOW_MAXIMIZED) ? 'X' : ' '); 
        printf("    SDL_WINDOW_INPUT_GRABBED [%c]\n", (flags & SDL_WINDOW_INPUT_GRABBED) ? 'X' : ' '); 
        printf("    SDL_WINDOW_INPUT_FOCUS   [%c]\n", (flags & SDL_WINDOW_INPUT_FOCUS) ? 'X' : ' '); 
        printf("    SDL_WINDOW_MOUSE_FOCUS   [%c]\n", (flags & SDL_WINDOW_MOUSE_FOCUS) ? 'X' : ' '); 
        printf("    SDL_WINDOW_FOREIGN       [%c]\n", (flags & SDL_WINDOW_FOREIGN) ? 'X' : ' '); 

        // Allocate a renderer info struct 
        SDL_RendererInfo *rend_info = (SDL_RendererInfo *) malloc(sizeof(SDL_RendererInfo)); 
        if (!rend_info) { 
                slog(WARN, LOG_CORE, "Couldn't allocate memory for the renderer info data structure\n"); 
        } 
        // Print the list of the available renderers 
        printf("\nAvailable 2D rendering drivers:\n"); 
        for (i = 0; i < SDL_GetNumRenderDrivers(); i++) { 
                if (SDL_GetRenderDriverInfo(i, rend_info) < 0) { 
                        slog(WARN, LOG_CORE, "Couldn't get SDL 2D render driver information: %s\n", SDL_GetError()); 
                } 
                printf("%2d: %s\n", i, rend_info->name); 
                printf("    SDL_RENDERER_SOFTWARE     [%c]\n", (rend_info->flags & SDL_RENDERER_SOFTWARE) ? 'X' : ' '); 
                printf("    SDL_RENDERER_ACCELERATED  [%c]\n", (rend_info->flags & SDL_RENDERER_ACCELERATED) ? 'X' : ' '); 
                printf("    SDL_RENDERER_PRESENTVSYNC [%c]\n", (rend_info->flags & SDL_RENDERER_PRESENTVSYNC) ? 'X' : ' '); 
        } 

        // Print the name of the current rendering driver 
        if (SDL_GetRendererInfo(renderer, rend_info) < 0) { 
                slog(WARN, LOG_CORE, "Couldn't get SDL 2D rendering driver information: %s\n", SDL_GetError()); 
        } 
        printf("Rendering driver in use: %s\n", rend_info->name); 
        printf("    SDL_RENDERER_SOFTWARE     [%c]\n", (rend_info->flags & SDL_RENDERER_SOFTWARE) ? 'X' : ' '); 
        printf("    SDL_RENDERER_ACCELERATED  [%c]\n", (rend_info->flags & SDL_RENDERER_ACCELERATED) ? 'X' : ' '); 
        printf("    SDL_RENDERER_PRESENTVSYNC [%c]\n", (rend_info->flags & SDL_RENDERER_PRESENTVSYNC) ? 'X' : ' '); 
}

bool dumpModes()
{
    SDL_DisplayMode mode;
    SDL_Rect r;
    int j,i,display_count;
    Uint32 f;
    if ((display_count = SDL_GetNumVideoDisplays()) < 1) {
        slog(WARN,LOG_CORE,"No VideoDisplays found.");
        return false;
    }
    slog(INFO,LOG_CORE,"VideoDisplays: %i", display_count);

    for (j=0; j<display_count; j++){
        SDL_GetDisplayBounds(j,&r);
        slog(DEBUG,LOG_CORE,"Display %d boundaries : %d x %d",j,r.w,r.h);
        // Store size of first display
        if(j == 0){
            w = r.w;
            h = r.h;
        }
        for (i=0; i<SDL_GetNumDisplayModes(j); i++){
          SDL_GetDisplayMode(j,i,&mode);
          f = mode.format;
          slog(DEBUG,LOG_CORE,"Display %d / Mode %d : %d x %d x %d bpp (%s) @ %d Hz", j, i, mode.w, mode.h, SDL_BITSPERPIXEL(f), SDL_GetPixelFormatName(f), mode.refresh_rate);
        }
    }
    return true;
}
bool loadSDL()
{
    bool mode2d = false;
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        slog(ERROR, LOG_CORE, "SDL could not initialize! SDL Error: %s", IMG_GetError() );
	return false;
    }
    int flags=IMG_INIT_JPG|IMG_INIT_PNG;
    int initted=IMG_Init(flags);
    if((initted&flags) != flags)
    {
        slog(ERROR,LOG_CORE, "SDL_image could not initialize PNG and JPG! SDL_image Error: %s", IMG_GetError() );
	return false;
    }
    if ( TTF_Init() == -1 ) {
        slog(ERROR,LOG_CORE, "SDL_TTF could not initialize! SDL_ttf Error: %s", TTF_GetError() );
	return false;
    }
    SDL_ShowCursor( 0 );

#ifdef DARWIN
    window = SDL_CreateWindow("wallyd", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1440, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
#else
    window = SDL_CreateWindow("wallyd", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_GRABBED);
#endif
    
    SDL_ShowCursor( 0 );
    if(mode2d){
           screenSurface = SDL_GetWindowSurface( window );
    } else {
       //renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED| SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC );
       renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED| SDL_RENDERER_TARGETTEXTURE );
       if(renderer == NULL) {
            slog(ERROR, LOG_CORE, "Hardware accelerated renderer could not initialize : %s", IMG_GetError() );
            slog(WARN, LOG_CORE, "Falling back to software renderer.");
       	    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE );
       }

       if(renderer == NULL){
            slog(ERROR, LOG_CORE, "Renderer could not initialize : %s", SDL_GetError() );
            return false;
       }
    }
    SDL_GetRendererOutputSize(renderer, &w, &h);

    return true;
}

bool showTexture(SDL_Texture *tex1, int rot){
      //SDL_Rect r = { h-16, 0, h, w };
      SDL_Rect r = {0, 0, fontSize, w };
      int tw,th;
      SDL_Texture *tex2;
      if(logStr) {
         tex2 = renderLog(logStr,&r.w, &r.h);
         r.x = 1;
         r.y = h - r.h;
      }
      if(rot == 0){
       	        SDL_RenderCopy( renderer, tex1, NULL, NULL);
                if(tex2) {
       	            SDL_RenderCopy( renderer, tex2, NULL, &r);
                }
      } else {
    	        SDL_RenderCopyEx( renderer, tex1, NULL, NULL,rot, NULL,SDL_FLIP_NONE);
                if(tex2){
    	            SDL_RenderCopyEx( renderer, tex2, NULL, &r,rot, NULL,SDL_FLIP_NONE);
                }
      }
      SDL_RenderPresent( renderer );
      SDL_DestroyTexture( tex2 );
      return true;
}

bool fadeOver(SDL_Texture *t1, SDL_Texture *t2,int rot, long delay){
    struct timespec t = { 0, delay };
    int v = 0;
    int i = 0;
    SDL_Rect size;
    SDL_Texture *temp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetTextureBlendMode(t1, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(t2, SDL_BLENDMODE_BLEND);
    for(i = 255; i >= 0;i-=4){
         SDL_SetRenderTarget(renderer, temp);
    	 SDL_RenderCopyEx( renderer, t2, NULL, NULL,rot, NULL,SDL_FLIP_NONE);
         SDL_SetTextureAlphaMod(t1,i);
    	 SDL_RenderCopyEx( renderer, t1, NULL, NULL,rot, NULL,SDL_FLIP_NONE);
         SDL_SetRenderTarget(renderer, NULL);
         showTexture(temp, rot);
         nanosleep(&t,NULL);
    }
    SDL_DestroyTexture(temp);
    SDL_SetTextureAlphaMod(t1,255);
    return true;
}


bool fadeImage(SDL_Texture *text, int rot, bool reverse, long delay){
    struct timespec t = { 0, delay};
    //SDL_SetRenderDrawColor(renderer, 0,0,0, 0xFF);
    //SDL_RenderClear(renderer);
    int v = 0;
    int i = 0;
    for(i = 0; i < 255;i+=2){
       if(reverse){
          v = 255-i; 
       } else {
          v = i;
       }
     //    SDL_SetRenderDrawColor(renderer, i,i,i, 0xFF);
    //     SDL_RenderClear(renderer);
   //      SDL_RenderPresent( renderer );
   //      nanosleep(&t,NULL);
   //rot=i*360/255;
         //SDL_SetTextureBlendMode(text, SDL_BLENDMODE_BLEND);
         //SDL_SetTextureAlphaMod(text,i);
         SDL_SetTextureColorMod(text, v, v, v);
         showTexture(text, rot);
         nanosleep(&t,NULL);
    }
    return true;
}

SDL_Texture *loadImage(char *name)
{
    bool mode2d = false;
    bool success = true;
    SDL_Surface* image = NULL;
    SDL_Texture* text = NULL;

    if(mode2d == true){
        image = IMG_Load( name );
        if( image == NULL )
        {
            slog(ERROR, LOG_CORE, "Unable to load image %s! SDL Error: %s", name, SDL_GetError() );
            return false;
        }
        SDL_Surface *optimizedSurface = SDL_ConvertSurface( image, screenSurface->format, 0 );

        if(SDL_BlitScaled( optimizedSurface, NULL, screenSurface, NULL )){
            slog(ERROR, LOG_CORE, "Unable to blit image %s! SDL Error: %s", name, SDL_GetError() );
            return false;
        }
        SDL_UpdateWindowSurface( window );
    } else {
    
       SDL_Rect rect={0,0,0,0};
       text = IMG_LoadTexture(renderer,name);
       if(text == NULL){
           slog(ERROR, LOG_CORE, "Error loading image : %s",IMG_GetError());
           return false;
       }
       SDL_SetTextureBlendMode(text, SDL_BLENDMODE_BLEND);
       //if(rot == 0){
       //		SDL_RenderCopy( renderer, text, NULL, NULL);
       //} else {
       //		SDL_RenderCopyEx( renderer, text, NULL, NULL,rot, NULL,SDL_FLIP_NONE);
       //}
       //SDL_DestroyTexture(text);
       //SDL_RenderPresent( renderer );
    }
    return text;
}

bool loadFont(char *file, int size){
   font = TTF_OpenFont( file, size );
   if ( font == NULL ) {
      slog(ERROR, LOG_CORE, "Failed to load font : %s ",TTF_GetError());
      return false;
   } else {
      return true;
   }
}


void closeSDL()
{
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    window = NULL;
    renderer = NULL;
    SDL_Quit();
}

SDL_Texture* renderLog(char *str,int *w, int *h)
{
   SDL_Rect dest;
   SDL_Surface *rsurf,*surf;
   SDL_Texture *text;
   SDL_Color *c = malloc(sizeof(SDL_Color));

   hexToColor(color,c);

   surf = TTF_RenderUTF8_Blended( font, str, *c );
   text = SDL_CreateTextureFromSurface( renderer, surf );
   TTF_SizeUTF8(font, str, w, h);

   // SDL_QueryTexture( text, NULL, NULL, w, h );
   SDL_FreeSurface( surf );
   free(c);
   return text;
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
            if(strncmp(lineCopy, "quit", 4) == 0){
                kill(getpid(),SIGINT);
            }
            char *myCmd = strsep(&lineCopy, " ");
            if(strcmp(myCmd,"nice") == 0){
              niceing = true;
            }
            if(strcmp(myCmd,"fadein") == 0){
                char *delayStr = strsep(&lineCopy, " ");
                char *file  = strsep(&lineCopy, " ");
                long delay = atol(delayStr);
                slog(DEBUG,LOG_CORE,"Fadein %s with delay %u",file, delay);
                if(file && delay) {
                    t1 = loadImage(file);
                    fadeImage(t1, rot, false, delay * 1000);
                } else {
                    slog(DEBUG,LOG_CORE,"fadein <delay> <file>");
                }
            }
            else if(strcmp(myCmd,"fadeover") == 0){
                char *delayStr = strsep(&lineCopy, " ");
                char *file  = strsep(&lineCopy, " ");
                long delay = atol(delayStr);
                slog(DEBUG,LOG_CORE,"Fadeover %s with delay %u",file, delay);
                if(file && delay) {
                    t2 = loadImage(file);
                    fadeOver(t1, t2, rot, delay * 1000);
                    SDL_DestroyTexture(t1);
                    t1 = t2;
                } else {
                    slog(DEBUG,LOG_CORE,"fadeover <delay> <file>");
                }
            }
            else if(strcmp(myCmd,"fadeloop") == 0){
                char *loopStr = strsep(&lineCopy, " ");
                char *delayStr = strsep(&lineCopy, " ");
                char *fileA  = strsep(&lineCopy, " ");
                char *fileB  = strsep(&lineCopy, " ");
                long delay = atol(delayStr);
                long loop = atol(loopStr);
                slog(DEBUG,LOG_CORE,"Fadeloop %d times from  %s to %s with delay %u",loop, fileA, fileB, delay);
                if(fileA && fileB && loop && delay) {
                    t1 = loadImage(fileA);
                    t2 = loadImage(fileB);
                    for(i = 0; i < loop; i++){
                        fadeOver(t1, t2, rot, delay * 1000);
                        fadeOver(t2, t1, rot, delay * 1000);
                    }
                    fadeOver(t1, t2, rot, delay * 1000);
                    SDL_DestroyTexture(t1);
                    t1 = t2;
                } else {
                    slog(DEBUG,LOG_CORE,"fadeloop <num> <delay> <fileA> <fileB>");
                }
            }
            else if(strcmp(myCmd,"fadeout") == 0){
                char *delayStr = strsep(&lineCopy, " ");
                long delay = 4500000;
                if(delayStr != NULL) {
                    delay = atol(delayStr);
                }
                slog(DEBUG,LOG_CORE,"Fadeout with delay %u",delay);
                if(delay) {
                    fadeImage(t1, rot, true, delay * 1000);
                } else {
                    slog(DEBUG,LOG_CORE,"fadeout <delay>");
                }
                SDL_DestroyTexture(t1);
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
            else if(strcmp(myCmd,"color") == 0){
                free(color);
                color = strdup(strsep(&lineCopy, " "));
                slog(DEBUG,LOG_CORE,"Set color to %s", color);
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

void processStartupScript(char *file){
  slog(DEBUG,LOG_CORE,"Reading wallystart config : %s",file);
  long fsize=0;
  char *cmds=NULL;

  FILE *f = fopen(file, "rb");
  if(!f){
      slog(DEBUG,LOG_CORE,"File not found. Not running any startup commands");
      return;
  }

  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  cmds = malloc(fsize + 1);
  fread(cmds, fsize, 1, f);
  fclose(f);

  cmds[fsize] = 0;
  slog(DEBUG,LOG_CORE,"Processing %d bytes from startupScript",fsize);
  processCommand(cmds);
  free(cmds);
}

void sig_handler(int signo){
    if (signo == SIGINT) {
        quit = true;
        exit(1);
    }
}

void hexToColor(char* colStr, SDL_Color *c)
{
   int color = strtol(colStr,NULL,16);
   c->r = (color >> 16) & 0xff;
   c->g = (color >> 8)  & 0xff;
   c->b = color & 0xff;
   c->a = 0;
}
