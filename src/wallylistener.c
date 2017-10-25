#include <wallystart.h>

extern char *answer;

char *repl_str(const char *str, const char *from, const char *to);

#ifndef WALLYPIXEL
extern bool startupDone;
extern char *logStr;
extern char *cmdStr;

void handleCommand(char *str) {
    SDL_Event sdlevent;

    char *oldStr;
    // Handle some basic commands while in startup mode
    if (startupDone == false) {
        if (strncmp(str, "quit",4) == 0){
	    kill(getpid(),SIGINT);
	}
        if (strncmp(str, "log ",4) == 0){
            slog(DEBUG, LOG_CORE, "Found log command in startup mode.");
            // Avoid nullpointer in the other thread
            oldStr = logStr;
            logStr = strndup(str+4, strlen(str) - 4);
            free(str);
	        if (oldStr) {
	            free(oldStr);
	        }
        }
    // The main proc should now use processCommand
    } else {
        cmdStr = str;
        sdlevent.type = SDL_CMD_EVENT;
        SDL_PushEvent(&sdlevent);
    }
}
#endif

void* logListener(void *ptr){
   int i = 0;
   int sockfd, newsockfd;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   int  n;
   char *buf;
   char *oldStr = NULL;
   char *tmpStr = NULL;
   int optval = 1;
   
   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if (sockfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   
   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(bindPort);

   setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
   
   /* Now bind the host address using bind() call.*/
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR on binding");
      exit(1);
   }
      
   listen(sockfd,5);
   clilen = sizeof(cli_addr);
   
   while (1) {
       newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
       if (newsockfd < 0) {
           perror("ERROR on accept");
           return NULL;
       }
  
       while ( 1 ) {
	       n = sgetline(newsockfd, &buf);
           //bzero(buffer,256);
           //n = read( newsockfd,buffer,255 );
           
           if (n < 0) {
              close(newsockfd);
              break;
           }
           // n = write(newsockfd,buf,n);
	       if(answer == NULL) {
	          write(newsockfd, buf, n);
	       } else {
	          write(newsockfd, answer, strlen(answer));
	       }
            
#ifndef WALLYPIXEL
           handleCommand(strndup(buf,n-1));
#else
           logStr = strndup(buf,n-1);
#endif
           free(buf);

           close(newsockfd);
           break;
       }
   }
   return NULL;
}

int sgetline(int fd, char ** out)
{
    int buf_size = 1024;
    int bytesloaded = 0;
    int ret;
    char chr;
    char * buf = malloc(buf_size);
    char * newbuf;

    if (NULL == buf){
        return -1;
    }

    bzero(buf,buf_size);

    while ( 1 )
    {
        // read a single byte
        ret = read(fd, &chr, 1);
        if (ret < 1)
        {
            // error or disconnect
            free(buf);
            return -1;
        }

        buf[bytesloaded] = chr;
        bytesloaded++;

        // has end of line been reached?
        if (chr == '\n')
            break; // yes

        // is more memory needed?
        if (bytesloaded >= buf_size)
        {
            buf_size += 128;
            newbuf = realloc(buf, buf_size);

            if (NULL == newbuf)
            {
                free(buf);
                return -1;
            }

            buf = newbuf;
        }
    }

    // if the line was terminated by "\r\n", ignore the
    // "\r". the "\n" is not in the buffer
    if ((bytesloaded) && (buf[bytesloaded-1] == '\r'))
        bytesloaded--;

    *out = buf; // complete line
    return bytesloaded; // number of bytes in the line, not counting the line break
}

char *repl_str(const char *str, const char *from, const char *to) {

	/* Adjust each of the below values to suit your needs. */

	/* Increment positions cache size initially by this number. */
	size_t cache_sz_inc = 16;
	/* Thereafter, each time capacity needs to be increased,
	 * multiply the increment by this factor. */
	const size_t cache_sz_inc_factor = 3;
	/* But never increment capacity by more than this number. */
	const size_t cache_sz_inc_max = 1048576;

	char *pret, *ret = NULL;
	const char *pstr2, *pstr = str;
	size_t i, count = 0;
	#if (__STDC_VERSION__ >= 199901L)
	uintptr_t *pos_cache_tmp, *pos_cache = NULL;
	#else
	ptrdiff_t *pos_cache_tmp, *pos_cache = NULL;
	#endif
	size_t cache_sz = 0;
	size_t cpylen, orglen, retlen, tolen, fromlen = strlen(from);

	/* Find all matches and cache their positions. */
	while ((pstr2 = strstr(pstr, from)) != NULL) {
		count++;

		/* Increase the cache size when necessary. */
		if (cache_sz < count) {
			cache_sz += cache_sz_inc;
			pos_cache_tmp = realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
			if (pos_cache_tmp == NULL) {
				goto end_repl_str;
			} else pos_cache = pos_cache_tmp;
			cache_sz_inc *= cache_sz_inc_factor;
			if (cache_sz_inc > cache_sz_inc_max) {
				cache_sz_inc = cache_sz_inc_max;
			}
		}

		pos_cache[count-1] = pstr2 - str;
		pstr = pstr2 + fromlen;
	}

	orglen = pstr - str + strlen(pstr);

	/* Allocate memory for the post-replacement string. */
	if (count > 0) {
		tolen = strlen(to);
		retlen = orglen + (tolen - fromlen) * count;
	} else	retlen = orglen;
	ret = malloc(retlen + 1);
	if (ret == NULL) {
		goto end_repl_str;
	}

	if (count == 0) {
		/* If no matches, then just duplicate the string. */
		strcpy(ret, str);
	} else {
		/* Otherwise, duplicate the string whilst performing
		 * the replacements using the position cache. */
		pret = ret;
		memcpy(pret, str, pos_cache[0]);
		pret += pos_cache[0];
		for (i = 0; i < count; i++) {
			memcpy(pret, to, tolen);
			pret += tolen;
			pstr = str + pos_cache[i] + fromlen;
			cpylen = (i == count-1 ? orglen : pos_cache[i+1]) - pos_cache[i] - fromlen;
			memcpy(pret, pstr, cpylen);
			pret += cpylen;
		}
		ret[retlen] = '\0';
	}

end_repl_str:
	/* Free the cache and return the post-replacement string,
	 * which will be NULL in the event of an error. */
	free(pos_cache);
	return ret;
}
