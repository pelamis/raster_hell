// алгоритм построчного сканировани€ многоугольника с упор€доченным списком ребер; 
//целочисленный алгоритм Ѕрезенхема с устранением ступенчатости;
//  http://www.glfw.org/docs/latest/quick.html
#define _USE_MATH_DEFINES
#include "stdafx.h"
#include "glew.h"
#include <gl\GLU.h>
#include <gl\GL.h>
#include "glfw3.h"
#include "math.h"
#include <list>
#include <vector>
#include <iterator>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct GLPoint
{
	GLfloat	x = -1;
	GLfloat	y = -1;
} GLPoint;

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


GLdouble A, B, C, D, mX, mY, minY, maxY;
typedef unsigned int uint;
typedef unsigned char uchar;
typedef std::list<int> IntList;
typedef std::vector<IntList> IntListArray;
typedef std::vector<Edge> Edges;
typedef std::vector<GLPoint> PPoly;
unsigned char ready = 0, alias = 0, go_fill = 0;

class Pixmap
{
public:
	RGB *map;
	Pixmap(uint h, uint w);
	~Pixmap();
	void clearMap();
	void setPixel(uint x, uint y, GLfloat R, GLfloat G, GLfloat B, GLfloat intens);
	void resize(int h, int w);
	RGB getPixel(uint x, uint y);
	uint height, width;
};

static void bresenham(GLPoint start, GLPoint end);
static void bresenham_aa(GLPoint start, GLPoint end);
void(*rasterL)(GLPoint start, GLPoint end) = bresenham;

void getPolyEdges();
void getCrosses(Edge e);
void drawContour();
void fillLine(int start, int end, int y);

void clearAllTheData();
GLint sgn(GLfloat expr);

PPoly polygon;
Edges lst;
IntListArray sscListArray;
Pixmap buffer(SCREEN_HEIGHT, SCREEN_WIDTH);

void getPolyEdges()
{
	Edge e;
	int i, k = polygon.size();

	for (i = 0; i < k; i++) //ѕреобразование набора точек полигона в массив граней
	{
		e.p0 = polygon[i%k], e.p1 = polygon[(i + 1) % k];
		lst.push_back(e);
	}
	return;
}

static void cursor_callback(GLFWwindow* window, double x, double y)
{
	
}

void reInitSscLArr(int newY)
{
	int i;
	IntList dummy;
	sscListArray.clear();
	for (i = 0; i < newY; i++) sscListArray.push_back(dummy);
	
}

static void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	//BEWARE OF IDIOTZ!!!
	GLPoint temp;
	temp.x = 0, temp.y = 0;
	if(button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			ready = 1;
		}
	}
	if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS))
	{
		glfwGetCursorPos(window, &mX, &mY);
		printf_s("Got cursor: (%lf,%lf)\n", mX, mY);
		if (ready)
		{
			ready = 0;
			clearAllTheData();
		}
		temp.x = (GLint)mX;
		temp.y = (GLint)mY;
		polygon.push_back(temp);
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
	buffer.resize((int)C*2,(int)A*4);
	reInitSscLArr((int)C * 2);
	printf("Reshape occured. New size is: %lf x %lf\n",A*4,C*2);
}

static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		buffer.clearMap();		//сигнал к очистке пол€
		clearAllTheData();
	}
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		if (!alias) alias=1,rasterL=bresenham_aa;
		else alias = 0, rasterL = bresenham;

	if (key == GLFW_KEY_F && action == GLFW_PRESS)
		go_fill = (go_fill==0) ? 1 : 0;
}

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

//=============================================================

void fillLine(int start, int end, int y)
{
	int i;
	for (i = start; i <= end; i++)
	{
		buffer.setPixel(i, y, 1, 1, 1, 1);
	}
}

