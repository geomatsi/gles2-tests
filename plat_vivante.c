#include "plat.h"

/* */

EGLNativeDisplayType native_display;
EGLNativeWindowType native_window;

/* */

int platform_open(void)
{
	native_display = fbGetDisplayByIndex(0);
	native_window = fbCreateWindow(native_display, 0, 0, 0, 0);
}

void platform_close(void)
{

}

EGLNativeDisplayType plat_get_display(void)
{
	return native_display;
}

EGLNativeWindowType plat_get_window(void)
{
	return native_window;
}
