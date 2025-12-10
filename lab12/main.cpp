#include "main.h"
#include "shaders.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

ShapeType shapetype = ShapeType::Gradient_Tetrahedron;

void Init()
{
	proj = glm::perspective(45.0f, 1.0f, 0.1f, 100.0f);
	affine = glm::mat4(1.0f);
	pie_scale = glm::mat4(1.0f);
	//Включаем проверку глубины
	glEnable(GL_DEPTH_TEST);
	// Инициализируем шейдеры
	InitShader();
	// Инициализируем вершинный буфер
	InitVBO();
	InitTextures();
}

int main()
{
	setlocale(LC_ALL, "Russian");
	sf::Window window(sf::VideoMode(600, 600), "Shapes", sf::Style::Default, sf::ContextSettings(24));
	SetIcon(window);
	window.setVerticalSyncEnabled(true); // Вертикальная синхронизация
	window.setActive(true); // Устанавливаем контекст OpenGL
	glewInit(); // Инициализируем GLEW
	Init(); // Инициализируем ресурсы
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed) // Если пользователь закрыл окно
			{
				window.close(); // Закрываем окно
				goto EXIT_IS_RIGHT_HERE; // Выходим из цикла
			}
			else if (event.type == sf::Event::Resized) // Если пользователь изменил размер окна
			{
				glViewport(0, 0, event.size.width, event.size.height); // Устанавливаем область вывода
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				// Rotation
				if (event.key.code == sf::Keyboard::P)
				{
					affine = glm::rotate(affine, 0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
				}
				else if (event.key.code == sf::Keyboard::R)
				{
					affine = glm::rotate(affine, 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
				}
				else if (event.key.code == sf::Keyboard::Y)
				{
					affine = glm::rotate(affine, 0.1f, glm::vec3(0.0f, 0.0f, 1.0f));
				}

				// Movement
				else if (event.key.code == sf::Keyboard::W)
				{
					affine = glm::translate(affine, glm::vec3(0.0f, 0.0f, 0.1f));
				}
				else if (event.key.code == sf::Keyboard::S)
				{
					affine = glm::translate(affine, glm::vec3(0.0f, 0.0f, -0.1f));
				}
				else if (event.key.code == sf::Keyboard::A)
				{
					affine = glm::translate(affine, glm::vec3(-0.1f, 0.0f, 0.0f));
				}
				else if (event.key.code == sf::Keyboard::D)
				{
					affine = glm::translate(affine, glm::vec3(0.1f, 0.0f, 0.0f));
				}
				else if (event.key.code == sf::Keyboard::Z)
				{
					affine = glm::translate(affine, glm::vec3(0.0f, 0.1f, 0.0f));
				}
				else if (event.key.code == sf::Keyboard::X)
				{
					affine = glm::translate(affine, glm::vec3(0.0f, -0.1f, 0.0f));
				}
				else if (event.key.code == sf::Keyboard::Add)
				{
					if (mix_value < 1.0f)
					{
						mix_value += 0.1f;
					}
				}
				else if (event.key.code == sf::Keyboard::Subtract)
				{
					if (mix_value > 0.1f)
					{
						mix_value -= 0.1f;
					}
				}

				else if (event.key.code == sf::Keyboard::F1)
				{
					proj = glm::perspective(45.0f, 1.0f, 0.1f, 100.0f);
				}

				else if (event.key.code == sf::Keyboard::F2)
				{
					proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
				}

				else if (event.key.code == sf::Keyboard::Escape)
				{
					affine = glm::mat4(1.0f);
					pie_scale = glm::mat4(1.0f);
				}

				else if (event.key.code == sf::Keyboard::Num1)
				{
					shapetype = ShapeType::Gradient_Tetrahedron;
				}

				else if (event.key.code == sf::Keyboard::Num2)
				{
					shapetype = ShapeType::Gradient_Texture_Cube;
				}

				else if (event.key.code == sf::Keyboard::Num3)
				{
					shapetype = ShapeType::Double_Texture_Cube;
				}

				else if (event.key.code == sf::Keyboard::Num4)
				{
					shapetype = ShapeType::Gradient_Pie;
				}

				// Pie Scaling
				else if (event.key.code == sf::Keyboard::Numpad8)
				{
					pie_scale = glm::scale(pie_scale, glm::vec3(1.0f, 1.1f, 1.0f));
				}
				else if (event.key.code == sf::Keyboard::Numpad2)
				{
					pie_scale = glm::scale(pie_scale, glm::vec3(1.0f, 0.9f, 1.0f));
				}
				else if (event.key.code == sf::Keyboard::Numpad4)
				{
					pie_scale = glm::scale(pie_scale, glm::vec3(0.9f, 1.0f, 1.0f));
				}
				else if (event.key.code == sf::Keyboard::Numpad6)
				{
					pie_scale = glm::scale(pie_scale, glm::vec3(1.1f, 1.0f, 1.0f));
				}
			}
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Очищаем буфер цвета и буфер глубины
		Draw(window); // Рисуем
		window.display(); // Выводим на экран
	}
EXIT_IS_RIGHT_HERE: // Метка выхода
	Release(); // Очищаем ресурсы
	return 0; // Выходим из программы
}

