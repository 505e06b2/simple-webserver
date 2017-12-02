#include <stdio.h>
#include <time.h>

int main() {
	time_t rawtime = time(NULL);
	char output[64];
	strftime(output, 64, "%T", localtime(&rawtime));
	printf("Content-Type: text/html\r\n\r\nServer's localtime: %s<br>Written in C", output);
	return 0;
}