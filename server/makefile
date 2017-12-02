#  Makefile for server
all:	server.c
	gcc server.c -o server
	
.PHONY:	run wkill lkill

wkill:
	taskkill /f /im server.exe
	
lkill:
	killall server

run: all
	./server 8080 ../htdocs