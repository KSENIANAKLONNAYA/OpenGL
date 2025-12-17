// main.cpp
#include "Utils.h"
#include <gl/GL.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ---------- Камера ----------
class Camera {
public:
    glm::vec3 position{ 0.0f, 0.0f, 5.0f };
    float yaw = -90.0f; // горизонтальный угол
    float pitch = 0.0f;   // вертикальный угол
    float speed = 5.0f;   // м/с
    float sensitivity = 0.1f;

    glm::mat4 getViewMatrix() const {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0, 1, 0));
    }

    void processKeyboard(const sf::Keyboard::Key key, float delta) {
        float velocity = speed * delta;
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        if (key == sf::Keyboard::Key::W) position += front * velocity;
        if (key == sf::Keyboard::Key::S) position -= front * velocity;
        if (key == sf::Keyboard::Key::A) position -= right * velocity;
        if (key == sf::Keyboard::Key::D) position += right * velocity;
        if (key == sf::Keyboard::Key::Space) position += up * velocity;
        if (key == sf::Keyboard::Key::LShift) position -= up * velocity;
    }

    void processMouse(float dx, float dy) {
        yaw += dx * sensitivity;
        pitch += dy * sensitivity;
        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }
};

// ---------- Шейдеры (простейший) ----------
const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 Normal;
out vec3 FragPos;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = proj * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos = vec3(5.0,5.0,5.0);
uniform vec3 viewPos;
uniform vec3 lightColor = vec3(1.0,1.0,1.0);
uniform vec3 objectColor = vec3(0.8,0.5,0.3);

void main()
{
    // ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

// ---------- Утилита создания шейдера ----------
GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return shader;
}

// ---------- main ----------
int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "OBJ Viewer",
        sf::State::Windowed, settings);
    window.setMouseCursorVisible(false);
    window.setFramerateLimit(60);

    // Активируем контекст OpenGL
    window.setActive(true);

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << std::hex << err << std::dec << std::endl;
    }

    // Проверяем версию OpenGL
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // ----- OpenGL init -----
    glEnable(GL_DEPTH_TEST);

    // ----- Шейдерная программа -----
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // проверка линковки
    GLint linked; glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cerr << "Program link error: " << log << "\n";
        return -1;
    }

    // ----- Загрузка модели -----
    Mesh model;
    if (!loadOBJ("model.obj", model)) {
        std::cerr << "Failed to load OBJ.\n";
        return -1;
    }
    model.uploadToGPU();

    // ----- Uniform locations -----
    GLint locModel = glGetUniformLocation(shaderProgram, "model");
    GLint locView = glGetUniformLocation(shaderProgram, "view");
    GLint locProj = glGetUniformLocation(shaderProgram, "proj");
    GLint locViewPos = glGetUniformLocation(shaderProgram, "viewPos");

    // ----- Камера -----
    Camera cam;
    sf::Clock deltaClock;
    sf::Vector2i lastMousePos = sf::Mouse::getPosition(window);
    bool firstMouse = true;

    // ----- Главный цикл -----
    while (window.isOpen()) {
        float deltaTime = deltaClock.restart().asSeconds();

        // ----- События -----
        for (std::optional<sf::Event> event = window.pollEvent(); event.has_value(); event = window.pollEvent())
        {
            if (event->is < sf::Event::Closed>())
                window.close();
            if (const auto* keyPressed = event->getIf < sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                window.close();

                // ----- Клавиатурное управление камерой -----
                if (keyPressed->code == sf::Keyboard::Key::W)  cam.processKeyboard(sf::Keyboard::Key::W, deltaTime);
                if (keyPressed->code == sf::Keyboard::Key::S) cam.processKeyboard(sf::Keyboard::Key::S, deltaTime);
                if (keyPressed->code == sf::Keyboard::Key::A)  cam.processKeyboard(sf::Keyboard::Key::A, deltaTime);
                if (keyPressed->code == sf::Keyboard::Key::D)  cam.processKeyboard(sf::Keyboard::Key::D, deltaTime);
                if (keyPressed->code == sf::Keyboard::Key::Space)   cam.processKeyboard(sf::Keyboard::Key::Space, deltaTime);
                if (keyPressed->code == sf::Keyboard::Key::LShift)  cam.processKeyboard(sf::Keyboard::Key::LShift, deltaTime);
            }
        }

        // ----- Мышиное вращение -----
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (firstMouse) {
            lastMousePos = mousePos;
            firstMouse = false;
        }
        float dx = static_cast<float>(mousePos.x - lastMousePos.x);
        float dy = static_cast<float>(lastMousePos.y - mousePos.y); // инвертируем Y
        lastMousePos = mousePos;
        cam.processMouse(dx, dy);

        // ----- Рендеринг -----
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // матрицы
        glm::mat4 modelMat = glm::mat4(1.0f);
        glm::mat4 viewMat = cam.getViewMatrix();
        glm::mat4 projMat = glm::perspective(glm::radians(45.0f),
            static_cast<float>(window.getSize().x) /
            static_cast<float>(window.getSize().y),
            0.1f, 100.0f);

        glUniformMatrix4fv(locModel, 1, GL_FALSE, &modelMat[0][0]);
        glUniformMatrix4fv(locView, 1, GL_FALSE, &viewMat[0][0]);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, &projMat[0][0]);
        glUniform3fv(locViewPos, 1, &cam.position[0]);

        model.draw();

        glUseProgram(0);
        window.display();
    }

    // ----- Очистка -----
    glDeleteVertexArrays(1, &model.vao);
    glDeleteBuffers(1, &model.vbo);
    glDeleteBuffers(1, &model.ebo);
    glDeleteProgram(shaderProgram);
    return 0;
}