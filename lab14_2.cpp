// main.cpp
#include "Utils.h"
#include <GL/gl.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp> // Для GUI
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <filesystem> // Для проверки файлов
#include <sstream>

// ---------- Камера ----------
class Camera {
public:
    glm::vec3 position{ 0.0f, 2.0f, 5.0f };
    float yaw = -90.0f;
    float pitch = -10.0f;
    float speed = 3.0f;
    float sensitivity = 0.1f;
    bool mouseCaptured = false;

    glm::mat4 getViewMatrix() const {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::lookAt(position, position + glm::normalize(front), glm::vec3(0, 1, 0));
    }

    void toggleMouseCapture(sf::Window& window) {
        mouseCaptured = !mouseCaptured;
        window.setMouseCursorVisible(!mouseCaptured);
        if (mouseCaptured) {
            sf::Mouse::setPosition(sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2), window);
        }
    }

    void processInput(sf::Window& window, float deltaTime) {
        // Обработка клавиш всегда активна
        float velocity = speed * deltaTime;
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            position += glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw))) * velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            position -= glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw))) * velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            position -= glm::normalize(glm::cross(glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw))), glm::vec3(0, 1, 0))) * velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            position += glm::normalize(glm::cross(glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw))), glm::vec3(0, 1, 0))) * velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            position.y += velocity;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
            position.y -= velocity;

        // Обработка вращения камеры только если захвачен курсор
        if (mouseCaptured) {
            sf::Vector2i currentMousePos = sf::Mouse::getPosition(window);
            sf::Vector2i center(window.getSize().x / 2, window.getSize().y / 2);
            
            if (firstMouse) {
                lastMousePos = currentMousePos;
                firstMouse = false;
            }
            
            float xoffset = (currentMousePos.x - center.x) * sensitivity;
            float yoffset = (center.y - currentMousePos.y) * sensitivity;
            
            yaw += xoffset;
            pitch += yoffset;
            
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
            
            sf::Mouse::setPosition(center, window);
        }
    }
    
private:
    bool firstMouse = true;
    sf::Vector2i lastMousePos;
};

// ---------- Структуры для источников света ----------
struct BaseLight {
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
};

struct PointLight : BaseLight {
    glm::vec3 position = glm::vec3(0.0f, 2.0f, 0.0f);
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    
    // Для GUI
    void drawGUI(const std::string& name) {
        ImGui::Text("%s", name.c_str());
        ImGui::SliderFloat3("Position", &position.x, -5.0f, 5.0f);
        ImGui::ColorEdit3("Color", &color.x);
        ImGui::SliderFloat("Intensity", &intensity, 0.0f, 5.0f);
        ImGui::Separator();
    }
};

struct DirectionalLight : BaseLight {
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    
    // Для GUI
    void drawGUI(const std::string& name) {
        ImGui::Text("%s", name.c_str());
        ImGui::SliderFloat3("Direction", &direction.x, -1.0f, 1.0f);
        ImGui::ColorEdit3("Color", &color.x);
        ImGui::SliderFloat("Intensity", &intensity, 0.0f, 5.0f);
        ImGui::Separator();
    }
};

struct SpotLight : BaseLight {
    glm::vec3 position = glm::vec3(0.0f, 3.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(17.5f));
    
    // Для GUI
    void drawGUI(const std::string& name) {
        ImGui::Text("%s", name.c_str());
        ImGui::SliderFloat3("Position", &position.x, -5.0f, 5.0f);
        ImGui::SliderFloat3("Direction", &direction.x, -1.0f, 1.0f);
        ImGui::ColorEdit3("Color", &color.x);
        ImGui::SliderFloat("Intensity", &intensity, 0.0f, 5.0f);
        
        // Преобразуем обратно в градусы для GUI
        float cutOffDeg = glm::degrees(glm::acos(cutOff));
        float outerCutOffDeg = glm::degrees(glm::acos(outerCutOff));
        
        if (ImGui::SliderFloat("Inner Cutoff", &cutOffDeg, 0.0f, 45.0f)) {
            cutOff = glm::cos(glm::radians(cutOffDeg));
        }
        if (ImGui::SliderFloat("Outer Cutoff", &outerCutOffDeg, 0.0f, 45.0f)) {
            outerCutOff = glm::cos(glm::radians(outerCutOffDeg));
        }
        
        ImGui::Separator();
    }
};

