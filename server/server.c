/*
 * project:     miniweb
 * author:      Oscar Sanchez (oms1005@gmail.com)
 * HTTP Server
 * WORKS ON BROWSERS TOO!
 * Inspired by IBM's nweb
*/

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
	{0,			0}
};

char logAbsolute[128];//Increase this if there are write/read errors
int parentPid;

char *formatLog(const char icon, const char *text) { //Must free malloc.
	char *out = (char *)malloc(strlen(text) + sizeof("[ ] [00:00]: \n")); //+ size of the text and \n and \0
	char timestring[6];
	const time_t rawtime = time(NULL);
	
	strftime(timestring, sizeof(timestring), "%H:%S", localtime(&rawtime));
	sprintf(out, "[%c] [%s]: %s\n", icon, timestring, text);
	return out;
}

void appendLog(int type, const char *fmt, ...) {
	int logFile;
	char formatLogbuffer[BUFLOG];
	char *appendLog;
	
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
			//sprintf(formatLogbuffer, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>Web Server Error: %s</H1></BODY></HTML>\r\n", formatLogbuffer);
			//write(ioout, appendLogbuffer, strlen(appendLogbuffer)); //Write to browser
			puts(formatLogbuffer); //Fuck it, this makes things nicer but probably slower but that doesn't matter since it's going to close
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

void web(int fileid, int request) {
	int j, file_fd, buflen, len;
	long i, ret;
	char *fstr;
	static char buffer[BUFSIZE+1];

	ret = read(fileid, buffer, BUFSIZE); 
	if(ret <= 0) {
		appendLog(SORRY, "Failed to read browser request - Error: %d", ret);
	} else if(ret > 0 && ret < BUFSIZE)	
		buffer[ret] = 0;
	else buffer[0] = 0;

	for(i = 0; i < ret; i++) if(buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = '\0'; //Split request in buffer

	if( strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4) ) appendLog(SORRY, "Only simple GET operation supported: %s", buffer);

	for(i = 4; i < BUFSIZE; i++) { 
		if(buffer[i] == ' ') { 
			buffer[i] = 0;
			break;
		}
	}
	
	appendLog(LOG, "%s <- %s", buffer, buffer+strlen(buffer)+17); //Buffer = "GET /[file]"; strlen(buffer)+2 = "HTTP 1.1"; strlen(buffer)+11 = "Host: [ip]"

	for(j = 0; j < i-1; j++) if(buffer[j] == '.' && buffer[j+1] == '.') appendLog(SORRY, "Parent directory (..) path names not supported: %s", buffer);

	if( !strncmp(&buffer[0],"GET /\0",6) || !strncmp(&buffer[0],"get /\0",6) ) strcpy(buffer,"GET /index.html"); //Default to index.html

	buflen = strlen(buffer);
	fstr = 0;
	for(i=0;extensions[i].ext != 0;i++) {
		len = strlen(extensions[i].ext);
		if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
			fstr = extensions[i].filetype;
			break;
		}
	}
	
	if(fstr == 0) appendLog(SORRY, "File extension type not supported: %s", buffer); //File not legit (exe / no ext / php)

	if((file_fd = open(&buffer[5], O_RDONLY)) == -1) appendLog(SORRY, "Failed to open file: %s", buffer+5);
	
	if( strncmp("executable", fstr, sizeof("executable")-1 ) == 0) { //=== YOU NEED TO SET THE CONTENT TYPE YOURSELF ===
		close(file_fd);
		write(fileid, "HTTP/1.0 200 OK\r\n", strlen("HTTP/1.0 200 OK\r\n"));
		buffer[3] = '.';
		FILE *command = popen(buffer+3, "rb");
		if(!command) appendLog(SORRY, "Failed to execute: %s", buffer+3);
		memset(buffer, 0, BUFSIZE+1);
		fread(buffer, sizeof(char), BUFSIZE, command);
		write(fileid, buffer, strlen(buffer));
		appendLog(LOG, "Program finished with exit code: %d", pclose(command));
		
	} else { //read file and output to browser
		//appendLog(LOG, "SEND %s", buffer+5); //Don't need to log that we're trying to send; just going to imply
		sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
		write(fileid, buffer, strlen(buffer));

		while ((ret = read(file_fd, buffer, BUFSIZE)) > 0) write(fileid, buffer, ret);
		close(file_fd);
	}
#if defined(LINUX) || defined(__CYGWIN__)
	//sleep(0.3); //Less refresh spamming
#endif
	exit(1);
}

int main(int argc, char **argv) {
	int i, port, pid, listenfd, socketfd;
	unsigned long long hit; //All it does is count the requests...
	socklen_t length;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;

	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		printf("usage: server [port] [server directory] &"
					 "\tExample: server 80 ./ &\n\n"
					 "\tOnly Supports:");
		for(i = 0; extensions[i].ext != 0; i++) printf(" %s", extensions[i].ext);

		printf("\n\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n");
		exit(0);
	}
	if( !strncmp(argv[2], "/"   , 2 ) || !strncmp(argv[2], "/etc", 5 ) ||
	    !strncmp(argv[2], "/bin", 5 ) || !strncmp(argv[2], "/lib", 5 ) ||
	    !strncmp(argv[2], "/tmp", 5 ) || !strncmp(argv[2], "/usr", 5 ) ||
	    !strncmp(argv[2], "/dev", 5 ) || !strncmp(argv[2], "/sbin",6) ) {
			printf("ERROR: Bad top directory %s, see server -?\n", argv[2]);
			exit(3);
	}
	
	//Set absolute path for log file or it becomes a security concern since anyone can read it
	if(getcwd(logAbsolute, sizeof(logAbsolute)) == NULL) {
		printf("ERROR: Can't get current working directory into %dbytes...\n", sizeof(logAbsolute));
		exit(4);
	} else {
		strcat(logAbsolute, "/server.log");
	}

	if(chdir(argv[2]) == -1) { 
		printf("ERROR: Can't Change to directory %s\n", argv[2]);
		exit(4);
	}

	if(fork() != 0) return 0; //This kills the parent so stdio is dead and we don't block any more
	signal(SIGCHLD, SIG_IGN); //Ignore when children close
	signal(SIGHUP, SIG_IGN); //Ignore if a process hangs
	for(i = 0; i < 32; i++) close(i);
	setpgrp(); //This makes a new session and removes access to stdio

	appendLog(BEGIN, "STARTING - Port: %s", argv[1]);

	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) appendLog(ERROR, "System call: socket");

	port = atoi(argv[1]);
	if(port < 0 || port > 60000) appendLog(ERROR, "Invalid port number try [1,60000] - %s", argv[1]);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	
	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) appendLog(ERROR, "System call: bind");
	if(listen(listenfd, 64) < 0) appendLog(ERROR, "System call: listen");
	
	length = sizeof(cli_addr); //This was inside the loop but it gets set the same value every time?
	for(hit = 1; ; hit++) { //Forever
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0) appendLog(ERROR, "System call: accept");

		if((pid = fork()) < 0) appendLog(ERROR, "System call: fork");
		else {
			if(pid == 0) {
				close(listenfd);
				web(socketfd, hit);
			} else {
				close(socketfd);
			}
		}
	}
}
