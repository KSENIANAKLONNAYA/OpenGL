#pragma once
#include <gl/glew.h>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texCoord{};
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint vao = 0, vbo = 0, ebo = 0;


    void uploadToGPU() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
            vertices.size() * sizeof(Vertex),
            vertices.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW);

        // позиция
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (void*)0);
        // нормаль
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (void*)offsetof(Vertex, normal));
        // tex‑coord
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES,
            static_cast<GLsizei>(indices.size()),
            GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
};

/* ------------------------------------------------------------------ */
// Хеш‑функция для кортежа (posIdx, texIdx, normIdx)
struct IndexTripleHash {
    std::size_t operator()(const std::tuple<int, int, int>& t) const noexcept {
        std::size_t h1 = std::hash<int>{}(std::get<0>(t));
        std::size_t h2 = std::hash<int>{}(std::get<1>(t));
        std::size_t h3 = std::hash<int>{}(std::get<2>(t));
        // простое комбинирование
        return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
    }
};

/* ------------------------------------------------------------------ */
// Улучшенный загрузчик
bool loadOBJ(const std::string& path, Mesh& outMesh) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;

    std::unordered_map<std::tuple<int, int, int>, unsigned int, IndexTripleHash> vertexCache;

    std::ifstream file(path);
    if (!file) {
        std::cerr << "Cannot open OBJ file: " << path << '\n';
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // игнорируем пустые строки и комментарии
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string prefix; ss >> prefix;

        if (prefix == "v") {                     // позиция
            glm::vec3 p; ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (prefix == "vt") {               // tex‑coord
            glm::vec2 t; ss >> t.x >> t.y;
            texCoords.push_back(t);
        }
        else if (prefix == "vn") {               // нормаль
            glm::vec3 n; ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (prefix == "f") {                // грань
            // читаем все вершины грани в вектор
            std::vector<std::tuple<int, int, int>> faceIndices;
            std::string vertStr;
            while (ss >> vertStr) {
                // заменяем '/' на пробел, чтобы легко разобрать
                std::replace(vertStr.begin(), vertStr.end(), '/', ' ');
                std::istringstream vs(vertStr);
                int vi = 0, ti = 0, ni = 0;
                vs >> vi;                     // позиция всегда есть
                if (vs.peek() != EOF) vs >> ti; // tex‑coord может отсутствовать
                if (vs.peek() != EOF) vs >> ni; // нормаль может отсутствовать

                // OBJ‑индексация начинается с 1, переводим в 0‑базу
                vi = (vi > 0) ? vi - 1 : vi + static_cast<int>(positions.size());
                ti = (ti > 0) ? ti - 1 : ti + static_cast<int>(texCoords.size());
                ni = (ni > 0) ? ni - 1 : ni + static_cast<int>(normals.size());

                faceIndices.emplace_back(vi, ti, ni);
            }

            // Triangulation fan‑style: (0, i, i+1)
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                std::array<std::tuple<int, int, int>, 3> tri = {
                    faceIndices[0],
                    faceIndices[i],
                    faceIndices[i + 1]
                };

                for (const auto& idxTuple : tri) {
                    // Если уже есть такая комбинация — используем её индекс
                    auto it = vertexCache.find(idxTuple);
                    if (it != vertexCache.end()) {
                        outMesh.indices.push_back(it->second);
                        continue;
                    }

                    // Иначе создаём новый Vertex
                    Vertex v{};
                    int pi, ti, ni;
                    std::tie(pi, ti, ni) = idxTuple;

                    v.position = positions[pi];
                    v.normal = (ni >= 0 && ni < static_cast<int>(normals.size())) ? normals[ni] : glm::vec3(0.0f);
                    v.texCoord = (ti >= 0 && ti < static_cast<int>(texCoords.size())) ? texCoords[ti] : glm::vec2(0.0f);

                    unsigned int newIndex = static_cast<unsigned int>(outMesh.vertices.size());
                    outMesh.vertices.push_back(v);
                    outMesh.indices.push_back(newIndex);
                    vertexCache[idxTuple] = newIndex;
                }
            }
        }
        // остальные префиксы (mtllib, usemtl, o, s и т.п.) игнорируем
    }

    // Если нормали не заданы в файле – посчитаем их усреднением по граням
    if (normals.empty()) {
        std::vector<glm::vec3> accum(outMesh.vertices.size(), glm::vec3(0.0f));
        for (size_t i = 0; i < outMesh.indices.size(); i += 3) {
            unsigned int i0 = outMesh.indices[i];
            unsigned int i1 = outMesh.indices[i + 1];
            unsigned int i2 = outMesh.indices[i + 2];

            const glm::vec3& p0 = outMesh.vertices[i0].position;
            const glm::vec3& p1 = outMesh.vertices[i1].position;
            const glm::vec3& p2 = outMesh.vertices[i2].position;

            glm::vec3 faceNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            accum[i0] += faceNormal;
            accum[i1] += faceNormal;
            accum[i2] += faceNormal;
        }
        for (size_t i = 0; i < outMesh.vertices.size(); ++i)
            outMesh.vertices[i].normal = glm::normalize(accum[i]);
    }

    return true;
}
