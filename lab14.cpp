// main.cpp
#include "Utils.h"
#include <gl/GL.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// ---------- Камера ----------
class Camera {
public:
    glm::vec3 position{ 0.0f, 0.0f, 5.0f };
    float yaw = -90.0f; // горизонтальный угол
    float pitch = 0.0f;   // вертикальный угол

    glm::mat4 getViewMatrix() const {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0, 1, 0));
    }
};

// ---------- Структура объекта сцены ----------
struct SceneObject {
    Mesh mesh;
    GLuint shaderProgram;
    GLuint textureID;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

    // Uniform locations для этого шейдера
    GLint locModel;
    GLint locView;
    GLint locProj;
    GLint locViewPos;
    GLint locTexture;

    SceneObject() : shaderProgram(0), textureID(0) {}
};

// ---------- Шейдеры ----------
// Шейдер 1: Освещение Фонга с текстурой
const char* vertexShaderSrc1 = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoord;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = proj * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSrc1 = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightPos = vec3(5.0,5.0,5.0);
uniform vec3 viewPos;
uniform vec3 lightColor = vec3(1.0,1.0,1.0);
uniform sampler2D texSampler;

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

    // texture
    vec3 texColor = texture(texSampler, TexCoord).rgb;
    
    vec3 result = (ambient + diffuse + specular) * texColor;
    FragColor = vec4(result, 1.0);
}
)";

// Шейдер 2: Простой цветной шейдер с текстурой
const char* vertexShaderSrc2 = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = proj * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSrc2 = R"(
#version 330 core
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texSampler;
uniform vec3 tintColor = vec3(0.8, 0.8, 1.0);

void main()
{
    vec3 texColor = texture(texSampler, TexCoord).rgb;
    FragColor = vec4(texColor * tintColor, 1.0);
}
)";

// Шейдер 3: Toon shading
const char* vertexShaderSrc3 = R"(
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

const char* fragmentShaderSrc3 = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos = vec3(5.0,5.0,5.0);
uniform vec3 viewPos;
uniform vec3 objectColor = vec3(0.8,0.3,0.3);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Toon shading эффект
    float intensity = diff;
    if (intensity > 0.8) intensity = 1.0;
    else if (intensity > 0.5) intensity = 0.6;
    else if (intensity > 0.2) intensity = 0.3;
    else intensity = 0.1;
    
    vec3 result = intensity * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

// Шейдер 4: (Придумать)
const char* vertexShaderSrc4 = R"(
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

const char* fragmentShaderSrc4 = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos = vec3(5.0,5.0,5.0);
uniform vec3 viewPos;
uniform vec3 objectColor = vec3(0.8,0.3,0.3);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Toon shading эффект
    float intensity = diff;
    if (intensity > 0.8) intensity = 1.0;
    else if (intensity > 0.5) intensity = 0.6;
    else if (intensity > 0.2) intensity = 0.3;
    else intensity = 0.1;
    
    vec3 result = intensity * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

// Шейдер 5: (Придумать)
const char* vertexShaderSrc5 = R"(
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

const char* fragmentShaderSrc5 = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos = vec3(5.0,5.0,5.0);
uniform vec3 viewPos;
uniform vec3 objectColor = vec3(0.8,0.3,0.3);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Toon shading эффект
    float intensity = diff;
    if (intensity > 0.8) intensity = 1.0;
    else if (intensity > 0.5) intensity = 0.6;
    else if (intensity > 0.2) intensity = 0.3;
    else intensity = 0.1;
    
    vec3 result = intensity * objectColor;
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

// ---------- Утилита создания шейдерной программы ----------
GLuint createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint linked; glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Program link error: " << log << "\n";
        return 0;
    }
    return program;
}

// ---------- Утилита загрузки текстуры ----------
GLuint loadTexture(const char* path) {
    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    image.flipVertically(); // OpenGL ожидает текстуру вверх ногами

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y,
        0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}

