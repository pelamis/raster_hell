// алгоритм построчного сканирования многоугольника с упорядоченным списком ребер; 
//целочисленный алгоритм Брезенхема с устранением ступенчатости;
//  http://www.glfw.org/docs/latest/quick.html
#define _USE_MATH_DEFINES
#include "stdafx.h"
#include "glew.h"
#include <gl\GLU.h>
#include <gl\GL.h>
#include "glfw3.h"
#include "math.h"
#include <random>
#include <list>
#include <vector>
#include <iterator>
#include <time.h>
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

GLdouble A, B, C, D, mX, mY, minY, maxY;
typedef unsigned int uint;
typedef unsigned char uchar;
typedef std::list<int> IntList;
typedef std::vector<IntList> IntListArray;
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
//сделать универсальным!

//сделать универсальным!
class List
{
public:
	Elem *first;
	int items;
	List();
	void clearAll();
	void addBeforeHead(Elem *elem);
	int insertAfter(Elem *target, Elem *elem);
	int insertBefore(Elem *target, Elem *elem);
	(Elem *)insSort(Elem *l, int lstLen);
	(Elem*) delElem(Elem *elem);
	void dEnumElemz(void(*enumElem)(void *d));
	(Elem*) searchElem(Elem *elem,int (*compare)(void *a, void *b));
};

class Pixmap
{
public:
	RGB *map;
	Pixmap(uint h, uint w);
	~Pixmap();
	void clearMap();
	void setPixel(uint x, uint y, GLfloat R, GLfloat G, GLfloat B, GLfloat intens);
	RGB getPixel(uint x, uint y);
	uint height, width;
};
static void bresenham(GLPoint start, GLPoint end, Pixmap *map);
static void bresenham_aa(GLPoint start, GLPoint end, Pixmap *map);
static void addX(int x, int y, List *sscListArray);
void enumEdges(void *data);
void crosses(void *data);
void (*rasterL)(GLPoint start, GLPoint end, Pixmap *map) = bresenham;
GLPoint first, prev, current;
List lst = List();
//List *sscListArray = new List[(int)C * 2];
GLint sgn(GLfloat expr);



static void cursor_callback(GLFWwindow* window, double x, double y)
{
	
}

GLvoid * pixx;
//REWORK THIS FUNC!!!
static void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	//BEWARE OF IDIOTZ!!!
	GLPoint temp;
	temp.x = 0, temp.y = 0;
	if(button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			inPoly = 0;		//Одинарный клик RMB - сигнал к завершению задания полигона
			ready = 1;
			prev.x = current.x, prev.y = current.y;
			current.x = first.x, current.y = first.y;
			printf_s("minY:\t%lf\nmaxY:\t%lf\n", minY, maxY);
			go_fill = 1;
			
		}
	}
	if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS))
	{
		glfwGetCursorPos(window, &mX, &mY);
		//mY = C * 2 - mY;
		printf_s("Got cursor: (%lf,%lf)\n", mX, mY);
		if (inPoly == 0)
		{
			inPoly = 1;
			minY = mY;
			maxY = mY;
			first.x = (GLint)mX;
			first.y = (GLint)mY;
			current.x = (GLint)mX;
			current.y = (GLint)mY;
			prev.x = current.x, prev.y = current.y;
		}
		else 
		{
			if (mY > maxY) maxY = mY;
			else if (mY < minY) minY = mY;
			prev.x = current.x, prev.y = current.y;
			current.x = (GLint)mX;
			current.y = (GLint)mY;
			ready = 1;
		}
		
	}
}

static void resize_callback(GLFWwindow* window, int width, int height)
{
	int pcside;
	pcside=(width>height?width:height)*2;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0.0, width, 0.0, height, -pcside, pcside);
	glMatrixMode(GL_MODELVIEW);
	A = width / 4.0;
	B = 0.0;
	C = D = height / 2.0;
	printf("Reshape occured. New size is: %lf x %lf\n",A*4,C*2);
}

static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		((Pixmap *)pixx)->clearMap();		//сигнал к очистке поля
		lst.clearAll();
		prev.x = -1, prev.y = -1;
		current.x = -1, current.y = -1;
	}
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		if (!alias) alias=1,rasterL=bresenham_aa;
		else alias = 0, rasterL = bresenham;
}

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

