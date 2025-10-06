CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
AR = ar

libtouch.a: touch_framework.o
	$(AR) rcs libtouch.a touch_framework.o

touch_framework.o: touch_framework.c touch_framework.h
	$(CC) $(CFLAGS) -c touch_framework.c -o touch_framework.o

clean:
	rm -f touch_framework.o libtouch.a
