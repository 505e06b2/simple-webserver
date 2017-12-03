#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h> //FOR APPENDLOG
#include <time.h> //For Logging

#define BUFSIZE 4096
#define BUFLOG 128
#define ERROR 42
#define SORRY 43
#define LOG   44
#define BEGIN 41

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif",		"image/gif"},  
	{"jpg",		"image/jpeg"}, 
	{"jpeg",	"image/jpeg"},
	{"png",		"image/png"},  
	{"zip",		"image/zip"},  
	{"gz", 		"image/gz"},  
	{"tar",		"image/tar"},  
	{"htm",		"text/html"},  
	{"html",	"text/html"}, 
	{"cgi",		"executable"}, 
	{"xml",		"text/xml"},  
	{"js",		"text/js"},
	{"css",		"test/css"},
	{"mp4",		"video/mp4"},
	{"webm",	"video/webm"},
	{"ogg",		"audio/ogg"},
	{NULL,			NULL}
};

char logAbsolute[128]; //Increase this if there are write/read errors

int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

char *formatLog(const char icon, const char *text) { //Must free malloc.
	char *out = (char *)malloc(strlen(text) + sizeof("[ ] [00:00:00]: \n") + 1); //+ size of the text and \n and \0
	char timestring[9];
	const time_t rawtime = time(NULL);
	
	strftime(timestring, sizeof(timestring), "%T", localtime(&rawtime));
	sprintf(out, "[%c] [%s]: %s\n", icon, timestring, text);
	return out;
}

void appendLog(int type, const char *fmt, ...) {
	int logFile;
	char formatLogbuffer[BUFLOG];
	char *appendLog;
	char *printError;
	
	va_list args;
    va_start(args, fmt);
	vsprintf(formatLogbuffer, fmt, args);
	va_end(args);

	switch (type) {
		case ERROR:
			appendLog = formatLog('#', formatLogbuffer);
			break;
		case SORRY:
			appendLog = formatLog('!', formatLogbuffer); //Write to file
			printError = malloc(sizeof(formatLogbuffer) + 108); //108 for the HTML below
			sprintf(printError, "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>Web Server Error: %s</H1></BODY></HTML>\r\n", formatLogbuffer); //404 just so the browser knows it's a bad boi page
			//write(ioout, appendLogbuffer, strlen(appendLogbuffer)); //Write to browser
			puts(printError); //Fuck it, this makes things nicer but probably slower but that doesn't matter since it's going to close
			free(printError);
			break;
		case BEGIN:
			remove(logAbsolute); //No break so we can continue with the log
		case LOG:
			appendLog = formatLog('*', formatLogbuffer);
			break;
	}
	
	if((logFile = open(logAbsolute, O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
		write(logFile, appendLog, strlen(appendLog));
		close(logFile);
	}
	free(appendLog); //Free the malloc
	if(type != LOG && type != BEGIN) exit(3); //if it's fucked; close request
}
