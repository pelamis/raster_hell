#define _USE_MATH_DEFINES
#include "stdafx.h"
#include "glew.h"
#include <gl\GLU.h>
#include <gl\GL.h>
#include "glfw3.h"
#include "math.h"
#include <random>
#include <time.h>
#include <list>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

GLdouble A, B, C, D, mX, mY, minY, maxY;
typedef unsigned int uint;
typedef unsigned char uchar;
unsigned char inPoly, ready = 0, alias = 0, go_fill = 0;

typedef struct GLPoint
{
	GLfloat	x = -1;
	GLfloat	y = -1;
} GLPoint;

typedef struct Elem
{
	void *data;
	Elem *next, *prev;
}Elem;

typedef struct Edge
{
	GLPoint p0, p1;
}Edge;

typedef struct Integer
{
	GLint value;
}Integer;

typedef struct RGB
{
	GLfloat r, g, b;
}RGB;