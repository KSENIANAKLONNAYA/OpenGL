#include "Utils.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>

// Структура для объекта сцены
struct SceneObject {
    Mesh mesh;
    GLuint textureID;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
    std::string name;
    int lightingModel; // 0=Phong, 1=Toon, 2=Oren-Nayar
    
    SceneObject() : textureID(0), lightingModel(0) {}
};

// Типы источников света
enum LightType {
    LIGHT_POINT,      // Точечный источник
    LIGHT_DIRECTIONAL, // Направленный источник
    LIGHT_SPOT        // Прожекторный источник
};

// Типы моделей освещения
enum LightingModel {
    PHONG_MODEL = 0,
    TOON_MODEL = 1,
    OREN_NAYAR_MODEL = 2
};

// Структура для источника света
struct Light {
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
    bool enabled;
    std::string name;
    
    Light() : type(LIGHT_POINT), position(0.0f, 5.0f, 0.0f), 
              direction(0.0f, -1.0f, 0.0f), color(1.0f), 
              intensity(1.0f), cutOff(12.5f), outerCutOff(15.0f), 
              enabled(true), name("Light") {}
};

// Шейдеры с поддержкой трех моделей освещения
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// Структуры для источников света
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    bool enabled;
    
    // Параметры затухания
    float constant;
    float linear;
    float quadratic;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
    bool enabled;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    bool enabled;
    
    // Параметры конуса
    float cutOff;
    float outerCutOff;
    
    // Параметры затухания
    float constant;
    float linear;
    float quadratic;
};

uniform sampler2D texture1;
uniform vec3 viewPos;

// Максимальное количество источников каждого типа
#define MAX_POINT_LIGHTS 1
#define MAX_DIR_LIGHTS 1
#define MAX_SPOT_LIGHTS 1

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform DirectionalLight dirLights[MAX_DIR_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

// Параметры моделей освещения
uniform int lightingModel; // 0=Phong, 1=Toon, 2=Oren-Nayar
uniform float roughness;   // Шероховатость для Oren-Nayar (0.0-1.0)
uniform int toonBands;     // Количество градаций для Toon (2-10)
uniform float specularPower; // Мощность блика для Phong

// Функции для разных моделей освещения

// 1. Модель Phong
vec3 phongLighting(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 lightColor, float intensity, vec3 diffuseColor) {
    // Диффузная составляющая
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightColor * diff * diffuseColor * intensity;
    
    // Зеркальная составляющая
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower);
    vec3 specular = lightColor * spec * intensity;
    
    return diffuse + specular;
}

// 2. Toon Shading (Cel Shading)
vec3 toonLighting(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 lightColor, float intensity, vec3 diffuseColor) {
    // Квантование диффузной составляющей
    float diff = max(dot(normal, lightDir), 0.0);
    float toonDiff = floor(diff * toonBands) / float(toonBands);
    vec3 diffuse = lightColor * toonDiff * diffuseColor * intensity;
    
    // Резкие блики для Toon
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = dot(viewDir, reflectDir);
    
    // Резкий блик (либо есть, либо нет)
    vec3 specular = vec3(0.0);
    if (spec > 0.95) {
        specular = lightColor * 0.8 * intensity;
    } else if (spec > 0.5) {
        specular = lightColor * 0.3 * intensity;
    }
    
    // Обводка (rim lighting)
    float rim = 1.0 - max(dot(normal, viewDir), 0.0);
    if (rim > 0.7) {
        diffuse += lightColor * 0.3 * intensity;
    }
    
    return diffuse + specular;
}