void InitVBO()
{
	glGenBuffers(1, &VBO); // Генерируем вершинный буфер
	glGenBuffers(1, &VBO2);
	Vertex data[] = {
		//Tetrahedron
		{0.0f, 0.0f, 0.0f, red}, {0.0f, 1.0f, 1.0f, green}, {1.0f, 0.0f, 1.0f, blue},
		{0.0f, 0.0f, 0.0f, red}, {0.0f, 1.0f, 1.0f, green}, {1.0f, 1.0f, 0.0f, white},
		{0.0f, 0.0f, 0.0f, red}, {1.0f, 1.0f, 0.0f, white}, {1.0f, 0.0f, 1.0f, blue},
		{1.0f, 1.0f, 0.0f, white}, {1.0f, 0.0f, 1.0f, blue}, {0.0f, 1.0f, 1.0f, green},

		//Cube
		{1.0f, 1.0f, -1.0f, red, 1.0f, 0.0f}, {-1.0f, 1.0f, -1.0f, blue, 0.0f, 0.0f}, {-1.0f, 1.0f, 1.0f, green, 0.0f, 1.0f},{1.0f, 1.0f, 1.0f, orange, 1.0f, 1.0f}, //top face
		{1.0f, -1.0f, 1.0f, yellow, 1.0f, 0.0f}, {-1.0f, -1.0f, 1.0f, violet, 0.0f, 0.0f}, {-1.0f, -1.0f, -1.0f, white, 0.0f, 1.0f}, {1.0f, -1.0f, -1.0f, cyan, 1.0f,1.0f}, // bottom face
		{1.0f, 1.0f, 1.0f, orange, 1.0f, 0.0f}, {-1.0f, 1.0f, 1.0f, green, 0.0f, 0.0f}, {-1.0f, -1.0f, 1.0f, violet, 0.0f, 1.0f}, {1.0f, -1.0f, 1.0f, yellow, 1.0f, 1.0f}, // front face
		{1.0f, -1.0f, -1.0f, cyan, 0.0f, 1.0f}, {-1.0f, -1.0f, -1.0f, white, 1.0f, 1.0f}, {-1.0f, 1.0f, -1.0f, blue, 1.0f, 0.0f}, {1.0f, 1.0f, -1.0f, red, 0.0f, 0.0f}, //back face
		{-1.0f, 1.0f, 1.0f, green, 1.0f, 0.0f}, {-1.0f, 1.0f, -1.0f, blue, 0.0f, 0.0f}, {-1.0f, -1.0f, -1.0f, white, 0.0f, 1.0f}, {-1.0f, -1.0f, 1.0f, violet, 1.0f, 1.0f}, //left face
		{1.0f, 1.0f, -1.0f, red, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, orange, 0.0f, 0.0f}, {1.0f, -1.0f, 1.0f, yellow, 0.0f, 1.0f}, {1.0f, -1.0f, -1.0f, cyan, 1.0f, 1.0f}, //right face

	};
	
	vector<Vertex> data2;
	data2.push_back({ 0.0f, 0.0f, 0.0f, white, 0.0f, 0.0f });
	double step = 2*M_PI / 360;
	double j = 0.0f;
	for (int i = 1; i <= 361; i++, j += step)
	{
		Vertex v = { cos(j), sin(j), 0.0f, RGB::r(j), RGB::g(j), RGB::b(j), 1.0f, 0.0f, 0.0f};
		data2.push_back(v);
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBO); // Привязываем вершинный буфер
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW); // Загружаем данные в буфер
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Отвязываем вершинный буфер
	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * data2.size(), data2.data(), GL_STATIC_DRAW); // Загружаем данные в буфер
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Отвязываем вершинный буфер
	checkOpenGLerror();
}