// ---------- main ----------
int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Multi-Object OBJ Viewer",
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

    // ----- Создаем несколько объектов сцены -----
    std::vector<SceneObject> sceneObjects;

    // Объект 1: Сфера с первым шейдером
    {
        SceneObject obj;

        // Загружаем модель
        if (!loadOBJ("Objects/MinFlatSphere.obj", obj.mesh)) {
            std::cerr << "Failed to load sphere OBJ.\n";
        }
        else {
            obj.mesh.uploadToGPU();

            // Создаем шейдер
            obj.shaderProgram = createShaderProgram(vertexShaderSrc1, fragmentShaderSrc1);

            // Загружаем текстуру
            //obj.textureID = loadTexture("Textures/stone_texture.png"); // Предполагается наличие текстуры

            // Настройки трансформации
            obj.position = glm::vec3(-2.0f, 0.0f, 0.0f);
            obj.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            obj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");
            obj.locTexture = glGetUniformLocation(obj.shaderProgram, "texSampler");

            sceneObjects.push_back(obj);
        }
    }

    // Объект 2: Вторая сфера со вторым шейдером
    {
        SceneObject obj;

        // Загружаем модель
        if (!loadOBJ("Objects/MinFlatSphere.obj", obj.mesh)) {
            std::cerr << "Failed to load second sphere OBJ.\n";
        }
        else {
            obj.mesh.uploadToGPU();

            // Создаем шейдер
            obj.shaderProgram = createShaderProgram(vertexShaderSrc2, fragmentShaderSrc2);

            // Загружаем другую текстуру
            //obj.textureID = loadTexture("Textures/metal_texture.png"); // Предполагается наличие текстуры

            // Настройки трансформации
            obj.position = glm::vec3(0.0f, 0.0f, 0.0f);
            obj.scale = glm::vec3(0.8f, 0.8f, 0.8f);
            obj.rotation = glm::vec3(0.0f, 45.0f, 0.0f);

            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locTexture = glGetUniformLocation(obj.shaderProgram, "texSampler");

            sceneObjects.push_back(obj);
        }
    }

    // Объект 3: Невыпуклый куб с третьим шейдером (без текстуры)
    {
        SceneObject obj;

        // Загружаем модель
        if (!loadOBJ("Objects/MinFlatSphere.obj", obj.mesh)) {
            std::cerr << "Failed to load test cube OBJ.\n";
        }
        else {
            obj.mesh.uploadToGPU();

            // Создаем шейдер (третий шейдер не использует текстуру)
            obj.shaderProgram = createShaderProgram(vertexShaderSrc3, fragmentShaderSrc3);

            // Настройки трансформации
            obj.position = glm::vec3(2.0f, 0.0f, 0.0f);
            obj.scale = glm::vec3(1.2f, 1.2f, 1.2f);
            obj.rotation = glm::vec3(0.0f, 90.0f, 0.0f);

            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");

            sceneObjects.push_back(obj);
        }
    }

    // Объект 4: (Придумать)
    {
        SceneObject obj;

        // Загружаем модель
        if (!loadOBJ("Objects/MinFlatSphere.obj", obj.mesh)) {
            std::cerr << "Failed to load third sphere OBJ.\n";
        }
        else {
            obj.mesh.uploadToGPU();

            // Создаем шейдер (третий шейдер не использует текстуру)
            obj.shaderProgram = createShaderProgram(vertexShaderSrc3, fragmentShaderSrc3);

            // Настройки трансформации
            obj.position = glm::vec3(0.0f, -0.4f, 2.0f);
            obj.scale = glm::vec3(0.1f, 0.1f, 0.1f);
            obj.rotation = glm::vec3(0.0f, 90.0f, 0.0f);

            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");

            sceneObjects.push_back(obj);
        }
    }

    // Объект 5: (Придумать)
    {
        SceneObject obj;

        // Загружаем модель
        if (!loadOBJ("Objects/MinFlatSphere.obj", obj.mesh)) {
            std::cerr << "Failed to load third sphere OBJ.\n";
        }
        else {
            obj.mesh.uploadToGPU();

            // Создаем шейдер (третий шейдер не использует текстуру)
            obj.shaderProgram = createShaderProgram(vertexShaderSrc3, fragmentShaderSrc3);

            // Настройки трансформации
            obj.position = glm::vec3(-1.0f, -0.5f, 1.0f);
            obj.scale = glm::vec3(0.2f, 0.2f, 0.2f);
            obj.rotation = glm::vec3(0.0f, 90.0f, 0.0f);

            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");

            sceneObjects.push_back(obj);
        }
    }

    // Проверяем, что есть хотя бы один объект
    if (sceneObjects.empty()) {
        std::cerr << "No objects loaded!\n";
        return -1;
    }

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
            }
        }

        // ----- Обновление анимации -----
        static float time = 0.0f;
        time += deltaTime;

        //// Анимируем объекты
        //if (sceneObjects.size() > 0) {
        //    sceneObjects[0].rotation.y = time * 30.0f; // Вращение первого объекта
        //}
        //if (sceneObjects.size() > 1) {
        //    sceneObjects[1].position.y = sin(time) * 0.5f; // Покачивание второго объекта
        //}
        //if (sceneObjects.size() > 2) {
        //    sceneObjects[2].scale.x = 1.0f + sin(time * 1.5f) * 0.2f; // Пульсация третьего объекта
        //    sceneObjects[2].scale.y = 1.0f + sin(time * 1.5f) * 0.2f;
        //    sceneObjects[2].scale.z = 1.0f + sin(time * 1.5f) * 0.2f;
        //}

        // ----- Рендеринг -----
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Матрицы камеры (одинаковые для всех объектов)
        glm::mat4 viewMat = cam.getViewMatrix();
        glm::mat4 projMat = glm::perspective(glm::radians(45.0f),
            static_cast<float>(window.getSize().x) /
            static_cast<float>(window.getSize().y),
            0.1f, 100.0f);

        // Рендерим каждый объект
        for (auto& obj : sceneObjects) {
            glUseProgram(obj.shaderProgram);

            // Создаем матрицу модели для этого объекта
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::translate(modelMat, obj.position);
            modelMat = glm::rotate(modelMat, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::rotate(modelMat, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMat = glm::rotate(modelMat, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            modelMat = glm::scale(modelMat, obj.scale);

            // Передаем uniform-переменные
            glUniformMatrix4fv(obj.locModel, 1, GL_FALSE, &modelMat[0][0]);
            glUniformMatrix4fv(obj.locView, 1, GL_FALSE, &viewMat[0][0]);
            glUniformMatrix4fv(obj.locProj, 1, GL_FALSE, &projMat[0][0]);

            // Передаем позицию камеры (если используется в шейдере)
            if (obj.locViewPos != -1) {
                glUniform3fv(obj.locViewPos, 1, &cam.position[0]);
            }

            // Привязываем текстуру (если есть)
            if (obj.textureID != 0 && obj.locTexture != -1) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, obj.textureID);
                glUniform1i(obj.locTexture, 0);
            }

            // Рисуем объект
            obj.mesh.draw();
        }

        glUseProgram(0);
        window.display();
    }

    // ----- Очистка -----
    for (auto& obj : sceneObjects) {
        if (obj.textureID != 0) {
            glDeleteTextures(1, &obj.textureID);
        }
        if (obj.shaderProgram != 0) {
            glDeleteProgram(obj.shaderProgram);
        }
        obj.mesh.cleanup();
    }

    return 0;
}