// 3. Модель Oren-Nayar (для матовых поверхностей)
vec3 orenNayarLighting(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 lightColor, float intensity, vec3 diffuseColor) {
    float roughness2 = roughness * roughness;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    float angleVN = acos(NdotV);
    float angleLN = acos(NdotL);
    
    float alpha = max(angleVN, angleLN);
    float beta = min(angleVN, angleLN);
    float gamma = dot(viewDir - normal * NdotV, lightDir - normal * NdotL);
    
    float A = 1.0 - 0.5 * (roughness2 / (roughness2 + 0.33));
    float B = 0.45 * (roughness2 / (roughness2 + 0.09));
    
    float C = sin(alpha) * tan(beta);
    
    float L1 = max(0.0, NdotL) * (A + B * max(0.0, gamma) * C);
    
    return lightColor * L1 * diffuseColor * intensity;
}

// Общие функции расчета для каждого типа света
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
{
    if (!light.enabled) return vec3(0.0);
    
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 result = vec3(0.0);
    
    if (lightingModel == 0) { // Phong
        result = phongLighting(lightDir, normal, viewDir, light.color, light.intensity, diffuseColor);
    } else if (lightingModel == 1) { // Toon
        result = toonLighting(lightDir, normal, viewDir, light.color, light.intensity, diffuseColor);
    } else if (lightingModel == 2) { // Oren-Nayar
        result = orenNayarLighting(lightDir, normal, viewDir, light.color, light.intensity, diffuseColor);
    }
    
    // Затухание
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                              light.quadratic * (distance * distance));
    
    return result * attenuation;
}

vec3 calcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor)
{
    if (!light.enabled) return vec3(0.0);
    
    vec3 lightDir = normalize(-light.direction);
    vec3 result = vec3(0.0);
    
    if (lightingModel == 0) { // Phong
        result = phongLighting(lightDir, normal, viewDir, light.color, light.intensity, diffuseColor);
    } else if (lightingModel == 1) { // Toon
        result = toonLighting(lightDir, normal, viewDir, light.color, light.intensity, diffuseColor);
    } else if (lightingModel == 2) { // Oren-Nayar
        result = orenNayarLighting(lightDir, normal, viewDir, light.color, light.intensity, diffuseColor);
    }
    
    return result;
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
{
    if (!light.enabled) return vec3(0.0);
    
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Проверка нахождения внутри конуса
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    if (theta > light.outerCutOff) {
        vec3 result = vec3(0.0);
        
        if (lightingModel == 0) { // Phong
            result = phongLighting(lightDir, normal, viewDir, light.color, light.intensity * intensity, diffuseColor);
        } else if (lightingModel == 1) { // Toon
            result = toonLighting(lightDir, normal, viewDir, light.color, light.intensity * intensity, diffuseColor);
        } else if (lightingModel == 2) { // Oren-Nayar
            result = orenNayarLighting(lightDir, normal, viewDir, light.color, light.intensity * intensity, diffuseColor);
        }
        
        // Затухание
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + 
                                  light.quadratic * (distance * distance));
        
        return result * attenuation;
    }
    
    return vec3(0.0);
}

void main()
{
    // Основные векторы
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Цвет текстуры
    vec3 diffuseColor = texture(texture1, TexCoord).rgb;
    
    // Фоновое освещение (ambient) - зависит от модели
    float ambientStrength;
    if (lightingModel == 1) { // Toon
        ambientStrength = 0.2; // Больше ambient для toon
    } else if (lightingModel == 2) { // Oren-Nayar
        ambientStrength = 0.15; // Среднее для матовых поверхностей
    } else { // Phong
        ambientStrength = 0.1;
    }
    vec3 ambient = ambientStrength * diffuseColor;
    
    // Результат освещения
    vec3 result = ambient;
    
    // Обработка всех точечных источников
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        result += calcPointLight(pointLights[i], norm, FragPos, viewDir, diffuseColor);
    }
    
    // Обработка всех направленных источников
    for (int i = 0; i < MAX_DIR_LIGHTS; i++) {
        result += calcDirectionalLight(dirLights[i], norm, viewDir, diffuseColor);
    }
    
    // Обработка всех прожекторных источников
    for (int i = 0; i < MAX_SPOT_LIGHTS; i++) {
        result += calcSpotLight(spotLights[i], norm, FragPos, viewDir, diffuseColor);
    }
    
    // Для Toon shading добавляем черные обводки
    if (lightingModel == 1) {
        // Проверяем угол между нормалью и направлением взгляда
        float edge = dot(norm, viewDir);
        if (edge < 0.3) {
            result = vec3(0.0, 0.0, 0.0); // Черные обводки
        }
    }
    
    FragColor = vec4(result, 1.0);
}
)";

