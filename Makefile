#
# simple make file
#

LIB_GLES2=-lGLESv2
LIB_EGL=-lEGL
LIB_X11=-lX11
LIB_MISC=-lm

all: test-x11

test-x11: main.o shader_utils.o plat_x11.o
	${CC} -o $@ $^ ${LIB_GLES2} ${LIB_EGL} ${LIB_X11} ${LIB_MISC} ${LDFLAGS}

test-vivante: main.o shader_utils.o plat_vivante.o
	${CC} -o $@ $^ ${LIB_GLES2} ${LIB_EGL} ${LIB_MISC} ${CFLAGS}

%.o: %.c
	${CC} -c -o $@ $^

clean:
	rm -rf *.o
	rm -rf test-x11 test-vivante