//=============================================================
static void bresenham_mod(GLPoint start, GLPoint end, List *sscListArray, Pixmap *map)
{
	GLint x = (GLint)start.x, y = (GLint)start.y,
		dx = (GLint)abs(end.x - start.x),
		dy = (GLint)abs(end.y - start.y),
		sx = sgn(end.x - start.x), i, tmp,
		sy = sgn(end.y - start.y), swap = 0, e,lock=0;
	

	if (dy == 0) return;
	if (dy > dx)
	{
		swap = 1;
		tmp = dx, dx = dy, dy = tmp;
	}
	e = 2 * dy - dx;
	for (i = 0; i < dx; i++)
	{
		map->setPixel(x, y, 1, 1, 1, 1);
		while (e >= 0)
		{
			/*if (!lock && swap && (!(sx + sy)))
			{
				addX(x, y, sscListArray);
				lock = 1;
			}*/
			
			if (swap)
			{
				x += sx;
			}
			else {
				y += sy;
				//lock = 0;
			}
			e -= 2 * dx;
		}
		if (swap) 
		{
			y += sy;
		}
		else x += sx;
		e += 2 * dy;
	}
	
	
	//clean up all the data!!!
}

static void addX(int x, int y, List *sscListArray)
{
	Integer *val = new Integer;
	Elem *xcoord = new Elem;
	val->value = (int)x;
	xcoord->data = (void *)val;
	sscListArray[(GLint)y].addBeforeHead(xcoord);
}

static void bresenham(GLPoint start, GLPoint end, Pixmap *map)
{
	GLint x = (GLint)start.x, y = (GLint)start.y,
		dx = (GLint)abs(end.x - start.x),
		dy = (GLint)abs(end.y - start.y),
		sx = sgn(end.x - start.x), i, tmp,
		sy = sgn(end.y - start.y), swap = 0, e;
	if (dy > dx)
	{
		swap = 1;
		tmp = dx, dx = dy, dy = tmp;
	}
	e = 2 * dy - dx;
	for (i = 0; i < dx; i++)
	{
		map->setPixel(x, y, 1, 1, 1, 1);
		while (e >= 0)
		{
			if (swap) x += sx;
			else y += sy;
			e -= 2 * dx;
		}
		if (swap) y += sy;
		else x += sx;
		e += 2 * dy;
	}
}
static void bresenham_aa(GLPoint start, GLPoint end, Pixmap *map)
{
	//BEWARE OF IDIOTZ!!!
	GLint x = (GLint)start.x, y = (GLint)start.y, I = 1,
		dx = (GLint)abs(end.x - start.x),
		dy = (GLint)abs(end.y - start.y),
		sx = sgn(end.x - start.x), i, tmp,
		sy = sgn(end.y - start.y), swap=0;
	GLdouble m, w, e,invert= (abs(sx+sy)>0) ? 1 : 0;
	if (dy >= dx)
	{
		swap = 1;
		tmp = dx, dx = dy, dy = tmp;
	}
	m = ((GLdouble)I * (GLdouble)dy) / (GLdouble)dx;
	w = (GLdouble)I - m, e = (GLdouble)I / 2;
	map->setPixel(x, y, 1, 1, 1, e);
	for (i = 0; i < dx;i++)
	{
		if (swap)
		{
			y += sy;
			if (!invert)
			{
				if (e > m) e -= m;
				else x += sx, e += w;
			}
			else
			{
				if (e < w) e = e + m;
				else x += sx, e = e - w;
			}
		}
		else
		{
			x += sx;
			if (invert)
			{
				if (e > m) e -= m;
				else y+=sy, e += w;
			}
			else
			{
				if (e < w) e = e + m;
				else y += sy, e = e - w;
			}
			
		}
		map->setPixel(x, y, 1, 1, 1, e);
	}
}

static void stringscan(Pixmap *buffer, List *list, List *sscListArray)
{
	int i;
	Elem *elem = list->first;
	for (i = 0; i < C * 2; i++)
	{
		if (sscListArray[i].first != NULL)
		{
			printf_s("\nString #%d:\n",i);
			sscListArray[i].dEnumElemz(crosses);
			
		}
		
	}
	for (i = 0; i < C * 2; i++)
	{
		if (sscListArray[i].first!=NULL) sscListArray[i].clearAll();
	}
	delete sscListArray;
	//for (i = 0; i < list->items; i++) mids[i].y = 0;
}