GLuint shaderProgram = 0;
std::vector<Light> lights;
int currentLightIndex = 0;

// Параметры моделей освещения
float roughness = 0.5f;
int toonBands = 4;
float specularPower = 32.0f;

// Функция установки источников света в шейдер
void setupLightsInShader() {
    glUseProgram(shaderProgram);

    std::vector<Light> pointLights;
    std::vector<Light> dirLights;
    std::vector<Light> spotLights;
    
    for (const auto& light : lights) {
        if (!light.enabled) continue;
        
        switch (light.type) {
            case LIGHT_POINT:
                if (pointLights.size() < 1) pointLights.push_back(light);
                break;
            case LIGHT_DIRECTIONAL:
                if (dirLights.size() < 1) dirLights.push_back(light);
                break;
            case LIGHT_SPOT:
                if (spotLights.size() < 1) spotLights.push_back(light);
                break;
        }
    }
    
    // Установка точечных источников
    for (size_t i = 0; i < 1; i++) {
        std::string base = "pointLights[" + std::to_string(i) + "].";
        
        if (i < pointLights.size()) {
            const auto& light = pointLights[i];
            
            glUniform3f(glGetUniformLocation(shaderProgram, (base + "position").c_str()),
                       light.position.x, light.position.y, light.position.z);
            glUniform3f(glGetUniformLocation(shaderProgram, (base + "color").c_str()),
                       light.color.r, light.color.g, light.color.b);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "intensity").c_str()),
                       light.intensity);
            glUniform1i(glGetUniformLocation(shaderProgram, (base + "enabled").c_str()), 1);
            
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "linear").c_str()), 0.09f);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "quadratic").c_str()), 0.032f);
        } else {
            glUniform1i(glGetUniformLocation(shaderProgram, (base + "enabled").c_str()), 0);
        }
    }
    
    // Установка направленных источников
    for (size_t i = 0; i < 1; i++) {
        std::string base = "dirLights[" + std::to_string(i) + "].";
        
        if (i < dirLights.size()) {
            const auto& light = dirLights[i];
            
            glUniform3f(glGetUniformLocation(shaderProgram, (base + "direction").c_str()),
                       light.direction.x, light.direction.y, light.direction.z);
            glUniform3f(glGetUniformLocation(shaderProgram, (base + "color").c_str()),
                       light.color.r, light.color.g, light.color.b);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "intensity").c_str()),
                       light.intensity);
            glUniform1i(glGetUniformLocation(shaderProgram, (base + "enabled").c_str()), 1);
        } else {
            glUniform1i(glGetUniformLocation(shaderProgram, (base + "enabled").c_str()), 0);
        }
    }
    
    // Установка прожекторных источников
    for (size_t i = 0; i < 1; i++) {
        std::string base = "spotLights[" + std::to_string(i) + "].";
        
        if (i < spotLights.size()) {
            const auto& light = spotLights[i];
            
            glUniform3f(glGetUniformLocation(shaderProgram, (base + "position").c_str()),
                       light.position.x, light.position.y, light.position.z);
            glUniform3f(glGetUniformLocation(shaderProgram, (base + "direction").c_str()),
                       light.direction.x, light.direction.y, light.direction.z);
            glUniform3f(glGetUniformLocation(shaderProgram, (base + "color").c_str()),
                       light.color.r, light.color.g, light.color.b);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "intensity").c_str()),
                       light.intensity);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "cutOff").c_str()),
                       cos(glm::radians(light.cutOff)));
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "outerCutOff").c_str()),
                       cos(glm::radians(light.outerCutOff)));
            glUniform1i(glGetUniformLocation(shaderProgram, (base + "enabled").c_str()), 1);
            
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "linear").c_str()), 0.09f);
            glUniform1f(glGetUniformLocation(shaderProgram, (base + "quadratic").c_str()), 0.032f);
        } else {
            glUniform1i(glGetUniformLocation(shaderProgram, (base + "enabled").c_str()), 0);
        }
    }
    
    // Установка параметров моделей освещения
    glUniform1i(glGetUniformLocation(shaderProgram, "toonBands"), toonBands);
    glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), roughness);
    glUniform1f(glGetUniformLocation(shaderProgram, "specularPower"), specularPower);
}

