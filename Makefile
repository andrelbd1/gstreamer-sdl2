MODULES= glib-2.0 gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 sdl2
CFLAGS= `pkg-config --cflags $(MODULES)`
LDFLAGS= `pkg-config --libs $(MODULES)`
pre: pre.c
