#!/bin/bash

printf "Content-Type: text/html\r\n\r\nServer's localtime: %s<br>Written in Bash" `date "+%T"`