// Функция инициализации источников света
void initLights() {
    Light pointLight;
    pointLight.type = LIGHT_POINT;
    pointLight.position = glm::vec3(0.0f, 5.0f, 0.0f);
    pointLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
    pointLight.intensity = 1.0f;
    pointLight.enabled = true;
    pointLight.name = "Точечный свет";
    lights.push_back(pointLight);
    
    Light dirLight;
    dirLight.type = LIGHT_DIRECTIONAL;
    dirLight.direction = glm::vec3(-0.5f, -1.0f, -0.5f);
    dirLight.color = glm::vec3(0.9f, 0.9f, 0.8f);
    dirLight.intensity = 0.5f;
    dirLight.enabled = false;
    dirLight.name = "Направленный свет";
    lights.push_back(dirLight);
    
    Light spotLight;
    spotLight.type = LIGHT_SPOT;
    spotLight.position = glm::vec3(2.0f, 3.0f, 2.0f);
    spotLight.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    spotLight.color = glm::vec3(1.0f, 0.9f, 0.8f);
    spotLight.intensity = 0.8f;
    spotLight.cutOff = 12.5f;
    spotLight.outerCutOff = 15.0f;
    spotLight.enabled = false;
    spotLight.name = "Прожектор";
    lights.push_back(spotLight);
}