//поиск и добавление в списки точек пересечени€ сканирующих строк с рЄбрами полигона
void getCrosses(Edge e)
{
	int i;
	GLfloat x;
	GLPoint beg, end;
	beg = (e.p0.y < e.p1.y) ? e.p0 : e.p1;
	end = (e.p0.y < e.p1.y) ? e.p1 : e.p0;
	if (beg.y == end.y) return;
	//концевые точки отрезка не учитываютс€
	for (i = beg.y+1; i <= end.y-1; i++)
	{
		x=nearbyintf(beg.x + (end.x - beg.x)*(i - beg.y) / (end.y - beg.y)); //вычисление точки пересечени€ сканирующей строки
		sscListArray[(int)i].push_back((int)x);								 //с обрабатываемым ребром
	}
	return;
}

void clearAllTheData()
{
	lst.clear();
	polygon.clear();
	sscListArray.clear();
}

static void bresenham(GLPoint start, GLPoint end)
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
		buffer.setPixel(x, y, 1, 1, 1, 1);
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

//Ѕрезенхем со сглаживанием
static void bresenham_aa(GLPoint start, GLPoint end)
{
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
	buffer.setPixel(x, y, 1, 1, 1, e);
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
		buffer.setPixel(x, y, 1, 1, 1, e);
	}
}

static void stringscan()
{
	int i, k, p_k = polygon.size(),tempLV;
	std::vector<int> temp;
	IntList l;
	getPolyEdges();

	// обработка вершин многоугольника
	if (sgn(polygon[0].y - polygon[p_k - 1].y) != sgn(polygon[1].y - polygon[0].y)) sscListArray[polygon[0].y].push_back(polygon[0].x);
	sscListArray[polygon[0].y].push_back(polygon[0].x);

	for (i = 1; i < p_k; i++)
	{
		if (sgn(polygon[i].y - polygon[i - 1].y) != sgn(polygon[(i + 1) % p_k].y - polygon[i].y)) sscListArray[polygon[i].y].push_back(polygon[i].x);
		sscListArray[polygon[i].y].push_back(polygon[i].x);
	}
	
	//ѕоиск пересечени€ со скан. строками неконцевых точек каждого ребра полигона
	for (i = 0; i < p_k; i++) getCrosses(lst[i]);

	for (i = 0; i < sscListArray.size(); i++)
	{	
		if (sscListArray[i].size() > 0)
		{
			sscListArray[i].sort();
			for (IntList::iterator j = sscListArray[i].begin(); j != sscListArray[i].end(); j++)
			{
				tempLV = *j;
				temp.push_back(tempLV);
			}

			for (k = 0; k < temp.size(); k += 2) fillLine(temp[k], temp[k + 1], i);
			
			temp.clear();
		}
	}
}

void draw()
{
	Edge e;

	int y,i,p_k=polygon.size(),peak=0;

	glDrawPixels(buffer.width, buffer.height, GL_RGB, GL_FLOAT, buffer.map);
	if (ready)
	{
		if (go_fill)
		{
			if (sscListArray.size() != buffer.height) reInitSscLArr(buffer.height);
			//заливка выполн€етс€ только дл€ замкнутых фигур
			if (polygon.size()>2) stringscan();
		}
		for (i = 0; i < p_k; i++) rasterL(polygon[i%p_k], polygon[(i + 1) % p_k]);
		
	}
	else
	{
		if (polygon.size()>0)
			for (i = 0; i < polygon.size()-1; i++) rasterL(polygon[i], polygon[i + 1]);
	}
}

int main(int argc, _TCHAR* argv[])
{
	A = SCREEN_WIDTH / 4.0;
	B = 0.0;
	C = D = SCREEN_HEIGHT / 2.0;
	int i;
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
		draw();

		glfwSwapBuffers(window);
		
		glfwPollEvents();
		//glfwWaitEvents();
	}

	glfwDestroyWindow(window);

	// clean up and exit
    glfwTerminate();

	clearAllTheData();
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

void Pixmap::resize(int h, int w)
{
	delete this->map;
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

GLint sgn(GLfloat expr)
{
	if (expr == 0) return 0;
	return (expr>0) ? 1 : -1;
}
