#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include "shader_utils.h"
#include "plat.h"

/* */

#define WIDTH  640
#define HEIGHT 480

/* */

EGLNativeDisplayType nativeDisplay;
EGLNativeWindowType  nativeWindow;

static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLContext eglContext = EGL_NO_CONTEXT;
static EGLSurface eglWindowSurface = EGL_NO_SURFACE;
static EGLConfig eglConfig;

static GLuint ShaderProgs;
static GLint positionLoc;
static GLint texCoordLoc;
static GLint rtmatrixLoc;
static GLuint textureId;

static GLfloat angle = 0.0;
static GLfloat delta = 0.01;

#define TW 128
#define TH 128

GLuint bitmap[TW][TH];

/* */

static void handle_egl_error(const char *name)
{
    static char * const error_strings[] = {
        "EGL_SUCCESS",
        "EGL_NOT_INITIALIZED",
        "EGL_BAD_ACCESS",
        "EGL_BAD_ALLOC",
        "EGL_BAD_ATTRIBUTE",
        "EGL_BAD_CONFIG",
        "EGL_BAD_CONTEXT",
        "EGL_BAD_CURRENT_SURFACE",
        "EGL_BAD_DISPLAY",
        "EGL_BAD_MATCH",
        "EGL_BAD_NATIVE_PIXMAP",
        "EGL_BAD_NATIVE_WINDOW",
        "EGL_BAD_PARAMETER",
        "EGL_BAD_SURFACE"
    };

    EGLint error_code=eglGetError();

    fprintf(stderr," %s: egl error \"%s\" (0x%x)\n", name,
            error_strings[error_code-EGL_SUCCESS], error_code);

    exit(EXIT_FAILURE);
}

static int eglOpen(void)
{

    EGLint majorVersion;
    EGLint minorVersion;
    EGLint numConfigs;
    EGLBoolean ret;

    const char *ver, *extensions;

    int k = 0, i = 0, fnd = 0;

    EGLint gl_context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

#if 0
    EGLint pi32ConfigAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 5,
        EGL_GREEN_SIZE, 6,
        EGL_BLUE_SIZE, 5,
        EGL_ALPHA_SIZE, EGL_DONT_CARE,
        EGL_NONE
    };

    EGLint pi32ConfigAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
#else
    EGLint pi32ConfigAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
#endif

    eglDisplay = eglGetDisplay(nativeDisplay);
    if (!eglDisplay)
    {
        printf("eglGetDisplay error\n");
        return -1;
    }

    if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion))
    {
        printf("eglInitialize failed\n");
        return -1;
    }

    ver = eglQueryString(eglDisplay, EGL_VERSION);
    printf("EGL_VERSION = %s\n", ver);

    extensions = eglQueryString(eglDisplay, EGL_EXTENSIONS);
    printf("EGL_EXTENSIONS: %s\n", extensions);

    eglBindAPI(EGL_OPENGL_ES_API);

    if (!eglChooseConfig(eglDisplay, pi32ConfigAttribs, &eglConfig, 1, &numConfigs) || numConfigs != 1)
    {
		handle_egl_error("eglChooseConfig");
        return -1;
    }

    eglWindowSurface = eglCreateWindowSurface(eglDisplay, eglConfig, nativeWindow, NULL);
    if (eglWindowSurface == EGL_NO_SURFACE)
    {
        handle_egl_error("eglCreateWindowSurface");
        return -1;
    }

    eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, gl_context_attribs);
    if (eglContext == EGL_NO_CONTEXT)
    {
        handle_egl_error("eglCreateContext");
        return -1;
    }

    ret = eglMakeCurrent(eglDisplay, eglWindowSurface, eglWindowSurface, eglContext);
    if (ret == EGL_FALSE)
    {
        handle_egl_error("eglMakeCurrent");
        return -1;
    }

    return 0;
}

static void eglClose(void)
{
    eglMakeCurrent(eglDisplay, NULL, NULL, NULL);
    eglDestroyContext(eglDisplay, eglContext);
    eglDestroySurface(eglDisplay, eglWindowSurface);
    eglTerminate(eglDisplay);
}

static GLuint LoadShader(const char *shaderSrc, GLenum type)
{
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);

    if (shader == 0)
        return 0;

    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        GLint infoLen=0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen>0)
        {
            char *infoLog = malloc(sizeof(char) * infoLen);

            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            fprintf(stderr,"Error compiling shader:\n%s\n", infoLog);
            free(infoLog);

            glDeleteShader(shader);
            return 0;
        }
    }

    return shader;
}