void draw(Pixmap *buffer, List *list, IntListArray *sscListArray)
{
	Elem *elem = NULL;
	Integer *val = NULL;
	Elem *xcoord = NULL;
	int y;
	float i,y_s,y_e,x_s,x_e,x;
	//GLint x=0;
	//List *sscListArray = new List[(int)C * 2];
	glDrawPixels(buffer->width, buffer->height, GL_RGB, GL_FLOAT, buffer->map);
	if (ready)
	{
		printf_s("Gotcha!\n");
		elem = new Elem;
		elem->data = new Edge;
		((Edge *)(elem->data))->p0 = prev, ((Edge *)(elem->data))->p1 = current, elem->next = NULL;
		list->addBeforeHead(elem);
		if (elem != NULL)
		{
			rasterL(prev, current, buffer);
			//x = x_s + (x_e-x_s)*(y_str-y_s)/(y_e-y_s)
			//bresenham_mod(prev, current, sscListArray, buffer);
			if (current.y != prev.y)
			{
				if (current.y > prev.y) y_e = current.y, y_s = prev.y,x_e=current.x,x_s=prev.x;
				else y_e = prev.y, y_s = current.y,x_e=prev.x,x_s=current.x;
				for (i = y_s; i <= y_e; i++)
				{
					xcoord = new Elem;
					val = new Integer;
					x=nearbyintf(x_s + (x_e - x_s)*(i - y_s) / (y_e - y_s));
					val->value = (int)x;
					xcoord->data = (void *)val;
					sscListArray[(GLint)i]
				}
			}
			
			//rasterL(prev, current, list,buffer);
		}
		ready = 0;
	}
	if (go_fill) {
		elem = new Elem;
		elem->data = new Edge;
		((Edge *)(elem->data))->p0 = prev, ((Edge *)(elem->data))->p1 = current, elem->next = NULL;
		list->addBeforeHead(elem);
		if (current.y != prev.y)
		{
			if (current.y > prev.y) y_e = current.y, y_s = prev.y, x_e = current.x, x_s = prev.x;
			else y_e = prev.y, y_s = current.y, x_e = prev.x, x_s = current.x;
			for (i = y_s; i <= y_e; i++)
			{
				xcoord = new Elem;
				val = new Integer;
				x = nearbyintf(x_s + (x_e - x_s)*(i - y_s) / (y_e - y_s));
				val->value = (int)x;
				xcoord->data = (void *)val;
				sscListArray[(GLint)i].addBeforeHead(xcoord);
			}
		}
		stringscan(buffer, list,sscListArray);
		go_fill = 0;
	}
}

int main(int argc, _TCHAR* argv[])
{
	A = SCREEN_WIDTH / 4.0;
	B = 0.0;
	C = D = SCREEN_HEIGHT / 2.0;
	inPoly = 0;
	pixx = NULL;
	int i;
	Pixmap buffer(SCREEN_HEIGHT, SCREEN_WIDTH);
	//vector<list<int>> sscListArray((int)C * 2);
	IntListArray sscListArray((int)C * 2);
	pixx = (GLvoid *)(&buffer);
	glRasterPos2s(0, 0);
	// initialise GLFW
    if(!glfwInit())
	{
		printf("glfwInit failed\n");
		return -1;
	}

	glfwSetErrorCallback(error_callback);

	GLFWwindow* window;
	//comment
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Test app", NULL, NULL);
	if (window == NULL)
	{
		printf("glfwOpenWindow failed. Can your hardware handle OpenGL 3.2?\n");
		glfwTerminate();
		return -2;
	}

	int attrib;
	attrib = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
	attrib = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
	attrib = glfwGetWindowAttrib(window, GLFW_OPENGL_PROFILE);

	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, keyboard_callback);
	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, cursor_callback);
	
    resize_callback(window, SCREEN_WIDTH, SCREEN_HEIGHT);
	while (!glfwWindowShouldClose(window))
	{
		draw(&buffer,&lst,sscListArray);

		glfwSwapBuffers(window);
		
		glfwPollEvents();
		//glfwWaitEvents();
	}

	glfwDestroyWindow(window);

	// clean up and exit
    glfwTerminate();
	lst.dEnumElemz(enumEdges);
	lst.clearAll();
	scanf_s("\n");
	return 0;
}

