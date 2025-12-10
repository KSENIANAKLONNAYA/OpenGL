#pragma once
#include <gl/glew.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <vector>
#include <corecrt_math_defines.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// ID ��������� ���������
GLuint Task1;
GLuint Task2;
GLuint Task3;
GLuint Task4;

GLuint texture1;
GLuint texture2;
GLint U2_mix_value;
GLint U3_mix_value;

// ID ��������
GLint A1_vertex;
GLint A1_color;
GLint U1_affine;
GLint U1_proj;

GLint A2_vertex;
GLint A2_color;
GLint A2_texCoord;
GLint U2_affine;
GLint U2_proj;

GLint A3_vertex;
GLint A3_color;
GLint A3_texCoord;
GLint U3_affine;
GLint U3_proj;

GLint A4_vertex;
GLint A4_color;
GLint U4_affine;
GLint U4_proj;
GLint U4_scale;

// ID ���������� ������
GLuint VBO;
GLuint VBO2;

// ������� ������� ��������������
glm::mat4 affine;
// ������� ��������
glm::mat4 proj;

// ������� ���������������
glm::mat4 pie_scale;

GLfloat mix_value = 0.5f;

// ��������� ��� �������� ������
struct Vertex
{
	//coords
	GLfloat x;
	GLfloat y;
	GLfloat z;

	//colors
	GLfloat r;
	GLfloat g;
	GLfloat b;
	GLfloat a;

	// texture coords
	GLfloat s;
	GLfloat t;
};

// ������� ��� ��������� ������ ����������
void SetIcon(sf::Window& wnd);
// ������� ��� �������� ������
void checkOpenGLerror();



void ShaderLog(unsigned int shader);
// ������� ��� �������� ��������
void InitShader();
void LoadAttrib(GLuint prog, GLint& attrib, const char* attr_name);
void LoadUniform(GLuint prog, GLint& attrib, const char* attr_name);
// ������� ��� ������������� ���������� ������
void InitVBO();
// ������� ��� ������������� ��������
void InitTextures();
void Init();
// ������� ��� ���������
void Draw(sf::Window& window);
// ������� ��� ������� ��������
void ReleaseShader();
// ������� ��� ������� ���������� ������
void ReleaseVBO();
// ������� ��� ������� ��������
void Release();

enum class ShapeType {
	Gradient_Tetrahedron = 0,
	Gradient_Texture_Cube,
	Double_Texture_Cube,
	Gradient_Pie
};

#define red    1.0, 0.0, 0.0, 1.0
#define green  0.0, 1.0, 0.0, 1.0
#define blue   0.0, 0.0, 1.0, 1.0
#define yellow 1.0, 1.0, 0.3, 1.0
#define orange 1.0, 0.5, 0.0, 1.0
#define violet 0.5, 0.0, 1.0, 1.0
#define white  1.0, 1.0, 1.0, 1.0
#define cyan   0.0, 1.0, 1.0, 1.0

class RGB {
	static constexpr float DPI = M_PI * 2;
	static constexpr float a60 = M_PI / 3;
	static constexpr float a120 = a60 * 2;
	static constexpr float a180 = M_PI;
	static constexpr float a240 = M_PI + a60;
	static constexpr float a300 = M_PI + a120;
	static constexpr float a360 = DPI;

public:
	static float r(float phi) {
		if (phi <= a60 || phi >= a300)
			return 1.f;
		
		if (phi >= a120 && phi <= a240)
			return 0.f;
		
		if (phi < a120)
			return 1.f - (phi - a60) / a60;
		
		if (phi > a240)
			return (phi - a240) / a60;
	}

	static float g(float phi) {
		if (phi >= a60 && phi <= a180)
			return 1.f;
		
		if (phi >= a240)
			return 0.f;
		
		if (phi < a60)
			return phi / a60;
		
		if (phi > a180)
			return 1.f - (phi - a180) / a60;
	}

	static float b(float phi) {
		return g(DPI - phi);
	}
};