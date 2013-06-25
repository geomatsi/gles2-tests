#
# simple make file
#

LIB_GLES2=-lGLESv2
LIB_EGL=-lEGL
LIB_X11=-lX11

all: test-x11

test-x11: main.o shader_utils.o plat_x11.o
	gcc -o $@ $^ ${LIB_GLES2} ${LIB_EGL} ${LIB_X11}

test-vivante: main.o shader_utils.o plat_vivante.o
	gcc -o $@ $^ ${LIB_GLES2} ${LIB_EGL}

%.o: %.c
	gcc -c -o $@ $^

clean:
	rm -rf *.o
	rm -rf test-x11 test-fbdev
