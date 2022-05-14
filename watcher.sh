#!/bin/sh
while true; do
	inotifywait ./xbacklight.c
	sleep 0.3
	cc -g ./xbacklight.c -o xbacklight -rdynamic
	sleep 1
	done