static int initShaders(void)
{
    char *vShaderStr;
    char *fShaderStr;

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

	/* load shaders code from files */
	fShaderStr = shader_fread(FRAGMENT_SHADER);

	if (!fShaderStr) {
		printf("failed to load %s shader\n", FRAGMENT_SHADER);
		return 0;
	}

	vShaderStr = shader_fread(VERTEX_SHADER);

	if (!vShaderStr) {
		printf("failed to load %s shader\n", VERTEX_SHADER);
		return 0;
	}

    /* load vertex/frag shader */
    vertexShader = LoadShader(vShaderStr, GL_VERTEX_SHADER);
    fragmentShader = LoadShader(fShaderStr, GL_FRAGMENT_SHADER);

    /* create the program object */
    programObject = glCreateProgram();

    if (programObject == 0)
        return 0;

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    /* bind vPosition to attrib 0 */
    glBindAttribLocation(programObject, 0, "vPosition");
	glBindAttribLocation(programObject, 1, "myUV");

    /* linkage */
    glLinkProgram(programObject);

    /* check status */
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        GLint infoLen=0;

        glGetShaderiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen>0)
        {
            char *infoLog = malloc(sizeof(char) * infoLen);

            glGetShaderInfoLog(programObject, infoLen, NULL, infoLog);
            fprintf(stderr,"Error linking shader:\n%s\n", infoLog);
            free(infoLog);

            return 0;
        }
    }

    ShaderProgs = programObject;

	rtmatrixLoc = glGetUniformLocation(ShaderProgs, "myPMVMatrix");
	positionLoc = glGetAttribLocation(ShaderProgs, "vPosition");
	texCoordLoc = glGetAttribLocation(ShaderProgs, "myUV");

    return GL_TRUE;
}

static void initGLES2(void)
{
	int i,j;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for (i = 0; i < TH; i++) {
		for (j = 0; j < TW; j++) {
#if 0
			GLuint col = (255L<<24) + ((255L-j*2)<<16) + ((255L-i)<<8) + (255L-i*2);
			if ( ((i*j)/8) % 2 )
				col = (GLuint) (255L<<24) + (255L<<16) + (0L<<8) + (255L);
#else
			GLuint col = 0x00ffff00 * (i%2) * (j%2);
#endif
			bitmap[j][i] = col;
		}
	}

	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, TW, TH, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);
}

static void draw(void)
{
	float pfIdentity[] = {
		cos(angle), -sin(angle), 0, 0,
		sin(angle), cos(angle), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	GLfloat rectangle_vertices[12] = {
		+0.5, +0.5,	+0.0,	// lower left
		-0.5, +0.5, +0.0,	// lower right
		-0.5, -0.5, +0.0,	// upper right
		+0.5, -0.5, +0.0,	// upper left
	};

	GLfloat texture_coordinates[8] = {
		1.0, 0.0,
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
	};

	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    glViewport(0, 0, WIDTH, HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ShaderProgs);

	glUniformMatrix4fv(rtmatrixLoc, 1, GL_FALSE, pfIdentity);

	glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, &rectangle_vertices);
	glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, &texture_coordinates);

	glEnableVertexAttribArray(positionLoc);
	glEnableVertexAttribArray(texCoordLoc);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, textureId);

	glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    eglSwapBuffers(eglDisplay, eglWindowSurface);

	angle += delta;
}

int main(int argc, char *argv[])
{
    int iterations = 100;
    int ret = 0;

    /* */

    if(argc > 1)
        iterations = atoi(argv[1]);

	if (0 > platform_open())
	{
		printf("can't init platform\n");
		return -1;
	}

	nativeDisplay = plat_get_display();

    if (0 > nativeDisplay)
    {
        printf("can't get platform display\n");
        return -1;
    }

	nativeWindow = plat_get_window();

    if (0 > nativeWindow)
    {
        printf("can't get platform window\n");
        return -1;
    }

    if (0 > eglOpen()) {
        printf("error in eglOpen\n");
        return -1;
    }

    initShaders();
	initGLES2();

    while(iterations-- >= 0)
    {
        draw();
    }

    eglClose();

	platform_close();

    return 0;
}
