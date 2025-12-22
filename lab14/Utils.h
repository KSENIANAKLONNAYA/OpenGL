#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Mesh {
    std::vector<Vertex> vertices;
    GLuint VAO = 0, VBO = 0;
    
    void uploadToGPU() {
        if (vertices.empty()) return;
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        
        // Позиция
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        
        // Нормаль
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        
        // Текстурные координаты
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        
        glBindVertexArray(0);
    }
    
    void draw() {
        if (VAO == 0) return;
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
        glBindVertexArray(0);
    }
    
    void cleanup() {
        if (VBO) glDeleteBuffers(1, &VBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);
        VBO = 0;
        VAO = 0;
    }
};

bool loadOBJ(const std::string& path, Mesh& mesh) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << path << std::endl;
        return false;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    
    mesh.vertices.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (type == "vt") {
            glm::vec2 texcoord;
            iss >> texcoord.x >> texcoord.y;
            texcoord.y = 1.0f - texcoord.y;
            texcoords.push_back(texcoord);
        }
        else if (type == "f") {
            std::string faceData[4];
            int vertexCount = 0;
            
            while (iss >> faceData[vertexCount] && vertexCount < 4) {
                vertexCount++;
            }
            
            // Обработка треугольников и четырехугольников
            for (int i = 1; i < vertexCount - 1; i++) {
                for (int j = 0; j < 3; j++) {
                    std::string vertexStr;
                    if (j == 0) vertexStr = faceData[0];
                    else if (j == 1) vertexStr = faceData[i];
                    else vertexStr = faceData[i + 1];
                    
                    Vertex vertex;
                    std::istringstream viss(vertexStr);
                    std::string index;
                    
                    // Позиция
                    if (std::getline(viss, index, '/') && !index.empty()) {
                        int idx = std::stoi(index) - 1;
                        if (idx >= 0 && idx < positions.size()) {
                            vertex.position = positions[idx];
                        }
                    }
                    
                    // Текстурные координаты
                    if (std::getline(viss, index, '/') && !index.empty()) {
                        int idx = std::stoi(index) - 1;
                        if (idx >= 0 && idx < texcoords.size()) {
                            vertex.texCoord = texcoords[idx];
                        }
                    }
                    
                    // Нормаль
                    if (std::getline(viss, index, '/') && !index.empty()) {
                        int idx = std::stoi(index) - 1;
                        if (idx >= 0 && idx < normals.size()) {
                            vertex.normal = normals[idx];
                        }
                    }
                    
                    mesh.vertices.push_back(vertex);
                }
            }
        }
    }
    
    file.close();
    
    if (mesh.vertices.empty()) {
        std::cerr << "Предупреждение: файл " << path << " не содержит данных" << std::endl;
        return false;
    }
    
    std::cout << "Загружено вершин: " << mesh.vertices.size() << " из " << path << std::endl;
    return true;
}