//=============================================================

void Pixmap::clearMap()
{
	uint x, y;
	for (y = 0; y < height; y++)
	for (x = 0; x < width; x++)
	{
		this->setPixel(x, y, 0.0, 0.0, 0, 1);
	}
}

Pixmap::Pixmap(uint h, uint w)
{
	this->height = h, this->width = w;
	this->map = new RGB[height*width];
	clearMap();
}

Pixmap::~Pixmap()
{
	delete this->map;
	printf_s("Pixmaps don't burn!\n");
}

void Pixmap::setPixel(uint x, uint y, GLfloat R, GLfloat G, GLfloat B, GLfloat intens)
{
	map[(height-y-1)*width + x].r = R*intens;
	map[(height - y - 1)*width + x].g = G*intens;
	map[(height - y - 1)*width + x].b = B*intens;
}

RGB Pixmap::getPixel(uint x, uint y)
{
	return map[y*width + x];
}

//=============================================================

List::List()
{
	first = NULL;
	items = 0;
}

void List::addBeforeHead(Elem *elem)
{
	Elem *tmp;
	if (elem != NULL)
	{
		tmp = first;
		//tmp->prev = elem;
		first = elem;
		first->prev = NULL;
		elem->next = tmp;
		items++;
	}
}

int List::insertAfter(Elem *target, Elem *elem)
{
	if ((target != NULL) && (elem != NULL))
	{
		elem->prev = target;
		elem->next = target->next;
		target->next->prev = elem;
		target->next = elem;
		items++;
	}
	else return -1;
	return 0;
}

int List::insertBefore(Elem *target, Elem *elem)
{
	Elem *preTarget = target->prev;
	target->prev = elem;
	elem->next = target;
	elem->prev = preTarget;
	preTarget->next = elem;
	return 0;
}
Elem* List::delElem(Elem *elem)
{
	Elem *tmp = first;
	for (; ((tmp!=NULL)&&(tmp->next != elem)); tmp = tmp->next);
	tmp->next = elem->next;
	elem->next = NULL;
	items--;
	return elem;
}

Elem* List::searchElem(Elem *elem, int (*compare)(void *a, void *b))
{
	Elem *tmp = first;
	int result = 0;
	for (; (result==0)&&(tmp->next != NULL); tmp = tmp->next)
	{
		result = compare(elem->data, tmp->data);
	}
	if (result) return tmp;
	printf_s("ЕГГОГ\n");
	return NULL;
}

void List::dEnumElemz(void(*enumElem)(void *d))
{
	Elem *curr = first;
	while (curr != NULL)
	{
		enumElem(curr->data);
		curr = curr->next;
	}
}

void List::clearAll()
{
	Elem *tmp = first;
	while (items > 0)
	{
		first = tmp->next;
		delete tmp->data;
		delete tmp;
		tmp = first;
		items--;
	}

}

Elem * List::insSort(Elem *l, int lstLen)
{
	Elem *pick = l->next, *save = pick->next,
		*loc = pick->prev;
	/*while ((pick != l))
	{
		loc = pick->prev;
		while ((loc != l) && (pick != save))
		{
			if (((loc->prev)->v <= pick->v) && (pick->v <= loc->v))
			{
				delElem(pick);
				insertBefore(loc,pick);
				pick = save;
			}
			loc = loc->prev;
		}
		if ((pick != save) && (loc == l))
		if (pick->v <= loc->v)
		{
			delElem(pick);
			insertBefore(loc,pick);
			l = pick;
		}
		pick = save;
		save = save->next;
	}*/
	return l;
}
//=============================================================

GLint sgn(GLfloat expr)
{
	if (expr == 0) return 0;
	return (expr>0) ? 1 : -1;
}

void enumEdges(void *d)
{
	printf_s("(%f;%f)--(%f;%f)\n", ((Edge *)d)->p0.x, ((Edge *)d)->p0.y, ((Edge *)d)->p1.x, ((Edge *)d)->p1.y);
}

void crosses(void *data)
{
	printf_s("( %d ) \n", ((Integer *)data)->value);
}