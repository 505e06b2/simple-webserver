//Original Author: Oscar Sanchez (oms1005@gmail.com)

/*
 *    ███████╗ ██████╗ ███████╗███████╗ ██████╗  ██████╗ ██████╗ ██████╗ 
 *    ██╔════╝██╔═████╗██╔════╝██╔════╝██╔═████╗██╔════╝ ██╔══██╗╚════██╗
 *    ███████╗██║██╔██║███████╗█████╗  ██║██╔██║███████╗ ██████╔╝ █████╔╝
 *    ╚════██║████╔╝██║╚════██║██╔══╝  ████╔╝██║██╔═══██╗██╔══██╗██╔═══╝ 
 *    ███████║╚██████╔╝███████║███████╗╚██████╔╝╚██████╔╝██████╔╝███████╗
 *    ╚══════╝ ╚═════╝ ╚══════╝╚══════╝ ╚═════╝  ╚═════╝ ╚═════╝ ╚══════╝
 */

#include "server.h"
#include "request.h"

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
				request(socketfd, hit);
			} else {
				close(socketfd);
			}
		}
	}
}