void displayLightInfo(sf::Window& window, Light& light) {
    std::cout << "\033[2J\033[1;1H";
    
    std::cout << "=== НАСТРОЙКА ИСТОЧНИКОВ СВЕТА ===\n\n";
    
    for (size_t i = 0; i < lights.size(); i++) {
        std::string typeStr;
        switch (lights[i].type) {
            case LIGHT_POINT: typeStr = "Точечный"; break;
            case LIGHT_DIRECTIONAL: typeStr = "Направленный"; break;
            case LIGHT_SPOT: typeStr = "Прожекторный"; break;
        }
        
        if (i == currentLightIndex) {
            std::cout << ">>> " << (i+1) << ". " << typeStr << " \"" << lights[i].name << "\" <<<\n";
        } else {
            std::cout << "    " << (i+1) << ". " << typeStr << " \"" << lights[i].name << "\"\n";
        }
        
        std::cout << "    Включен: " << (lights[i].enabled ? "ДА" : "НЕТ") << "\n";
        std::cout << "    Цвет: (" << lights[i].color.r << ", " 
                  << lights[i].color.g << ", " << lights[i].color.b << ")\n";
        std::cout << "    Интенсивность: " << lights[i].intensity << "\n";
        
        if (lights[i].type != LIGHT_DIRECTIONAL) {
            std::cout << "    Позиция: (" << lights[i].position.x << ", " 
                      << lights[i].position.y << ", " << lights[i].position.z << ")\n";
        }
        
        if (lights[i].type != LIGHT_POINT) {
            std::cout << "    Направление: (" << lights[i].direction.x << ", " 
                      << lights[i].direction.y << ", " << lights[i].direction.z << ")\n";
        }
        
        if (lights[i].type == LIGHT_SPOT) {
            std::cout << "    Угол конуса: " << lights[i].cutOff << "° / " 
                      << lights[i].outerCutOff << "°\n";
        }
        
        std::cout << "\n";
    }
    
    std::cout << "=== МОДЕЛИ ОСВЕЩЕНИЯ ===\n";
    std::cout << "P - Phong (по умолчанию)\n";
    std::cout << "T - Toon Shading\n";
    std::cout << "O - Oren-Nayar\n";
    std::cout << "F/G - Изменить шероховатость (Oren-Nayar) +/- 0.1\n";
    std::cout << "H/J - Изменить количество градаций (Toon) +/- 1\n";
    std::cout << "B/N - Изменить мощность блика (Phong) +/- 4\n\n";
    
    std::cout << "=== УПРАВЛЕНИЕ СВЕТОМ ===\n";
    std::cout << "1, 2, 3 - Выбрать источник света\n";
    std::cout << "Space - Включить/выключить текущий источник\n";
    std::cout << "W/S - Изменить интенсивность (+/- 0.1)\n";
    std::cout << "A/D - Изменить цвет (R канал +/- 0.1)\n";
    std::cout << "Z/X - Изменить цвет (G канал +/- 0.1)\n";
    std::cout << "C/V - Изменить цвет (B канал +/- 0.1)\n";
    
    if (lights[currentLightIndex].type != LIGHT_DIRECTIONAL) {
        std::cout << "Стрелки влево/вправо - Изменить позицию X (+/- 0.5)\n";
        std::cout << "Стрелки вверх/вниз - Изменить позицию Z (+/- 0.5)\n";
        std::cout << "PageUp/PageDown - Изменить позицию Y (+/- 0.5)\n";
    }
    
    if (lights[currentLightIndex].type != LIGHT_POINT) {
        std::cout << "I/K - Изменить направление X (+/- 0.1)\n";
        std::cout << "J/L - Изменить направление Y (+/- 0.1)\n";
        std::cout << "U/O - Изменить направление Z (+/- 0.1)\n";
    }
    
    if (lights[currentLightIndex].type == LIGHT_SPOT) {
        std::cout << "M/, - Изменить внутренний угол конуса (+/- 1°)\n";
        std::cout << "./? - Изменить внешний угол конуса (+/- 1°)\n";
    }
    
    std::cout << "\n=== УПРАВЛЕНИЕ КАМЕРОЙ ===\n";
    std::cout << "WASD - Движение камеры\n";
    std::cout << "Стрелки - Движение камеры\n";
    std::cout << "R - Сбросить камеру\n";
    std::cout << "ESC - Выход из программы\n";
    std::cout << "====================\n";
}

bool compileShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции вершинного шейдера:\n" << infoLog << std::endl;
        return false;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции фрагментного шейдера:\n" << infoLog << std::endl;
        return false;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Ошибка линковки шейдерной программы:\n" << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

GLuint loadTexture(const std::string& path) {
    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "Не удалось загрузить текстуру: " << path << std::endl;
        return 0;
    }
    
    image.flipVertically();
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                 image.getSize().x, image.getSize().y,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                 image.getPixelsPtr());
    
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

