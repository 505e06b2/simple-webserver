#  Makefile for server
all:	src/server.c
	gcc src/server.c -Isrc -o server
	
.PHONY:	run wkill lkill

wkill:
	taskkill /f /im server.exe
	
lkill:
	killall server

run: all
	./server 8080 ../htdocs