// ---------- Материал объекта ----------
struct Material {
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(0.5f);
    float shininess = 32.0f;
    GLuint textureID = 0;
    bool hasTexture = false;
    
    // Для GUI
    void drawGUI(const std::string& name) {
        ImGui::Text("Material: %s", name.c_str());
        ImGui::ColorEdit3("Ambient", &ambient.x);
        ImGui::ColorEdit3("Diffuse", &diffuse.x);
        ImGui::ColorEdit3("Specular", &specular.x);
        ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
        ImGui::Separator();
    }
};

// ---------- Структура объекта сцены ----------
struct SceneObject {
    Mesh mesh;
    GLuint shaderProgram;
    Material material;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
    
    std::string lightingModel; // "phong", "toon", "minnaert", "oren-nayar", "cook-torrance"
    std::string name; // Имя объекта для GUI

    // Uniform locations
    GLint locModel;
    GLint locView;
    GLint locProj;
    GLint locViewPos;
    GLint locTexture;
    
    // Material uniforms
    GLint locMaterialAmbient;
    GLint locMaterialDiffuse;
    GLint locMaterialSpecular;
    GLint locMaterialShininess;
    
    // Light uniforms
    GLint locPointLightPos;
    GLint locPointLightColor;
    GLint locPointLightIntensity;
    GLint locDirLightDir;
    GLint locDirLightColor;
    GLint locSpotLightPos;
    GLint locSpotLightDir;
    GLint locSpotLightColor;
    GLint locSpotLightCutOff;
    GLint locSpotLightOuterCutOff;

    SceneObject() : shaderProgram(0) {}
    
    // Для GUI
    void drawGUI() {
        if (ImGui::TreeNode(name.c_str())) {
            ImGui::SliderFloat3("Position", &position.x, -5.0f, 5.0f);
            ImGui::SliderFloat3("Scale", &scale.x, 0.1f, 3.0f);
            material.drawGUI(name);
            ImGui::TreePop();
        }
    }
};

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
    // Проверка существования файла
    if (!std::filesystem::exists(path)) {
        std::cerr << "Texture file not found: " << path << std::endl;
        return 0;
    }
    
    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    image.flipVertically();

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

    std::cout << "Texture loaded: " << path << std::endl;
    return textureID;
}

// ---------- Функция проверки существования .obj файла ----------
bool checkOBJFileExists(const std::string& filename) {
    if (!std::filesystem::exists(filename)) {
        std::cerr << "ERROR: OBJ file not found: " << filename << std::endl;
        std::cerr << "Please create the following files in Objects/ folder:" << std::endl;
        std::cerr << "  sphere.obj, cube.obj, torus.obj, cylinder.obj, cone.obj" << std::endl;
        std::cerr << "Or you can download sample models and place them in Objects/ folder." << std::endl;
        return false;
    }
    return true;
}

// ---------- Модифицированная функция загрузки OBJ ----------
bool loadOBJWithCheck(const std::string& filename, Mesh& mesh) {
    if (!checkOBJFileExists(filename)) {
        return false;
    }
    
    std::cout << "Loading OBJ: " << filename << std::endl;
    return loadOBJ(filename.c_str(), mesh);
}

// ---------- Вершинный шейдер (без изменений) ----------
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
out vec2 TexCoord;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = proj * view * vec4(FragPos, 1.0);
}
)";

// Фрагментные шейдеры (без изменений, оставлены как были)
// [Все фрагментные шейдеры остаются теми же: fragmentShaderPhong, fragmentShaderToon, 
// fragmentShaderMinnaert, fragmentShaderOrenNayar, fragmentShaderCookTorrance]

