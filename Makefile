#PROGRAMS = test
#GST = glib-2.9 gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0
#SDL = sdl2-config
#MODULES = $(GST) $(SDL)
#CFLAGS = -g -Wall -Wextra `pkg-config --cflags $(MODULES)`
#all: $(PROGRAMS)
#clean:
#	-rm -f $(PROGRAMS)
#.PHONY: all clean


GST=`pkg-config --cflags --libs glib-2.0  gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0`
SDL=`sdl2-config --libs`

pre: pre.c
	$(CC) -Wall -Wextra pre.c $(SDL) $(GST) -std=c++11 $(LDFLAGS)