int main() {
    std::cout << "=== ЛАБОРАТОРНАЯ РАБОТА: МОДЕЛИ ОСВЕЩЕНИЯ ===\n\n";
    
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 3;

    sf::Window window(sf::VideoMode(sf::Vector2u(1280, 720)), 
                    "3D Scene - Multiple Lighting Models", 
                    sf::State::Windowed, 
                    settings);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Ошибка инициализации GLEW: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
    
    if (!compileShaders()) {
        return -1;
    }
    
    initLights();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    
    std::vector<std::string> textureFiles = {
        "Textures/texture1.jpg",
        "Textures/texture2.jpg",
        "Textures/texture3.jpg",
        "Textures/texture4.jpg",
        "Textures/texture5.jpg"
    };
    
    std::vector<GLuint> textures;
    for (const auto& texFile : textureFiles) {
        GLuint tex = loadTexture(texFile);
        if (tex) {
            textures.push_back(tex);
        }
    }
    
    if (textures.empty()) {
        GLuint testTexture;
        glGenTextures(1, &testTexture);
        glBindTexture(GL_TEXTURE_2D, testTexture);
        unsigned char pixels[] = {255, 255, 255, 255, 200, 200, 200, 255, 
                                  150, 150, 150, 255, 100, 100, 100, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        textures.push_back(testTexture);
    }
    
    std::vector<SceneObject> sceneObjects;
    
    // 1. Шар - Phong (по умолчанию)
    {
        SceneObject obj;
        if (loadOBJ("Objects/SphereSmooth.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.textureID = textures.size() > 2 ? textures[2] : textures[0];
            obj.position = glm::vec3(-1.0f, 3.0f, 3.0f);
            obj.scale = glm::vec3(0.5f);
            obj.rotation = glm::vec3(0.0f);
            obj.name = "Шар";
            obj.lightingModel = PHONG_MODEL;
            sceneObjects.push_back(obj);
        }
    }
    
    // 2. Микки Маус - Toon Shading
    {
        SceneObject obj;
        if (loadOBJ("Objects/Mickey Mouse.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.textureID = textures.size() > 3 ? textures[3] : textures[0];
            obj.position = glm::vec3(0.0f, 0.0f, 0.0f);
            obj.scale = glm::vec3(0.010f);
            obj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            obj.name = "Микки Маус";
            obj.lightingModel = TOON_MODEL;
            sceneObjects.push_back(obj);
        }
    }
    
    // 3. Стол - Oren-Nayar (матовая поверхность)
    {
        SceneObject obj;
        bool loaded = false;
        if (!loaded && std::ifstream("Objects/table.obj")) {
            loaded = loadOBJ("Objects/table.obj", obj.mesh);
        }
        if (!loaded && std::ifstream("Objects/Table.obj")) {
            loaded = loadOBJ("Objects/Table.obj", obj.mesh);
        }
        
        if (loaded) {
            obj.mesh.uploadToGPU();
            obj.textureID = textures[0];
            obj.position = glm::vec3(3.0f, 0.5f, 2.5f);
            obj.scale = glm::vec3(0.99f);
            obj.rotation = glm::vec3(0.0f, 100.0f, -10.0f);
            obj.name = "Стол";
            obj.lightingModel = OREN_NAYAR_MODEL;
            sceneObjects.push_back(obj);
        }
    }
    
    // 4. Чайник - Phong
    {
        SceneObject obj;
        if (loadOBJ("Objects/utah_teapot_lowpoly.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.textureID = textures.size() > 4 ? textures[4] : textures[0];
            obj.position = glm::vec3(4.0f, 0.5f, 0.0f);
            obj.scale = glm::vec3(0.9f);
            obj.rotation = glm::vec3(0.0f, 45.0f, 0.0f);
            obj.name = "Чайник";
            obj.lightingModel = PHONG_MODEL;
            sceneObjects.push_back(obj);
        }
    }
    
    sf::Clock clock;
    bool running = true;
    
    glm::vec3 cameraPos = glm::vec3(0.0f, 2.5f, 8.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.5f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 initialCameraPos = cameraPos;
    glm::vec3 initialCameraTarget = cameraTarget;
    
    bool showInfo = true;
    
    while (running) {
        float deltaTime = clock.restart().asSeconds();
        
        for (auto event = window.pollEvent(); event.has_value(); event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                running = false;
            }
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                Light& currentLight = lights[currentLightIndex];
                
                // Используем keyPressed->code
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    running = false;
                }
                
                if (keyPressed->code == sf::Keyboard::Key::Num4) {  // Цифра 4
                    std::cout << "Phong модель (4)" << std::endl;
                    for (auto& obj : sceneObjects) {
                        obj.lightingModel = PHONG_MODEL;
                    }
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::Num5) {  // Цифра 5
                    std::cout << "Toon Shading (5)" << std::endl;
                    for (auto& obj : sceneObjects) {
                        obj.lightingModel = TOON_MODEL;
                    }
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::Num6) {  // Цифра 6
                    std::cout << "Oren-Nayar модель (6)" << std::endl;
                    for (auto& obj : sceneObjects) {
                        obj.lightingModel = OREN_NAYAR_MODEL;
                    }
                    showInfo = true;
                }
                
                // Параметры моделей
                if (keyPressed->code == sf::Keyboard::Key::F) {
                    roughness = std::max(0.0f, roughness - 0.1f);
                    std::cout << "Шероховатость: " << roughness << "\n";
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::G) {
                    roughness = std::min(1.0f, roughness + 0.1f);
                    std::cout << "Шероховатость: " << roughness << "\n";
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::H) {
                    toonBands = std::max(2, toonBands - 1);
                    std::cout << "Градаций Toon: " << toonBands << "\n";
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::J) {
                    toonBands = std::min(10, toonBands + 1);
                    std::cout << "Градаций Toon: " << toonBands << "\n";
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::B) {
                    specularPower = std::max(4.0f, specularPower - 4.0f);
                    std::cout << "Мощность блика: " << specularPower << "\n";
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::N) {
                    specularPower = std::min(256.0f, specularPower + 4.0f);
                    std::cout << "Мощность блика: " << specularPower << "\n";
                    showInfo = true;
                }
                
                // Выбор источника света
                if (keyPressed->code == sf::Keyboard::Key::Num1) {
                    currentLightIndex = 0;
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::Num2) {
                    currentLightIndex = 1;
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::Num3) {
                    currentLightIndex = 2;
                    showInfo = true;
                }
                
                // Включение/выключение
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    currentLight.enabled = !currentLight.enabled;
                    std::cout << "Источник " << (currentLight.enabled ? "включен" : "выключен") << "\n";
                    showInfo = true;
                }
                
                // Сброс камеры
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    cameraPos = initialCameraPos;
                    cameraTarget = initialCameraTarget;
                    std::cout << "Камера сброшена\n";
                    showInfo = true;
                }
                
                // Интенсивность
                if (keyPressed->code == sf::Keyboard::Key::W) {
                    currentLight.intensity += 0.1f;
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::S) {
                    currentLight.intensity = std::max(0.0f, currentLight.intensity - 0.1f);
                    showInfo = true;
                }
                
                // Цвет
                if (keyPressed->code == sf::Keyboard::Key::A) {
                    currentLight.color.r = std::min(1.0f, currentLight.color.r + 0.1f);
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::D) {
                    currentLight.color.r = std::max(0.0f, currentLight.color.r - 0.1f);
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::Z) {
                    currentLight.color.g = std::min(1.0f, currentLight.color.g + 0.1f);
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::X) {
                    currentLight.color.g = std::max(0.0f, currentLight.color.g - 0.1f);
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::C) {
                    currentLight.color.b = std::min(1.0f, currentLight.color.b + 0.1f);
                    showInfo = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::V) {
                    currentLight.color.b = std::max(0.0f, currentLight.color.b - 0.1f);
                    showInfo = true;
                }
                
                // Позиция (если есть)
                if (currentLight.type != LIGHT_DIRECTIONAL) {
                    if (keyPressed->code == sf::Keyboard::Key::Left) {
                        currentLight.position.x -= 0.5f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::Right) {
                        currentLight.position.x += 0.5f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::Up) {
                        currentLight.position.z -= 0.5f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::Down) {
                        currentLight.position.z += 0.5f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::PageUp) {
                        currentLight.position.y += 0.5f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::PageDown) {
                        currentLight.position.y -= 0.5f;
                        showInfo = true;
                    }
                }
                
                // Направление (если есть)
                if (currentLight.type != LIGHT_POINT) {
                    if (keyPressed->code == sf::Keyboard::Key::I) {
                        currentLight.direction.x += 0.1f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::K) {
                        currentLight.direction.x -= 0.1f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::J) {
                        currentLight.direction.y += 0.1f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::L) {
                        currentLight.direction.y -= 0.1f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::U) {
                        currentLight.direction.z += 0.1f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::O) {
                        currentLight.direction.z -= 0.1f;
                        showInfo = true;
                    }
                }
                
                // Угол конуса (только для прожектора)
                if (currentLight.type == LIGHT_SPOT) {
                    if (keyPressed->code == sf::Keyboard::Key::M) {
                        currentLight.cutOff += 1.0f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::Comma) {
                        currentLight.cutOff = std::max(0.0f, currentLight.cutOff - 1.0f);
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::Period) {
                        currentLight.outerCutOff += 1.0f;
                        showInfo = true;
                    }
                    if (keyPressed->code == sf::Keyboard::Key::Slash) {
                        currentLight.outerCutOff = std::max(0.0f, currentLight.outerCutOff - 1.0f);
                        showInfo = true;
                    }
                }
            }
        }

        
        // Управление камерой WASD
        float cameraSpeed = 5.0f * deltaTime;
        glm::vec3 cameraFront = glm::normalize(cameraTarget - cameraPos);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

        // Проверка нажатых клавиш
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            cameraPos += cameraFront * cameraSpeed;
            cameraTarget += cameraFront * cameraSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            cameraPos -= cameraFront * cameraSpeed;
            cameraTarget -= cameraFront * cameraSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            cameraPos -= cameraRight * cameraSpeed;
            cameraTarget -= cameraRight * cameraSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            cameraPos += cameraRight * cameraSpeed;
            cameraTarget += cameraRight * cameraSpeed;
        }
        
        // Отображение информации
        if (showInfo) {
            displayLightInfo(window, lights[currentLightIndex]);
            showInfo = false;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        
        setupLightsInShader();
        
        glm::mat4 projection = glm::perspective(
            glm::radians(60.0f),
            1280.0f / 720.0f,
            0.1f,
            100.0f
        );
        
        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraTarget,
            cameraUp
        );
        
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        
        // Рендеринг объектов с разными моделями освещения
        for (size_t i = 0; i < sceneObjects.size(); i++) {
            SceneObject& obj = sceneObjects[i];
            
            // Устанавливаем модель освещения для текущего объекта
            GLuint lightingModelLoc = glGetUniformLocation(shaderProgram, "lightingModel");
            glUniform1i(lightingModelLoc, obj.lightingModel);
            
            // Устанавливаем параметры моделей освещения
            GLuint toonBandsLoc = glGetUniformLocation(shaderProgram, "toonBands");
            glUniform1i(toonBandsLoc, toonBands);
            
            GLuint roughnessLoc = glGetUniformLocation(shaderProgram, "roughness");
            glUniform1f(roughnessLoc, roughness);
            
            GLuint specularPowerLoc = glGetUniformLocation(shaderProgram, "specularPower");
            glUniform1f(specularPowerLoc, specularPower);
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            
            if (obj.rotation.y != 0.0f) {
                model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            }
            if (obj.rotation.x != 0.0f) {
                model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            }
            if (obj.rotation.z != 0.0f) {
                model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            }
            
            model = glm::scale(model, obj.scale);
            
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            if (obj.textureID != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, obj.textureID);
                glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
            }
            
            if (obj.mesh.VAO != 0) {
                glBindVertexArray(obj.mesh.VAO);
                glDrawArrays(GL_TRIANGLES, 0, obj.mesh.vertices.size());
                glBindVertexArray(0);
            }
        }
        
        window.display();
    }
    
    for (auto tex : textures) {
        glDeleteTextures(1, &tex);
    }
    
    for (auto& obj : sceneObjects) {
        obj.mesh.cleanup();
    }
    
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
    }
    
    std::cout << "\nПрограмма завершена.\n";
    return 0;
}