// ---------- Создание сцены ----------
std::vector<SceneObject> createSceneObjects() {
    std::vector<SceneObject> objects;
    
    // Объект 1: Шар с моделью Phong
    {
        SceneObject obj;
        obj.name = "Sphere (Phong)";
        if (loadOBJWithCheck("Objects/sphere.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.shaderProgram = createShaderProgram(vertexShaderSrc, fragmentShaderPhong);
            obj.lightingModel = "phong";
            
            obj.material.ambient = glm::vec3(0.1f, 0.1f, 0.15f);
            obj.material.diffuse = glm::vec3(0.6f, 0.6f, 0.8f);
            obj.material.specular = glm::vec3(1.0f);
            obj.material.shininess = 64.0f;
            
            // Попытка загрузки текстуры
            obj.material.textureID = loadTexture("Textures/metal.jpg");
            obj.material.hasTexture = (obj.material.textureID != 0);
            if (!obj.material.hasTexture) {
                std::cout << "Using solid color for sphere" << std::endl;
            }
            
            obj.position = glm::vec3(-2.0f, 1.0f, 0.0f);
            obj.scale = glm::vec3(0.5f);
            
            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");
            obj.locTexture = glGetUniformLocation(obj.shaderProgram, "texSampler");
            
            obj.locMaterialAmbient = glGetUniformLocation(obj.shaderProgram, "materialAmbient");
            obj.locMaterialDiffuse = glGetUniformLocation(obj.shaderProgram, "materialDiffuse");
            obj.locMaterialSpecular = glGetUniformLocation(obj.shaderProgram, "materialSpecular");
            obj.locMaterialShininess = glGetUniformLocation(obj.shaderProgram, "materialShininess");
            
            obj.locPointLightPos = glGetUniformLocation(obj.shaderProgram, "pointLightPos");
            obj.locPointLightColor = glGetUniformLocation(obj.shaderProgram, "pointLightColor");
            obj.locPointLightIntensity = glGetUniformLocation(obj.shaderProgram, "pointLightIntensity");
            obj.locDirLightDir = glGetUniformLocation(obj.shaderProgram, "dirLightDir");
            obj.locDirLightColor = glGetUniformLocation(obj.shaderProgram, "dirLightColor");
            obj.locSpotLightPos = glGetUniformLocation(obj.shaderProgram, "spotLightPos");
            obj.locSpotLightDir = glGetUniformLocation(obj.shaderProgram, "spotLightDir");
            obj.locSpotLightColor = glGetUniformLocation(obj.shaderProgram, "spotLightColor");
            obj.locSpotLightCutOff = glGetUniformLocation(obj.shaderProgram, "spotLightCutOff");
            obj.locSpotLightOuterCutOff = glGetUniformLocation(obj.shaderProgram, "spotLightOuterCutOff");
            
            objects.push_back(obj);
        } else {
            std::cerr << "Failed to load sphere.obj" << std::endl;
        }
    }
    
    // Объект 2: Кубик с Toon Shading
    {
        SceneObject obj;
        obj.name = "Cube (Toon)";
        if (loadOBJWithCheck("Objects/cube.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.shaderProgram = createShaderProgram(vertexShaderSrc, fragmentShaderToon);
            obj.lightingModel = "toon";
            
            obj.material.diffuse = glm::vec3(0.8f, 0.2f, 0.2f);
            obj.material.textureID = loadTexture("Textures/wood.jpg");
            obj.material.hasTexture = (obj.material.textureID != 0);
            
            obj.position = glm::vec3(0.0f, 0.5f, 0.0f);
            obj.scale = glm::vec3(0.5f);
            
            // Получаем uniform locations (только необходимые для toon)
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");
            obj.locTexture = glGetUniformLocation(obj.shaderProgram, "texSampler");
            
            obj.locMaterialDiffuse = glGetUniformLocation(obj.shaderProgram, "materialDiffuse");
            
            obj.locPointLightPos = glGetUniformLocation(obj.shaderProgram, "pointLightPos");
            obj.locPointLightColor = glGetUniformLocation(obj.shaderProgram, "pointLightColor");
            obj.locDirLightDir = glGetUniformLocation(obj.shaderProgram, "dirLightDir");
            obj.locSpotLightPos = glGetUniformLocation(obj.shaderProgram, "spotLightPos");
            obj.locSpotLightDir = glGetUniformLocation(obj.shaderProgram, "spotLightDir");
            obj.locSpotLightCutOff = glGetUniformLocation(obj.shaderProgram, "spotLightCutOff");
            obj.locSpotLightOuterCutOff = glGetUniformLocation(obj.shaderProgram, "spotLightOuterCutOff");
            
            objects.push_back(obj);
        } else {
            std::cerr << "Failed to load cube.obj" << std::endl;
        }
    }
    
    // Объект 3: Тор с моделью Minnaert
    {
        SceneObject obj;
        obj.name = "Torus (Minnaert)";
        if (loadOBJWithCheck("Objects/torus.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.shaderProgram = createShaderProgram(vertexShaderSrc, fragmentShaderMinnaert);
            obj.lightingModel = "minnaert";
            
            obj.material.diffuse = glm::vec3(0.2f, 0.8f, 0.3f);
            obj.material.textureID = loadTexture("Textures/fabric.jpg");
            obj.material.hasTexture = (obj.material.textureID != 0);
            
            obj.position = glm::vec3(2.0f, 1.0f, 0.0f);
            obj.scale = glm::vec3(0.4f);
            
            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");
            obj.locTexture = glGetUniformLocation(obj.shaderProgram, "texSampler");
            
            obj.locMaterialDiffuse = glGetUniformLocation(obj.shaderProgram, "materialDiffuse");
            obj.locPointLightPos = glGetUniformLocation(obj.shaderProgram, "pointLightPos");
            obj.locPointLightColor = glGetUniformLocation(obj.shaderProgram, "pointLightColor");
            obj.locDirLightDir = glGetUniformLocation(obj.shaderProgram, "dirLightDir");
            obj.locDirLightColor = glGetUniformLocation(obj.shaderProgram, "dirLightColor");
            obj.locSpotLightPos = glGetUniformLocation(obj.shaderProgram, "spotLightPos");
            obj.locSpotLightDir = glGetUniformLocation(obj.shaderProgram, "spotLightDir");
            
            objects.push_back(obj);
        } else {
            std::cerr << "Failed to load torus.obj" << std::endl;
        }
    }
    
    // Объект 4: Цилиндр с моделью Oren-Nayar
    {
        SceneObject obj;
        obj.name = "Cylinder (Oren-Nayar)";
        if (loadOBJWithCheck("Objects/cylinder.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.shaderProgram = createShaderProgram(vertexShaderSrc, fragmentShaderOrenNayar);
            obj.lightingModel = "oren-nayar";
            
            obj.material.diffuse = glm::vec3(0.8f, 0.8f, 0.2f);
            obj.material.textureID = loadTexture("Textures/concrete.jpg");
            obj.material.hasTexture = (obj.material.textureID != 0);
            
            obj.position = glm::vec3(-1.0f, 0.0f, 2.0f);
            obj.scale = glm::vec3(0.3f, 0.6f, 0.3f);
            
            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");
            obj.locTexture = glGetUniformLocation(obj.shaderProgram, "texSampler");
            
            obj.locMaterialDiffuse = glGetUniformLocation(obj.shaderProgram, "materialDiffuse");
            obj.locDirLightDir = glGetUniformLocation(obj.shaderProgram, "dirLightDir");
            obj.locDirLightColor = glGetUniformLocation(obj.shaderProgram, "dirLightColor");
            
            objects.push_back(obj);
        } else {
            std::cerr << "Failed to load cylinder.obj" << std::endl;
        }
    }
    
    // Объект 5: Конус с моделью Cook-Torrance
    {
        SceneObject obj;
        obj.name = "Cone (Cook-Torrance)";
        if (loadOBJWithCheck("Objects/cone.obj", obj.mesh)) {
            obj.mesh.uploadToGPU();
            obj.shaderProgram = createShaderProgram(vertexShaderSrc, fragmentShaderCookTorrance);
            obj.lightingModel = "cook-torrance";
            
            obj.material.diffuse = glm::vec3(0.9f, 0.6f, 0.1f);
            obj.material.specular = glm::vec3(1.0f);
            obj.material.shininess = 128.0f;
            obj.material.textureID = loadTexture("Textures/gold.jpg");
            obj.material.hasTexture = (obj.material.textureID != 0);
            
            obj.position = glm::vec3(1.0f, 0.0f, 2.0f);
            obj.scale = glm::vec3(0.4f, 0.6f, 0.4f);
            
            // Получаем uniform locations
            obj.locModel = glGetUniformLocation(obj.shaderProgram, "model");
            obj.locView = glGetUniformLocation(obj.shaderProgram, "view");
            obj.locProj = glGetUniformLocation(obj.shaderProgram, "proj");
            obj.locViewPos = glGetUniformLocation(obj.shaderProgram, "viewPos");
            obj.locTexture = glGetUniformLocation(obj.shaderProgram, "texSampler");
            
            obj.locMaterialDiffuse = glGetUniformLocation(obj.shaderProgram, "materialDiffuse");
            obj.locMaterialSpecular = glGetUniformLocation(obj.shaderProgram, "materialSpecular");
            obj.locMaterialShininess = glGetUniformLocation(obj.shaderProgram, "materialShininess");
            obj.locDirLightDir = glGetUniformLocation(obj.shaderProgram, "dirLightDir");
            obj.locDirLightColor = glGetUniformLocation(obj.shaderProgram, "dirLightColor");
            
            objects.push_back(obj);
        } else {
            std::cerr << "Failed to load cone.obj" << std::endl;
        }
    }
    
    if (objects.empty()) {
        std::cerr << "\n=== NO OBJECTS LOADED ===" << std::endl;
        std::cerr << "Please create the following .obj files in Objects/ folder:" << std::endl;
        std::cerr << "1. sphere.obj" << std::endl;
        std::cerr << "2. cube.obj" << std::endl;
        std::cerr << "3. torus.obj" << std::endl;
        std::cerr << "4. cylinder.obj" << std::endl;
        std::cerr << "5. cone.obj" << std::endl;
        std::cerr << "\nYou can create simple .obj files or download samples." << std::endl;
    }
    
    return objects;
}

// ---------- GUI класс ----------
class LightingGUI {
private:
    bool showControls = true;
    bool showLights = true;
    bool showObjects = true;
    bool showInfo = true;
    
public:
    void draw(PointLight& pointLight, DirectionalLight& dirLight, SpotLight& spotLight, 
              std::vector<SceneObject>& objects, Camera& cam, float fps) {
        
        if (!showControls) return;
        
        // Начинаем новый ImGui frame
        ImGui::SFML::Update(static_cast<sf::RenderWindow&>(*sf::Window::getActiveWindow()), 
                           sf::seconds(1.0f/60.0f));
        
        // Создаем окно с GUI
        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Lighting Controls", &showControls);
        
        // FPS и информация
        if (ImGui::CollapsingHeader("Info", &showInfo)) {
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", 
                       cam.position.x, cam.position.y, cam.position.z);
            ImGui::Text("Camera Yaw/Pitch: %.1f, %.1f", cam.yaw, cam.pitch);
            ImGui::Separator();
            ImGui::Text("Controls:");
            ImGui::Text("WASD - Move camera");
            ImGui::Text("Space/LShift - Up/Down");
            ImGui::Text("Tab - Toggle GUI");
            ImGui::Text("ESC - Exit");
        }
        
        // Управление источниками света
        if (ImGui::CollapsingHeader("Light Sources", &showLights)) {
            pointLight.drawGUI("Point Light");
            dirLight.drawGUI("Directional Light");
            spotLight.drawGUI("Spot Light");
            
            // Кнопки для сброса настроек
            if (ImGui::Button("Reset All Lights")) {
                pointLight = PointLight();
                dirLight = DirectionalLight();
                spotLight = SpotLight();
            }
        }
        
        // Управление объектами
        if (ImGui::CollapsingHeader("Scene Objects", &showObjects)) {
            for (auto& obj : objects) {
                obj.drawGUI();
            }
        }
        
        // Настройки рендеринга
        if (ImGui::CollapsingHeader("Rendering")) {
            static bool animate = true;
            ImGui::Checkbox("Animate Lights", &animate);
            
            static bool rotateObjects = true;
            ImGui::Checkbox("Rotate Objects", &rotateObjects);
            
            static int wireframeMode = 0;
            ImGui::RadioButton("Solid", &wireframeMode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Wireframe", &wireframeMode, 1);
            
            if (wireframeMode == 1) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }
        
        ImGui::End();
    }
    
    bool isVisible() const { return showControls; }
    void toggle() { showControls = !showControls; }
};

// ---------- Установка uniform переменных для освещения ----------
void setupLightUniforms(GLuint program, 
                       const PointLight& pointLight,
                       const DirectionalLight& dirLight,
                       const SpotLight& spotLight,
                       const std::unordered_map<std::string, GLint>& locations) {
    
    // Точечный источник
    if (locations.count("pointLightPos"))
        glUniform3fv(locations.at("pointLightPos"), 1, &pointLight.position[0]);
    if (locations.count("pointLightColor"))
        glUniform3fv(locations.at("pointLightColor"), 1, &pointLight.color[0]);
    if (locations.count("pointLightIntensity"))
        glUniform1f(locations.at("pointLightIntensity"), pointLight.intensity);
    
    // Направленный источник
    if (locations.count("dirLightDir"))
        glUniform3fv(locations.at("dirLightDir"), 1, &dirLight.direction[0]);
    if (locations.count("dirLightColor"))
        glUniform3fv(locations.at("dirLightColor"), 1, &dirLight.color[0]);
    
    // Прожектор
    if (locations.count("spotLightPos"))
        glUniform3fv(locations.at("spotLightPos"), 1, &spotLight.position[0]);
    if (locations.count("spotLightDir"))
        glUniform3fv(locations.at("spotLightDir"), 1, &spotLight.direction[0]);
    if (locations.count("spotLightColor"))
        glUniform3fv(locations.at("spotLightColor"), 1, &spotLight.color[0]);
    if (locations.count("spotLightCutOff"))
        glUniform1f(locations.at("spotLightCutOff"), spotLight.cutOff);
    if (locations.count("spotLightOuterCutOff"))
        glUniform1f(locations.at("spotLightOuterCutOff"), spotLight.outerCutOff);
}

// ---------- main ----------
int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;
    
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Multiple Lighting Models Demo with GUI",
                           sf::Style::Default, settings);
    window.setMouseCursorVisible(false);
    window.setFramerateLimit(60);
    window.setActive(true);
    
    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }
    
    // Инициализация ImGui
    ImGui::SFML::Init(window);
    
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // Проверка папок
    std::cout << "\n=== Checking required folders ===" << std::endl;
    if (!std::filesystem::exists("Objects")) {
        std::cout << "Creating Objects folder..." << std::endl;
        std::filesystem::create_directory("Objects");
    }
    if (!std::filesystem::exists("Textures")) {
        std::cout << "Creating Textures folder..." << std::endl;
        std::filesystem::create_directory("Textures");
    }
    std::cout << "==============================\n" << std::endl;
    
    // Создаем сцену
    auto sceneObjects = createSceneObjects();
    if (sceneObjects.empty()) {
        std::cerr << "\n=== CRITICAL ERROR ===" << std::endl;
        std::cerr << "No objects were loaded. Please create .obj files as described above." << std::endl;
        std::cerr << "The program will continue but no objects will be rendered." << std::endl;
    }
    
    // Настраиваем источники света
    PointLight pointLight;
    pointLight.position = glm::vec3(0.0f, 3.0f, 0.0f);
    pointLight.color = glm::vec3(1.0f, 0.9f, 0.8f); // Теплый свет
    pointLight.intensity = 1.0f;
    
    DirectionalLight dirLight;
    dirLight.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight.color = glm::vec3(0.8f, 0.8f, 1.0f); // Холодный свет
    dirLight.intensity = 0.5f;
    
    SpotLight spotLight;
    spotLight.position = glm::vec3(2.0f, 3.0f, 2.0f);
    spotLight.direction = glm::vec3(-0.5f, -1.0f, -0.5f);
    spotLight.color = glm::vec3(0.9f, 1.0f, 0.9f); // Зеленоватый свет
    spotLight.cutOff = glm::cos(glm::radians(15.0f));
    spotLight.outerCutOff = glm::cos(glm::radians(25.0f));
    
    Camera cam;
    sf::Clock deltaClock;
    sf::Clock fpsClock;
    sf::Mouse::setPosition(sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2), window);
    
    // GUI
    LightingGUI gui;
    
    float time = 0.0f;
    int frameCount = 0;
    float fps = 0.0f;
    
    // Основной цикл
    while (window.isOpen()) {
        float deltaTime = deltaClock.restart().asSeconds();
        time += deltaTime;
        frameCount++;
        
        // Расчет FPS каждую секунду
        if (fpsClock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frameCount / fpsClock.restart().asSeconds();
            frameCount = 0;
        }
        
        // Обработка событий
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();
                if (event.key.code == sf::Keyboard::Tab)
                    gui.toggle();
                if (event.key.code == sf::Keyboard::F1)
                    cam.toggleMouseCapture(window);
            }
        }
        
        // Обновление камеры
        cam.processInput(window, deltaTime);
        
        // Анимация источников света (если включена в GUI)
        pointLight.position.y = 2.5f + sin(time) * 0.5f;
        spotLight.position.x = 2.0f + cos(time * 0.5f) * 1.5f;
        spotLight.position.z = 2.0f + sin(time * 0.5f) * 1.5f;
        
        // Рендеринг
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 viewMat = cam.getViewMatrix();
        glm::mat4 projMat = glm::perspective(glm::radians(45.0f),
                                            (float)window.getSize().x / (float)window.getSize().y,
                                            0.1f, 100.0f);
        
        // Рендеринг объектов
        for (auto& obj : sceneObjects) {
            if (obj.shaderProgram == 0) continue;
            
            glUseProgram(obj.shaderProgram);
            
            // Матрица модели
            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::translate(modelMat, obj.position);
            modelMat = glm::rotate(modelMat, time * glm::radians(20.0f), glm::vec3(0, 1, 0));
            modelMat = glm::scale(modelMat, obj.scale);
            
            // Базовые uniforms
            glUniformMatrix4fv(obj.locModel, 1, GL_FALSE, &modelMat[0][0]);
            glUniformMatrix4fv(obj.locView, 1, GL_FALSE, &viewMat[0][0]);
            glUniformMatrix4fv(obj.locProj, 1, GL_FALSE, &projMat[0][0]);
            
            if (obj.locViewPos != -1)
                glUniform3fv(obj.locViewPos, 1, &cam.position[0]);
            
            // Материал
            if (obj.locMaterialAmbient != -1)
                glUniform3fv(obj.locMaterialAmbient, 1, &obj.material.ambient[0]);
            if (obj.locMaterialDiffuse != -1)
                glUniform3fv(obj.locMaterialDiffuse, 1, &obj.material.diffuse[0]);
            if (obj.locMaterialSpecular != -1)
                glUniform3fv(obj.locMaterialSpecular, 1, &obj.material.specular[0]);
            if (obj.locMaterialShininess != -1)
                glUniform1f(obj.locMaterialShininess, obj.material.shininess);
            
            // Текстура
            if (obj.locTexture != -1 && obj.material.hasTexture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, obj.material.textureID);
                glUniform1i(obj.locTexture, 0);
                glUniform1i(glGetUniformLocation(obj.shaderProgram, "hasTexture"), 1);
            } else {
                glUniform1i(glGetUniformLocation(obj.shaderProgram, "hasTexture"), 0);
            }
            
            // Источники света
            std::unordered_map<std::string, GLint> lightLocs;
            if (obj.locPointLightPos != -1) lightLocs["pointLightPos"] = obj.locPointLightPos;
            if (obj.locPointLightColor != -1) lightLocs["pointLightColor"] = obj.locPointLightColor;
            if (obj.locPointLightIntensity != -1) lightLocs["pointLightIntensity"] = obj.locPointLightIntensity;
            if (obj.locDirLightDir != -1) lightLocs["dirLightDir"] = obj.locDirLightDir;
            if (obj.locDirLightColor != -1) lightLocs["dirLightColor"] = obj.locDirLightColor;
            if (obj.locSpotLightPos != -1) lightLocs["spotLightPos"] = obj.locSpotLightPos;
            if (obj.locSpotLightDir != -1) lightLocs["spotLightDir"] = obj.locSpotLightDir;
            if (obj.locSpotLightColor != -1) lightLocs["spotLightColor"] = obj.locSpotLightColor;
            if (obj.locSpotLightCutOff != -1) lightLocs["spotLightCutOff"] = obj.locSpotLightCutOff;
            if (obj.locSpotLightOuterCutOff != -1) lightLocs["spotLightOuterCutOff"] = obj.locSpotLightOuterCutOff;
            
            setupLightUniforms(obj.shaderProgram, pointLight, dirLight, spotLight, lightLocs);
            
            obj.mesh.draw();
        }
        
        // Рендеринг GUI
        gui.draw(pointLight, dirLight, spotLight, sceneObjects, cam, fps);
        ImGui::SFML::Render(window);
        
        window.display();
    }
    
    // Очистка
    ImGui::SFML::Shutdown();
    
    for (auto& obj : sceneObjects) {
        if (obj.material.hasTexture)
            glDeleteTextures(1, &obj.material.textureID);
        if (obj.shaderProgram)
            glDeleteProgram(obj.shaderProgram);
        obj.mesh.cleanup();
    }
    
    return 0;
}