void InitTextures()
{
	glGenTextures(1, &texture1); // Генерируем текстуру
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1); // Привязываем текстуру
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Устанавливаем параметры текстуры
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height, channels; // Загружаем текстуру
	unsigned char* data = stbi_load("miks.jpg", &width, &height, &channels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data); // Освобождаем память

	glGenTextures(1, &texture2); // Генерируем текстуру
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2); // Привязываем текстуру
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Устанавливаем параметры текстуры
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	data = stbi_load("wall.jpg", &width, &height, &channels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data); // Освобождаем память
}

void LoadAttrib(GLuint prog, GLint& attrib, const char* attr_name)
{
	attrib = glGetAttribLocation(prog, attr_name);
	if (attrib == -1)
	{
		std::cout << "could not bind attrib " << attr_name << std::endl;
		return;
	}
}

void LoadUniform(GLuint prog, GLint& attrib, const char* attr_name)
{
	attrib = glGetUniformLocation(prog, attr_name);
	if (attrib == -1)
	{
		std::cout << "could not bind uniform " << attr_name << std::endl;
		return;
	}
}

void InitShader()
{
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vShader, 1, &VertexShaderSource, NULL);
	glCompileShader(vShader);
	std::cout << "vertex shader \n";
	ShaderLog(vShader);

	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fShader, 1, &FragShaderSource, NULL);
	glCompileShader(fShader);
	std::cout << "fragment shader \n";
	ShaderLog(fShader);

	GLuint texVshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(texVshader, 1, &TexVShader, NULL);
	glCompileShader(texVshader);
	std::cout << "texture vertex shader \n";
	ShaderLog(texVshader);

	GLuint texColFshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(texColFshader, 1, &TexColorFshader, NULL);
	glCompileShader(texColFshader);
	std::cout << "texture color fragment shader \n";
	ShaderLog(texColFshader);

	GLuint texTexshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(texTexshader, 1, &TexTextureFshader, NULL);
	glCompileShader(texTexshader);
	std::cout << "texture texture fragment shader \n";
	ShaderLog(texTexshader);

	GLuint pieVShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(pieVShader, 1, &PieVShader, NULL);
	glCompileShader(pieVShader);
	std::cout << "pie vertex shader" << std::endl;
	ShaderLog(pieVShader);

	GLuint pieFShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(pieFShader, 1, &PieFShader, NULL);
	glCompileShader(pieFShader);
	std::cout << "pie fragment shader" << std::endl;
	ShaderLog(pieFShader);

	// Создаем шейдерную программу
	Task1 = glCreateProgram();
	Task2 = glCreateProgram();
	Task3 = glCreateProgram();
	Task4 = glCreateProgram();

	// Прикрепляем шейдеры к программе
	glAttachShader(Task1, vShader);
	glAttachShader(Task1, fShader);

	glAttachShader(Task2, texVshader);
	glAttachShader(Task2, texColFshader);

	glAttachShader(Task3, texVshader);
	glAttachShader(Task3, texTexshader);

	glAttachShader(Task4, pieVShader);
	glAttachShader(Task4, pieFShader);

	// Линкуем шейдерную программу
	glLinkProgram(Task1);
	glLinkProgram(Task2);
	glLinkProgram(Task3);
	glLinkProgram(Task4);

	int link1, link2, link3, link4;
	glGetProgramiv(Task1, GL_LINK_STATUS, &link1);
	glGetProgramiv(Task2, GL_LINK_STATUS, &link2);
	glGetProgramiv(Task3, GL_LINK_STATUS, &link3);
	glGetProgramiv(Task4, GL_LINK_STATUS, &link4);

	// Проверяем на ошибки
	if (!link1 || !link2 || !link3 || !link4)
	{
		std::cout << "error attach shaders \n";
		return;
	}

	LoadAttrib(Task1, A1_vertex, "coord");
	LoadAttrib(Task1, A1_color, "color");
	LoadUniform(Task1, U1_affine, "affine");
	LoadUniform(Task1, U1_proj, "proj");

	LoadAttrib(Task2, A2_vertex, "position");
	LoadAttrib(Task2, A2_color, "color");
	LoadAttrib(Task2, A2_texCoord, "texCoord");
	LoadUniform(Task2, U2_affine, "affine");
	LoadUniform(Task2, U2_proj, "proj");
	LoadUniform(Task2, U2_mix_value, "mixValue");

	LoadAttrib(Task3, A3_vertex, "position");
	//LoadAttrib(Task3, A3_color, "color");
	LoadAttrib(Task3, A3_texCoord, "texCoord");
	LoadUniform(Task3, U3_affine, "affine");
	LoadUniform(Task3, U3_proj, "proj");
	LoadUniform(Task3, U3_mix_value, "mixValue");

	LoadAttrib(Task4, A4_vertex, "coord");
	LoadAttrib(Task4, A4_color, "color");
	LoadUniform(Task4, U4_affine, "affine");
	LoadUniform(Task4, U4_proj, "proj");
	LoadUniform(Task4, U4_scale, "scale");
	checkOpenGLerror();
}

