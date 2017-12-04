void request(int fileid, int request) {
	int j, file_fd, len;
	long i, ret;
	char *fstr, *bufferFile;
	static char buffer[BUFSIZE+1];
	char ip[] = "255.255.255.255:65535"; //Can fit localhost too
	char get[256]; //256 chars long should be OK unless it's in UTF8...

	ret = read(fileid, buffer, BUFSIZE); 
	if(ret <= 0) {
		appendLog(SORRY, "Failed to read browser request - Error: %d", ret);
	} else if(ret > 0 && ret < BUFSIZE)	
		buffer[ret] = 0;
	else buffer[0] = 0;

	if(strncasecmp(buffer, "GET ", 4) != 0) appendLog(SORRY, "Only simple GET operation supported: %s", buffer);
	
	for(i = 0; i < ret; i++) if(buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = '\0'; //Split request in buffer

	for(i = 4; i < sizeof(get) + sizeof(ip) + 5; i++) { //I only need up to the host which is < 256
		if(buffer[i] == ' ') { 
			buffer[i] = 0;
			break;
		}
	}
	
	strcpy(get, buffer+5); //Fill GET with path
	memcpy(ip, buffer+strlen(buffer)+17, sizeof(ip)-1); //Get IP but make sure there's always a \0 at the end
	bufferFile = buffer+5;
	
	appendLog(LOG, "Get /%s <- %s", get, ip); //Buffer = "GET /[file]"; strlen(buffer)+2 = "HTTP 1.1"; strlen(buffer)+11 = "Host: [ip]"
	
	len = strlen(get);
	if(len == 0 || get[len-1] == '/') memcpy(bufferFile+len, "index.html", sizeof("index.html")); //Default to index.html if there's a slash at the end of a get request
	else if(isDirectory(get)) { //Redirect to slash
		len += strlen(ip) + 90-5; //90 is the http stuff -5 for formatting and removing \0
		char *redirect = malloc(len);
		sprintf(redirect, "HTTP/1.0 301 Moved Permanently\r\nLocation: http://%s/%s/\r\n", ip, get);
		write(fileid, redirect, len);
		exit(1);
	}

	if((file_fd = open(bufferFile, O_RDONLY)) == -1) appendLog(SORRY, "Failed to open file: %s", get);

	for(i = 0, j = 0, fstr = NULL, len = strlen(bufferFile); extensions[i].ext != 0; i++) {
		j = strlen(extensions[i].ext);
		if( !strncmp(bufferFile + (len-j), extensions[i].ext, j)) {
			fstr = extensions[i].filetype;
			break;
		}
	}
	
	if(fstr == NULL) {
		close(file_fd);
		appendLog(SORRY, "File extension type not supported: %s", buffer); //File not legit (exe / no ext / php)
	}
	
	if( strncmp("executable", fstr, sizeof("executable")-1 ) == 0) { //=== YOU NEED TO SET THE CONTENT TYPE YOURSELF ===
		close(file_fd);
		write(fileid, "HTTP/1.0 200 OK\r\n", sizeof("HTTP/1.0 200 OK\r\n")-1);
		buffer[3] = '.';
		FILE *command = popen(buffer+3, "rb");
		if(!command) appendLog(SORRY, "Failed to execute: %s", buffer+3);
		memset(buffer, 0, BUFSIZE+1);
		fread(buffer, sizeof(char), BUFSIZE, command);
		write(fileid, buffer, strlen(buffer));
		appendLog(LOG, "%s finished with exit code: %d", get, pclose(command));
		
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