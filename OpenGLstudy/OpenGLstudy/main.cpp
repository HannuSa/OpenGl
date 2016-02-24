#include <Windows.h>
#include <cassert>
#include <iostream>
#include "Glew/glew.h"
#include "Glew/wglew.h"
#include <gl/GL.h>
#include <gl/GLU.h>
#include "lodepng.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
		break;
	}
}

void errorMessage(std::string error)
{
	MessageBoxA(0, error.c_str(), "Error in Code", 0);
}

static const GLchar* VERTEX_SOURCE =
"attribute vec3 attrPosition;\n"
"attribute vec3 attrColor;\n"

"attribute vec2 textCoord;\n"

"uniform mat4 unifProjection;\n"
"uniform mat4 unifView\n;"
"uniform mat4 unifWorld;\n"

"varying vec3 varyColor;\n"
"varying vec2 f_textCoord;\n"
"varying float unifAlpha;\n"

"void main()\n"
"{\n"
"varyColor = attrColor;\n"
"gl_Position = unifProjection * unifView * unifWorld * vec4(attrPosition, 1.0);\n"
"f_textCoord = textCoord;\n"
"}"
;

//TODO:
//UNIFORM SAMPLER 2D variable
static const GLchar* FRAGMENT_SOURCE =
"uniform float unifAlpha;\n"
"varying vec3 varyColor;\n"
"varying vec2 f_textCoord;\n"
"uniform sampler2D mytexture;\n"
"void main()\
{\
 gl_FragColor = texture2D(mytexture, f_textCoord);\
}\
";

void checkShaderErrors(GLuint shader)
{
	GLint result;
	int logLength;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	char errorMsg[1200] = "\n";
	glGetShaderInfoLog(shader, logLength, NULL, errorMsg);
	errorMessage(std::string(errorMsg));
	std::cout << errorMessage << std::endl;
	std::cout << errorMsg << std::endl;
}