void Draw(sf::Window& window)
{
	switch (shapetype)
	{
		case ShapeType::Gradient_Tetrahedron:
			window.setTitle("Gradient Tetrahedron");
			glUseProgram(Task1);
			glUniformMatrix4fv(U1_affine, 1, GL_FALSE, glm::value_ptr(affine));
			glUniformMatrix4fv(U1_proj, 1, GL_FALSE, glm::value_ptr(proj));
			glEnableVertexAttribArray(A1_vertex);
			glEnableVertexAttribArray(A1_color);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glVertexAttribPointer(A1_vertex, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
			glVertexAttribPointer(A1_color, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLES, 0, 12);
			glDisableVertexAttribArray(A1_vertex);
			glDisableVertexAttribArray(A1_color);
			glUseProgram(0);
			break;
		case ShapeType::Gradient_Texture_Cube:
			window.setTitle("Gradient & Texture Cube");
			glUseProgram(Task2);
			glUniformMatrix4fv(U2_affine, 1, GL_FALSE, glm::value_ptr(affine));
			glUniformMatrix4fv(U2_proj, 1, GL_FALSE, glm::value_ptr(proj));
			glUniform1f(U2_mix_value, mix_value);
			glUniform1i(glGetUniformLocation(Task2, "ourTexture"), 0);
			glEnableVertexAttribArray(A2_vertex);
			glEnableVertexAttribArray(A2_color);
			glEnableVertexAttribArray(A2_texCoord);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glVertexAttribPointer(A2_vertex, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
			glVertexAttribPointer(A2_color, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
			glVertexAttribPointer(A2_texCoord, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(7 * sizeof(GLfloat)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_QUADS, 12, 24);
			glDisableVertexAttribArray(A2_vertex);
			glDisableVertexAttribArray(A2_color);
			glDisableVertexAttribArray(A2_texCoord);
			glUseProgram(0);
			break;
		
		case ShapeType::Double_Texture_Cube:
			window.setTitle("Double Texture Cube");
			glUseProgram(Task3);
			glUniformMatrix4fv(U3_affine, 1, GL_FALSE, glm::value_ptr(affine));
			glUniformMatrix4fv(U3_proj, 1, GL_FALSE, glm::value_ptr(proj));
			glUniform1f(U3_mix_value, mix_value);
			glUniform1i(glGetUniformLocation(Task3, "ourTexture1"), 0);
			glUniform1i(glGetUniformLocation(Task3, "ourTexture2"), 1);
			glEnableVertexAttribArray(A3_vertex);
			//glEnableVertexAttribArray(A3_color);
			glEnableVertexAttribArray(A3_texCoord);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glVertexAttribPointer(A3_vertex, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
			//glVertexAttribPointer(A3_color, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
			glVertexAttribPointer(A3_texCoord, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(7 * sizeof(GLfloat)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_QUADS, 12, 24);
			glDisableVertexAttribArray(A3_vertex);
			//glDisableVertexAttribArray(A3_color);
			glDisableVertexAttribArray(A3_texCoord);
			glUseProgram(0);
			break;
		
		case ShapeType::Gradient_Pie:
			window.setTitle("Gradient Pie");
			glUseProgram(Task4);
			glUniformMatrix4fv(U4_affine, 1, GL_FALSE, glm::value_ptr(affine));
			glUniformMatrix4fv(U4_proj, 1, GL_FALSE, glm::value_ptr(proj));
			glUniformMatrix4fv(U4_scale, 1, GL_FALSE, glm::value_ptr(pie_scale));
			glEnableVertexAttribArray(A4_vertex);
			glEnableVertexAttribArray(A4_color);
			glBindBuffer(GL_ARRAY_BUFFER, VBO2);
			glVertexAttribPointer(A4_vertex, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
			glVertexAttribPointer(A4_color, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 362);
			glDisableVertexAttribArray(A4_vertex);
			glDisableVertexAttribArray(A4_color);
			glUseProgram(0);
			break;
		default:
			break;
	}
	glUseProgram(0); // Отключаем шейдерную программу
	checkOpenGLerror(); // Проверяем на ошибки
}

void Release()
{
	ReleaseShader(); // Очищаем шейдеры
	ReleaseVBO(); // Очищаем буфер
}

void ReleaseVBO()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Отвязываем буфер
	glDeleteBuffers(1, &VBO); // Удаляем буфер
}

void ReleaseShader()
{
	glUseProgram(0); // Отключаем шейдерную программу
	glDeleteProgram(Task1); // Удаляем шейдерную программу
}

void ShaderLog(unsigned int shader)
{
	int infologLen = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 1)
	{
		int charsWritten = 0;
		std::vector<char> infoLog(infologLen);
		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog.data());
		std::cout << "InfoLog: " << infoLog.data() << std::endl;
	}
}

void checkOpenGLerror()
{
	GLenum errCode;
	const GLubyte* errString;
	if ((errCode = glGetError()) != GL_NO_ERROR)
	{
		errString = gluErrorString(errCode);
		std::cout << "OpenGL error: " << errString << std::endl;
	}
}

void SetIcon(sf::Window& wnd)
{
	sf::Image image;
	image.create(16, 16);
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			image.setPixel(i, j, { (uint8_t)(i * 16), (uint8_t)(j * 16), 0 });
		}
	}

	wnd.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
}