bool loadOBJ(const char* path, std::vector<GLfloat> &out_vertices, std::vector<GLuint> &out_indices)
{
	std::vector< GLuint > vertexIndices, uvIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;

	FILE * file = fopen(path, "r");
	if (file == NULL)
	{
		errorMessage("Impossible to open the file!");
		return false;
	}

	while (1)
	{
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0)
		{
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0)
		{
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "f") == 0){
			//std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3];
			int matches = fscanf(file, "%d/%d %d/%d %d/%d\n", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2]);
			if (matches != 6){
				errorMessage("File can't be read by our simple parser");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0] - 1);
			vertexIndices.push_back(vertexIndex[1] - 1);
			vertexIndices.push_back(vertexIndex[2] - 1);
			uvIndices.push_back(uvIndex[0] - 1);
			uvIndices.push_back(uvIndex[1] - 1);
			uvIndices.push_back(uvIndex[2] - 1);

		}
	}

	out_vertices.resize(vertexIndices.size() * 5);
	unsigned int index, index2;

	for (unsigned int i = 0; i < vertexIndices.size(); i++)
	{
		index = vertexIndices[i];
		out_vertices[5 * index] = temp_vertices[vertexIndices[i]].x;
		out_vertices[5 * index + 1] = temp_vertices[vertexIndices[i]].y;
		out_vertices[5 * index + 2] = temp_vertices[vertexIndices[i]].z;

		index2 = uvIndices[i];
		out_vertices[5 * index2 + 3] = temp_uvs[uvIndices[i]].x;
		out_vertices[5 * index2 + 4] = temp_uvs[uvIndices[i]].y;
	}


	out_indices = vertexIndices;
	return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS windowClass;
	HWND windowHandle;
	HINSTANCE instance = NULL;
	int pixelformat;

	windowClass.style = CS_OWNDC;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = instance;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = L"Window";
	windowClass.lpfnWndProc = proc;


	if (!RegisterClass(&windowClass))
		printf("error: %d\r\n", GetLastError());

	windowHandle = CreateWindowEx(
		WS_EX_APPWINDOW,
		L"Window",
		L"TEST",
		WS_OVERLAPPED | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		800,
		600,
		(HWND)NULL,
		(HMENU)NULL,
		NULL, NULL);

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if (!windowHandle)
	{
		//error here
		errorMessage("Major problem with windowhandle");
		return 0;
	}

	ShowWindow(windowHandle, true);

	//OpenGL context creation
	HDC hdc = GetDC(windowHandle);
	HGLRC hglrc;

	pixelformat = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixelformat, &pfd);
	hglrc = wglCreateContext(hdc);
	wglMakeCurrent(GetDC(windowHandle), hglrc);

	//OpenGl and shader stuff

	GLenum err = glewInit();
	const int r = wglSwapIntervalEXT(1);
	assert(r != 0);
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	//Create shaders
	GLuint programObject = glCreateProgram();

	GLuint vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShaderObject, 1u, &VERTEX_SOURCE, NULL);

	glShaderSource(fragmentShaderObject, 1u, &FRAGMENT_SOURCE, NULL);

	GLint vertexCompileSuccess;
	glCompileShader(vertexShaderObject);
	glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &vertexCompileSuccess);
	if (vertexCompileSuccess != GL_TRUE)
	{
		checkShaderErrors(vertexShaderObject);
		errorMessage("Vertex compile failed");
	}

	GLint fragmentCompileSuccess;
	glCompileShader(fragmentShaderObject);
	glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &fragmentCompileSuccess);
	if (fragmentCompileSuccess != GL_TRUE)
	{
		checkShaderErrors(fragmentShaderObject);
		errorMessage("Fragment compile failed");
	}

	//First attach both, then link
	glAttachShader(programObject, vertexShaderObject);
	glAttachShader(programObject, fragmentShaderObject);

	glLinkProgram(programObject);

	GLint LinkSuccess;
	glGetProgramiv(programObject, GL_LINK_STATUS, &LinkSuccess);

	if (LinkSuccess != GL_TRUE)
	{
		errorMessage("Linking failed");
	}

	//

	//Enable attribute array

	GLint positionIndex = glGetAttribLocation(programObject, "attrPosition");
	assert(positionIndex >= 0);
	glEnableVertexAttribArray(positionIndex);

	GLint textureIndex = glGetAttribLocation(programObject, "textCoord");
	assert(textureIndex >= 0);
	glEnableVertexAttribArray(textureIndex);

	//Create vertex and index buffers

	GLuint buffers[4];
	glGenBuffers(4, buffers);

	std::vector<GLfloat> VertexData;
	std::vector<GLuint> IndexData;
	bool res = loadOBJ("Teapot.obj", VertexData, IndexData);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, VertexData.size()*sizeof(GLfloat), VertexData.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexData.size()*sizeof(GLuint), IndexData.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	std::vector<GLfloat> VertexData2;
	std::vector<GLuint> IndexData2;
	bool res2 = loadOBJ("Monkeyhead.obj", VertexData2, IndexData2);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[2]);
	glBufferData(GL_ARRAY_BUFFER, VertexData2.size()*sizeof(GLfloat), VertexData2.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexData2.size()*sizeof(GLuint), IndexData2.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	//Create texture

	GLuint texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	unsigned width, height;
	std::vector<unsigned char> image;

	unsigned error = lodepng::decode(image, width, height, "handle_bump.png");
	assert(error == 0);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	GLenum glerr = glGetError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GLint uniform_mytexture;
	uniform_mytexture = glGetUniformLocation(programObject, "handle_bump.png");
	glUniform1i(uniform_mytexture, 0);

	glActiveTexture(GL_TEXTURE0);

	////Blending
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	glClearColor(0, 1, 0, 0);

	//Setting uniforms

	glUseProgram(programObject);

	//Projection

	GLint projectionIndex = glGetUniformLocation(programObject, "unifProjection");
	assert(projectionIndex != -1);

	//FOV, aspect ratio, Near draw, Far draw
	glm::mat4 projectionTransform = glm::perspective(60.0f, 800.0f / 600.0f, 0.01f, 1000.0f);
	//glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, reinterpret_cast<float*>(&projectionTransform));
	glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projectionTransform));

	//View
	float Camerarotation = 1;

	GLint viewIndex = glGetUniformLocation(programObject, "unifView");
	assert(viewIndex != -1);

	glm::mat4 viewTransform = glm::translate(glm::vec3(0.0f, 0.0f, -5.0f));
	glUniformMatrix4fv(viewIndex, 1, GL_FALSE, reinterpret_cast<float*>(&viewTransform));

	//World stuff
	float rotation = 1;
	GLint worldIndex = glGetUniformLocation(programObject, "unifWorld");
	assert(worldIndex != -1);

	glm::mat4 worldTransform = glm::translate(glm::vec3(0.0f, 0.0f, -1.0f));
	glUniformMatrix4fv(worldIndex, 1, GL_FALSE, reinterpret_cast<float*>(&worldTransform));

	glUseProgram(0u);

	float dt = 0.0;

	for (;;)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}

		LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
		LARGE_INTEGER Frequency;

		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&StartingTime);


		//Stencil mask default is 0, clear 0 is 0
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);

		//Draw
		glUseProgram(programObject);

		//World rotation happens here
		rotation += 1.0f;
		worldTransform = /*glm::mat4(); glm::translate(glm::vec3(0.0f, 0.0f, -5.0f)) **/ glm::rotate(rotation, glm::vec3(0.0f, dt, 0.0f));
		glUniformMatrix4fv(worldIndex, 1, GL_FALSE, reinterpret_cast<float*>(&worldTransform));

		dt = 0;

		////Camera rotation
		//Camerarotation += 1.0f;
		//glm::mat4 viewTransform = glm::translate(glm::vec3(-0.5f, -0.5f, -2.5f))* glm::rotate(Camerarotation, glm::vec3(1.0f, 1.0f, 1.0f));
		//glUniformMatrix4fv(viewIndex, 1, GL_FALSE, reinterpret_cast<float*>(&viewTransform));
		//
		glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
		//attrib, amount of dimensional attributes, type of atttributes , normalized?, reference, pointer to data
		glVertexAttribPointer(positionIndex, 3, GL_FLOAT, GL_FALSE, 20, reinterpret_cast<GLvoid*>(0));
		glVertexAttribPointer(textureIndex, 2, GL_FLOAT, GL_FALSE, 20, reinterpret_cast<GLvoid*>(12));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);

		glDrawElements(GL_TRIANGLES, IndexData.size(), GL_UNSIGNED_INT, reinterpret_cast<GLvoid*>(0));

		glBindBuffer(GL_ARRAY_BUFFER, buffers[2]);
		//attrib, amount of dimensional attributes, type of atttributes , normalized?, reference, pointer to data
		glVertexAttribPointer(positionIndex, 3, GL_FLOAT, GL_FALSE, 20, reinterpret_cast<GLvoid*>(0));
		glVertexAttribPointer(textureIndex, 2, GL_FLOAT, GL_FALSE, 20, reinterpret_cast<GLvoid*>(12));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[3]);

		glDrawElements(GL_TRIANGLES, IndexData2.size(), GL_UNSIGNED_INT, reinterpret_cast<GLvoid*>(0));

		glUseProgram(0u);

		SwapBuffers(hdc);

		QueryPerformanceCounter(&EndingTime);
		ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

		std::cout << "Time used: " << std::endl;
		ElapsedMicroseconds.QuadPart *= 100000000;
		ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

		std::cout << ElapsedMicroseconds.QuadPart << std::endl;
		dt = dt + ElapsedMicroseconds.QuadPart / 1000;

	}

	//Delete texture
	glDeleteTextures(1, &texture);

	//Destroy buffers
	glDeleteBuffers(4, buffers);

	//OpenGL context destroy
	wglMakeCurrent(GetDC(windowHandle), NULL);
	wglDeleteContext(hglrc);

	return 0;
}