using System;
using System.Collections.Generic;
using System.IO;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Windowing.Common;
using OpenTK.Windowing.Desktop;
using OpenTK.Windowing.GraphicsLibraryFramework;
using OpenTK.Mathematics;

namespace SolarSystem
{
    class Program
    {
        static void Main(string[] args)
        {
            var nativeWindowSettings = new NativeWindowSettings()
            {
                Size = new Vector2i(1280, 720),
                Title = "Солнечная система с необычными планетами",
                API = ContextAPI.OpenGL,
                APIVersion = new Version(4, 1),
                NumberOfSamples = 8
            };

            using (var game = new SolarSystemGame(GameWindowSettings.Default, nativeWindowSettings))
            {
                game.Run();
            }
        }
    }

    // Класс для загрузки OBJ файлов
    class ObjLoader
    {
        public static Model LoadFromFile(string filePath)
        {
            var model = new Model();

            List<Vector3> vertices = new List<Vector3>();
            List<Vector2> texCoords = new List<Vector2>();
            List<Vector3> normals = new List<Vector3>();
            List<Face> faces = new List<Face>();

            try
            {
                string[] lines = File.ReadAllLines(filePath);

                foreach (string line in lines)
                {
                    string[] parts = line.Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

                    if (parts.Length == 0)
                        continue;

                    switch (parts[0])
                    {
                        case "v": // Вершина
                            if (parts.Length >= 4)
                            {
                                float x = float.Parse(parts[1]);
                                float y = float.Parse(parts[2]);
                                float z = float.Parse(parts[3]);
                                vertices.Add(new Vector3(x, y, z));
                            }
                            break;

                        case "vt": // Текстурные координаты
                            if (parts.Length >= 3)
                            {
                                float u = float.Parse(parts[1]);
                                float v = float.Parse(parts[2]);
                                texCoords.Add(new Vector2(u, v));
                            }
                            break;

                        case "vn": // Нормали
                            if (parts.Length >= 4)
                            {
                                float x = float.Parse(parts[1]);
                                float y = float.Parse(parts[2]);
                                float z = float.Parse(parts[3]);
                                normals.Add(new Vector3(x, y, z));
                            }
                            break;

                        case "f": // Грань
                            if (parts.Length >= 4)
                            {
                                Face face = new Face();

                                for (int i = 1; i < parts.Length; i++)
                                {
                                    string[] indices = parts[i].Split('/');

                                    if (indices.Length >= 1)
                                        face.VertexIndices.Add(int.Parse(indices[0]) - 1);

                                    if (indices.Length >= 2 && !string.IsNullOrEmpty(indices[1]))
                                        face.TexCoordIndices.Add(int.Parse(indices[1]) - 1);

                                    if (indices.Length >= 3 && !string.IsNullOrEmpty(indices[2]))
                                        face.NormalIndices.Add(int.Parse(indices[2]) - 1);
                                }
                                faces.Add(face);
                            }
                            break;
                    }
                }

                // Преобразуем данные в формат модели
                foreach (Face face in faces)
                {
                    // Для треугольников
                    if (face.VertexIndices.Count >= 3)
                    {
                        // Треугольник
                        for (int i = 0; i < 3; i++)
                        {
                            int vi = face.VertexIndices[i];
                            if (vi >= 0 && vi < vertices.Count)
                                model.AddVertex(vertices[vi]);

                            if (face.TexCoordIndices.Count > i)
                            {
                                int ti = face.TexCoordIndices[i];
                                if (ti >= 0 && ti < texCoords.Count)
                                    model.AddTexCoord(texCoords[ti]);
                            }
                            else
                            {
                                model.AddTexCoord(new Vector2(0, 0));
                            }

                            if (face.NormalIndices.Count > i)
                            {
                                int ni = face.NormalIndices[i];
                                if (ni >= 0 && ni < normals.Count)
                                    model.AddNormal(normals[ni]);
                            }
                        }

                        // Если грань - четырехугольник, добавляем второй треугольник
                        if (face.VertexIndices.Count == 4)
                        {
                            int[] quadIndices = { 0, 2, 3 };
                            foreach (int idx in quadIndices)
                            {
                                int vi = face.VertexIndices[idx];
                                if (vi >= 0 && vi < vertices.Count)
                                    model.AddVertex(vertices[vi]);

                                if (face.TexCoordIndices.Count > idx)
                                {
                                    int ti = face.TexCoordIndices[idx];
                                    if (ti >= 0 && ti < texCoords.Count)
                                        model.AddTexCoord(texCoords[ti]);
                                }
                                else
                                {
                                    model.AddTexCoord(new Vector2(0, 0));
                                }

                                if (face.NormalIndices.Count > idx)
                                {
                                    int ni = face.NormalIndices[idx];
                                    if (ni >= 0 && ni < normals.Count)
                                        model.AddNormal(normals[ni]);
                                }
                            }
                        }
                    }
                }

                model.Build();
                Console.WriteLine($"Загружена модель: {filePath}");
                Console.WriteLine($"  Вершин: {vertices.Count}");
                Console.WriteLine($"  Граней: {faces.Count}");
                Console.WriteLine($"  Треугольников: {model.VertexCount / 3}");

                return model;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка загрузки модели {filePath}: {ex.Message}");
                return CreateDefaultModel();
            }
        }

        private static Model CreateDefaultModel()
        {
            var model = new Model();

            float size = 1.0f;

            // Передняя грань
            model.AddVertex(new Vector3(-size, -size, size)); model.AddTexCoord(new Vector2(0, 0));
            model.AddVertex(new Vector3(size, -size, size)); model.AddTexCoord(new Vector2(1, 0));
            model.AddVertex(new Vector3(size, size, size)); model.AddTexCoord(new Vector2(1, 1));

            model.AddVertex(new Vector3(-size, -size, size)); model.AddTexCoord(new Vector2(0, 0));
            model.AddVertex(new Vector3(size, size, size)); model.AddTexCoord(new Vector2(1, 1));
            model.AddVertex(new Vector3(-size, size, size)); model.AddTexCoord(new Vector2(0, 1));

            model.Build();
            return model;
        }

        private class Face
        {
            public List<int> VertexIndices = new List<int>();
            public List<int> TexCoordIndices = new List<int>();
            public List<int> NormalIndices = new List<int>();
        }
    }

    // Класс модели
    class Model
    {
        private List<Vector3> vertices;
        private List<Vector2> texCoords;
        private List<Vector3> normals;
        private int vao, vbo, tbo, nbo;

        public int VertexCount => vertices?.Count ?? 0;

        public Model()
        {
            vertices = new List<Vector3>();
            texCoords = new List<Vector2>();
            normals = new List<Vector3>();
        }

        public void AddVertex(Vector3 vertex)
        {
            vertices.Add(vertex);
        }

        public void AddTexCoord(Vector2 texCoord)
        {
            texCoords.Add(texCoord);
        }

        public void AddNormal(Vector3 normal)
        {
            normals.Add(normal);
        }

        public void Build()
        {
            vao = GL.GenVertexArray();
            GL.BindVertexArray(vao);

            vbo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Count * 3 * sizeof(float),
                         vertices.ToArray(), BufferUsageHint.StaticDraw);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 0, 0);
            GL.EnableVertexAttribArray(0);

            if (texCoords.Count > 0)
            {
                tbo = GL.GenBuffer();
                GL.BindBuffer(BufferTarget.ArrayBuffer, tbo);
                GL.BufferData(BufferTarget.ArrayBuffer, texCoords.Count * 2 * sizeof(float),
                             texCoords.ToArray(), BufferUsageHint.StaticDraw);
                GL.VertexAttribPointer(1, 2, VertexAttribPointerType.Float, false, 0, 0);
                GL.EnableVertexAttribArray(1);
            }

            if (normals.Count > 0)
            {
                nbo = GL.GenBuffer();
                GL.BindBuffer(BufferTarget.ArrayBuffer, nbo);
                GL.BufferData(BufferTarget.ArrayBuffer, normals.Count * 3 * sizeof(float),
                             normals.ToArray(), BufferUsageHint.StaticDraw);
                GL.VertexAttribPointer(2, 3, VertexAttribPointerType.Float, false, 0, 0);
                GL.EnableVertexAttribArray(2);
            }

            GL.BindVertexArray(0);
        }

        public void Render()
        {
            if (vao == 0) return;

            GL.BindVertexArray(vao);
            GL.DrawArrays(PrimitiveType.Triangles, 0, vertices.Count);
            GL.BindVertexArray(0);
        }

        public static Model FromObjFile(string filePath)
        {
            return ObjLoader.LoadFromFile(filePath);
        }
    }

    // Основной класс игры
    class SolarSystemGame : GameWindow
    {
        private Camera camera;
        private List<CelestialBody> celestialBodies;
        private Dictionary<string, Model> models;
        private Dictionary<string, int> textures;
        private Shader shader;

        // Параметры управления с увеличенной чувствительностью
        private float moveSpeed = 5.0f;
        private float mouseSensitivity = 0.003f; // Увеличено с 0.002
        private Vector2 lastMousePos;
        private bool firstMove = true;

        public SolarSystemGame(GameWindowSettings gameWindowSettings, NativeWindowSettings nativeWindowSettings)
            : base(gameWindowSettings, nativeWindowSettings)
        {
            camera = new Camera();
            celestialBodies = new List<CelestialBody>();
            models = new Dictionary<string, Model>();
            textures = new Dictionary<string, int>();
        }

        protected override void OnLoad()
        {
            base.OnLoad();

            GL.ClearColor(0.0f, 0.0f, 0.1f, 1.0f);
            GL.Enable(EnableCap.DepthTest);
            GL.Enable(EnableCap.Texture2D);

            shader = new Shader();

            LoadModels();
            LoadTextures();
            CreateSolarSystem();

            CursorState = CursorState.Grabbed;
            lastMousePos = new Vector2(MouseState.X, MouseState.Y);
        }

        private void LoadModels()
        {
            string basePath = AppDomain.CurrentDomain.BaseDirectory;
            string modelsPath = Path.Combine(basePath, "Models");

            bool objsLoaded = false;

            if (Directory.Exists(modelsPath))
            {
                objsLoaded = LoadObjModels(modelsPath);
            }
            else
            {
                Directory.CreateDirectory(modelsPath);
                Console.WriteLine("Создана папка Models. Добавьте туда ваши OBJ файлы.");
                CreateExampleObjFiles(modelsPath);
            }

            if (!objsLoaded)
            {
                Console.WriteLine("OBJ файлы не найдены. Создаю процедурные модели...");
                LoadProceduralModels();
            }
        }

        private bool LoadObjModels(string modelsPath)
        {
            bool anyLoaded = false;

            string[] modelFiles = {
                "spaceship.obj", "station.obj", "asteroid.obj",
                "satellite.obj", "comet.obj"
            };

            foreach (var fileName in modelFiles)
            {
                string filePath = Path.Combine(modelsPath, fileName);
                if (File.Exists(filePath))
                {
                    try
                    {
                        string modelName = Path.GetFileNameWithoutExtension(fileName);
                        models[modelName] = Model.FromObjFile(filePath);
                        anyLoaded = true;
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Ошибка загрузки {fileName}: {ex.Message}");
                    }
                }
            }

            return anyLoaded;
        }

        private void CreateExampleObjFiles(string modelsPath)
        {
            // Простой космический корабль
            string spaceshipObj = @"# Простой космический корабль в OBJ формате
v -1.0 -0.5 0.0
v 1.0 -0.5 0.0
v 0.0 1.0 0.0
v 0.0 -0.5 2.0
v -0.8 -0.5 -1.0
v 0.8 -0.5 -1.0
v -0.5 0.3 0.5
v 0.5 0.3 0.5

vt 0.0 0.0
vt 1.0 0.0
vt 0.5 1.0
vt 0.5 0.5
vt 0.0 0.5
vt 1.0 0.5
vt 0.25 0.75
vt 0.75 0.75

vn 0.0 0.0 1.0
vn 0.0 0.0 -1.0
vn 0.0 1.0 0.0
vn 0.0 -1.0 0.0
vn 1.0 0.0 0.0
vn -1.0 0.0 0.0

# Корпус
f 1/1/1 2/2/1 3/3/1
f 1/1/1 3/3/2 4/4/2
f 2/2/3 4/4/3 3/3/3
f 1/5/4 4/4/4 2/6/4
# Крылья
f 1/5/5 5/7/5 4/4/5
f 2/6/6 4/4/6 6/8/6
# Кабина
f 7/7/1 8/8/1 3/3/1";

            File.WriteAllText(Path.Combine(modelsPath, "spaceship.obj"), spaceshipObj);
            Console.WriteLine("Создан пример файла spaceship.obj в папке Models");
        }

        private void LoadProceduralModels()
        {
            models["spaceship"] = CreateSpaceshipModel();
            models["station"] = CreateSpaceStationModel();
            models["asteroid"] = CreateAsteroidModel();
            models["satellite"] = CreateSatelliteModel();
            models["comet"] = CreateCometModel();
        }

        private Model CreateSpaceshipModel()
        {
            var model = new Model();

            CreateCylinder(model, 1.0f, 0.5f, 3.0f, 20, 10, 0, 0, 0);
            CreateWing(model, -1.5f, 0, 0, 1.5f, 0.1f, 2.0f);
            CreateWing(model, 1.5f, 0, 0, 1.5f, 0.1f, 2.0f);
            CreateCylinder(model, 0.3f, 0.1f, 1.0f, 10, 5, 0, 0, -2.0f);
            CreateCylinder(model, 0.3f, 0.1f, 1.0f, 10, 5, 0.7f, 0.5f, -2.0f);
            CreateCylinder(model, 0.3f, 0.1f, 1.0f, 10, 5, -0.7f, 0.5f, -2.0f);

            model.Build();
            return model;
        }

        private Model CreateSpaceStationModel()
        {
            var model = new Model();

            CreateTorus(model, 1.5f, 0.3f, 20, 10, 0, 0, 0);
            CreateSphere(model, 0.8f, 12, 12, 0, 0, 0);
            CreateBox(model, 3.0f, 0.1f, 2.0f, 0, 1.5f, 0);
            CreateBox(model, 3.0f, 0.1f, 2.0f, 0, -1.5f, 0);
            CreateCylinder(model, 0.05f, 0.05f, 2.0f, 8, 3, 1.2f, 0, 0.5f);
            CreateCylinder(model, 0.05f, 0.05f, 2.0f, 8, 3, -1.2f, 0, 0.5f);

            model.Build();
            return model;
        }

        private Model CreateAsteroidModel()
        {
            var model = new Model();

            Random rand = new Random(42);
            CreateSphere(model, 1.0f, 16, 16, 0, 0, 0);

            for (int i = 0; i < 15; i++)
            {
                float angle1 = (float)(rand.NextDouble() * Math.PI * 2);
                float angle2 = (float)(rand.NextDouble() * Math.PI);
                float r = 1.0f + (float)rand.NextDouble() * 0.5f;

                float x = (float)(Math.Sin(angle2) * Math.Cos(angle1) * r);
                float y = (float)(Math.Sin(angle2) * Math.Sin(angle1) * r);
                float z = (float)(Math.Cos(angle2) * r);

                CreateSphere(model, 0.2f, 6, 6, x, y, z);
            }

            model.Build();
            return model;
        }

        private Model CreateSatelliteModel()
        {
            var model = new Model();

            CreateBox(model, 0.8f, 0.8f, 1.2f, 0, 0, 0);
            CreateCylinder(model, 0.03f, 0.03f, 1.5f, 6, 3, 0, 0, 1.0f);
            CreateCylinder(model, 0.03f, 0.03f, 1.0f, 6, 3, 0.5f, 0, 0);
            CreateCylinder(model, 0.03f, 0.03f, 1.0f, 6, 3, -0.5f, 0, 0);
            CreateBox(model, 0.1f, 2.0f, 1.5f, 0.6f, 0, 0);
            CreateBox(model, 0.1f, 2.0f, 1.5f, -0.6f, 0, 0);

            model.Build();
            return model;
        }

        private Model CreateCometModel()
        {
            var model = new Model();

            CreateSphere(model, 0.7f, 12, 12, 0, 0, 0);
            CreateCone(model, 0.8f, 2.5f, 12, 0, 0, -1.5f);

            model.Build();
            return model;
        }

        private void CreateSphere(Model model, float radius, int sectors, int stacks,
                                 float offsetX, float offsetY, float offsetZ)
        {
            for (int i = 0; i < stacks; ++i)
            {
                float lat0 = (float)Math.PI * (-0.5f + (float)(i) / stacks);
                float z0 = (float)Math.Sin(lat0);
                float zr0 = (float)Math.Cos(lat0);

                float lat1 = (float)Math.PI * (-0.5f + (float)(i + 1) / stacks);
                float z1 = (float)Math.Sin(lat1);
                float zr1 = (float)Math.Cos(lat1);

                for (int j = 0; j < sectors; ++j)
                {
                    float lng0 = 2.0f * (float)Math.PI * (float)(j) / sectors;
                    float x0 = (float)Math.Cos(lng0);
                    float y0 = (float)Math.Sin(lng0);

                    float lng1 = 2.0f * (float)Math.PI * (float)(j + 1) / sectors;
                    float x1 = (float)Math.Cos(lng1);
                    float y1 = (float)Math.Sin(lng1);

                    model.AddVertex(new Vector3(
                        offsetX + radius * x0 * zr0,
                        offsetY + radius * y0 * zr0,
                        offsetZ + radius * z0));
                    model.AddTexCoord(new Vector2((float)j / sectors, (float)i / stacks));

                    model.AddVertex(new Vector3(
                        offsetX + radius * x1 * zr0,
                        offsetY + radius * y1 * zr0,
                        offsetZ + radius * z0));
                    model.AddTexCoord(new Vector2((float)(j + 1) / sectors, (float)i / stacks));

                    model.AddVertex(new Vector3(
                        offsetX + radius * x0 * zr1,
                        offsetY + radius * y0 * zr1,
                        offsetZ + radius * z1));
                    model.AddTexCoord(new Vector2((float)j / sectors, (float)(i + 1) / stacks));

                    model.AddVertex(new Vector3(
                        offsetX + radius * x1 * zr0,
                        offsetY + radius * y1 * zr0,
                        offsetZ + radius * z0));
                    model.AddTexCoord(new Vector2((float)(j + 1) / sectors, (float)i / stacks));

                    model.AddVertex(new Vector3(
                        offsetX + radius * x1 * zr1,
                        offsetY + radius * y1 * zr1,
                        offsetZ + radius * z1));
                    model.AddTexCoord(new Vector2((float)(j + 1) / sectors, (float)(i + 1) / stacks));

                    model.AddVertex(new Vector3(
                        offsetX + radius * x0 * zr1,
                        offsetY + radius * y0 * zr1,
                        offsetZ + radius * z1));
                    model.AddTexCoord(new Vector2((float)j / sectors, (float)(i + 1) / stacks));
                }
            }
        }

        private void CreateCylinder(Model model, float radiusTop, float radiusBottom,
                                   float height, int sectors, int stacks,
                                   float offsetX, float offsetY, float offsetZ)
        {
            float stackHeight = height / stacks;

            for (int i = 0; i < stacks; ++i)
            {
                float y0 = -height / 2.0f + i * stackHeight;
                float y1 = y0 + stackHeight;
                float r0 = radiusBottom + (radiusTop - radiusBottom) * (i / (float)stacks);
                float r1 = radiusBottom + (radiusTop - radiusBottom) * ((i + 1) / (float)stacks);

                for (int j = 0; j < sectors; ++j)
                {
                    float angle0 = 2.0f * (float)Math.PI * j / sectors;
                    float angle1 = 2.0f * (float)Math.PI * (j + 1) / sectors;

                    float cos0 = (float)Math.Cos(angle0);
                    float sin0 = (float)Math.Sin(angle0);
                    float cos1 = (float)Math.Cos(angle1);
                    float sin1 = (float)Math.Sin(angle1);

                    model.AddVertex(new Vector3(offsetX + r0 * cos0, offsetY + y0, offsetZ + r0 * sin0));
                    model.AddTexCoord(new Vector2((float)j / sectors, (float)i / stacks));
                    model.AddVertex(new Vector3(offsetX + r0 * cos1, offsetY + y0, offsetZ + r0 * sin1));
                    model.AddTexCoord(new Vector2((float)(j + 1) / sectors, (float)i / stacks));
                    model.AddVertex(new Vector3(offsetX + r1 * cos0, offsetY + y1, offsetZ + r1 * sin0));
                    model.AddTexCoord(new Vector2((float)j / sectors, (float)(i + 1) / stacks));

                    model.AddVertex(new Vector3(offsetX + r0 * cos1, offsetY + y0, offsetZ + r0 * sin1));
                    model.AddTexCoord(new Vector2((float)(j + 1) / sectors, (float)i / stacks));
                    model.AddVertex(new Vector3(offsetX + r1 * cos1, offsetY + y1, offsetZ + r1 * sin1));
                    model.AddTexCoord(new Vector2((float)(j + 1) / sectors, (float)(i + 1) / stacks));
                    model.AddVertex(new Vector3(offsetX + r1 * cos0, offsetY + y1, offsetZ + r1 * sin0));
                    model.AddTexCoord(new Vector2((float)j / sectors, (float)(i + 1) / stacks));
                }
            }
        }

        private void CreateCylinder(Model model, float radius, float height,
                                   int sectors, float offsetX, float offsetY, float offsetZ)
        {
            CreateCylinder(model, radius, radius, height, sectors, 1, offsetX, offsetY, offsetZ);
        }

        private void CreateBox(Model model, float width, float height, float depth,
                              float offsetX, float offsetY, float offsetZ)
        {
            float w = width / 2;
            float h = height / 2;
            float d = depth / 2;

            AddQuad(model,
                new Vector3(offsetX - w, offsetY - h, offsetZ + d),
                new Vector3(offsetX + w, offsetY - h, offsetZ + d),
                new Vector3(offsetX + w, offsetY + h, offsetZ + d),
                new Vector3(offsetX - w, offsetY + h, offsetZ + d),
                new Vector2(0, 0), new Vector2(1, 0), new Vector2(1, 1), new Vector2(0, 1));

            AddQuad(model,
                new Vector3(offsetX + w, offsetY - h, offsetZ - d),
                new Vector3(offsetX - w, offsetY - h, offsetZ - d),
                new Vector3(offsetX - w, offsetY + h, offsetZ - d),
                new Vector3(offsetX + w, offsetY + h, offsetZ - d),
                new Vector2(0, 0), new Vector2(1, 0), new Vector2(1, 1), new Vector2(0, 1));

            AddQuad(model,
                new Vector3(offsetX - w, offsetY + h, offsetZ + d),
                new Vector3(offsetX + w, offsetY + h, offsetZ + d),
                new Vector3(offsetX + w, offsetY + h, offsetZ - d),
                new Vector3(offsetX - w, offsetY + h, offsetZ - d),
                new Vector2(0, 0), new Vector2(1, 0), new Vector2(1, 1), new Vector2(0, 1));

            AddQuad(model,
                new Vector3(offsetX - w, offsetY - h, offsetZ - d),
                new Vector3(offsetX + w, offsetY - h, offsetZ - d),
                new Vector3(offsetX + w, offsetY - h, offsetZ + d),
                new Vector3(offsetX - w, offsetY - h, offsetZ + d),
                new Vector2(0, 0), new Vector2(1, 0), new Vector2(1, 1), new Vector2(0, 1));

            AddQuad(model,
                new Vector3(offsetX - w, offsetY - h, offsetZ - d),
                new Vector3(offsetX - w, offsetY - h, offsetZ + d),
                new Vector3(offsetX - w, offsetY + h, offsetZ + d),
                new Vector3(offsetX - w, offsetY + h, offsetZ - d),
                new Vector2(0, 0), new Vector2(1, 0), new Vector2(1, 1), new Vector2(0, 1));

            AddQuad(model,
                new Vector3(offsetX + w, offsetY - h, offsetZ + d),
                new Vector3(offsetX + w, offsetY - h, offsetZ - d),
                new Vector3(offsetX + w, offsetY + h, offsetZ - d),
                new Vector3(offsetX + w, offsetY + h, offsetZ + d),
                new Vector2(0, 0), new Vector2(1, 0), new Vector2(1, 1), new Vector2(0, 1));
        }

        private void AddQuad(Model model, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4,
                            Vector2 t1, Vector2 t2, Vector2 t3, Vector2 t4)
        {
            model.AddVertex(v1); model.AddTexCoord(t1);
            model.AddVertex(v2); model.AddTexCoord(t2);
            model.AddVertex(v3); model.AddTexCoord(t3);

            model.AddVertex(v1); model.AddTexCoord(t1);
            model.AddVertex(v3); model.AddTexCoord(t3);
            model.AddVertex(v4); model.AddTexCoord(t4);
        }

        private void CreateCone(Model model, float radius, float height, int sectors,
                               float offsetX, float offsetY, float offsetZ)
        {
            for (int i = 0; i < sectors; ++i)
            {
                float angle0 = 2.0f * (float)Math.PI * i / sectors;
                float angle1 = 2.0f * (float)Math.PI * (i + 1) / sectors;

                float cos0 = (float)Math.Cos(angle0);
                float sin0 = (float)Math.Sin(angle0);
                float cos1 = (float)Math.Cos(angle1);
                float sin1 = (float)Math.Sin(angle1);

                model.AddVertex(new Vector3(offsetX, offsetY, offsetZ));
                model.AddTexCoord(new Vector2(0.5f, 0.5f));
                model.AddVertex(new Vector3(offsetX + radius * cos0, offsetY, offsetZ + radius * sin0));
                model.AddTexCoord(new Vector2(0.5f + 0.5f * cos0, 0.5f + 0.5f * sin0));
                model.AddVertex(new Vector3(offsetX + radius * cos1, offsetY, offsetZ + radius * sin1));
                model.AddTexCoord(new Vector2(0.5f + 0.5f * cos1, 0.5f + 0.5f * sin1));

                model.AddVertex(new Vector3(offsetX, offsetY - height, offsetZ));
                model.AddTexCoord(new Vector2(0.5f, 0.5f));
                model.AddVertex(new Vector3(offsetX + radius * cos1, offsetY - height, offsetZ + radius * sin1));
                model.AddTexCoord(new Vector2(0.5f + 0.5f * cos1, 0.5f + 0.5f * sin1));
                model.AddVertex(new Vector3(offsetX + radius * cos0, offsetY - height, offsetZ + radius * sin0));
                model.AddTexCoord(new Vector2(0.5f + 0.5f * cos0, 0.5f + 0.5f * sin0));
            }
        }

        private void CreateTorus(Model model, float radius, float tubeRadius,
                                int sides, int rings, float offsetX, float offsetY, float offsetZ)
        {
            for (int i = 0; i < sides; ++i)
            {
                float theta0 = 2.0f * (float)Math.PI * i / sides;
                float theta1 = 2.0f * (float)Math.PI * (i + 1) / sides;

                for (int j = 0; j < rings; ++j)
                {
                    float phi0 = 2.0f * (float)Math.PI * j / rings;
                    float phi1 = 2.0f * (float)Math.PI * (j + 1) / rings;

                    Vector3 v0 = GetTorusPoint(radius, tubeRadius, theta0, phi0);
                    Vector3 v1 = GetTorusPoint(radius, tubeRadius, theta1, phi0);
                    Vector3 v2 = GetTorusPoint(radius, tubeRadius, theta1, phi1);
                    Vector3 v3 = GetTorusPoint(radius, tubeRadius, theta0, phi1);

                    AddQuad(model,
                        new Vector3(offsetX + v0.X, offsetY + v0.Y, offsetZ + v0.Z),
                        new Vector3(offsetX + v1.X, offsetY + v1.Y, offsetZ + v1.Z),
                        new Vector3(offsetX + v2.X, offsetY + v2.Y, offsetZ + v2.Z),
                        new Vector3(offsetX + v3.X, offsetY + v3.Y, offsetZ + v3.Z),
                        new Vector2((float)i / sides, (float)j / rings),
                        new Vector2((float)(i + 1) / sides, (float)j / rings),
                        new Vector2((float)(i + 1) / sides, (float)(j + 1) / rings),
                        new Vector2((float)i / sides, (float)(j + 1) / rings));
                }
            }
        }

        private Vector3 GetTorusPoint(float radius, float tubeRadius, float theta, float phi)
        {
            return new Vector3(
                (radius + tubeRadius * (float)Math.Cos(phi)) * (float)Math.Cos(theta),
                tubeRadius * (float)Math.Sin(phi),
                (radius + tubeRadius * (float)Math.Cos(phi)) * (float)Math.Sin(theta));
        }

        private void CreateWing(Model model, float offsetX, float offsetY, float offsetZ,
                               float width, float thickness, float length)
        {
            float w = width / 2;
            float t = thickness / 2;
            float l = length / 2;

            for (int i = 0; i < 10; ++i)
            {
                float x0 = -w + 2 * w * i / 10.0f;
                float x1 = -w + 2 * w * (i + 1) / 10.0f;

                float z0 = (float)Math.Sin(x0 / w * Math.PI) * l;
                float z1 = (float)Math.Sin(x1 / w * Math.PI) * l;

                model.AddVertex(new Vector3(offsetX + x0, offsetY + t, offsetZ + z0));
                model.AddTexCoord(new Vector2(x0 / w / 2 + 0.5f, z0 / l / 2 + 0.5f));
                model.AddVertex(new Vector3(offsetX + x1, offsetY + t, offsetZ + z1));
                model.AddTexCoord(new Vector2(x1 / w / 2 + 0.5f, z1 / l / 2 + 0.5f));
                model.AddVertex(new Vector3(offsetX + x0, offsetY + t, offsetZ + z0 - 0.5f));
                model.AddTexCoord(new Vector2(x0 / w / 2 + 0.5f, (z0 - 0.5f) / l / 2 + 0.5f));

                model.AddVertex(new Vector3(offsetX + x1, offsetY + t, offsetZ + z1));
                model.AddTexCoord(new Vector2(x1 / w / 2 + 0.5f, z1 / l / 2 + 0.5f));
                model.AddVertex(new Vector3(offsetX + x1, offsetY + t, offsetZ + z1 - 0.5f));
                model.AddTexCoord(new Vector2(x1 / w / 2 + 0.5f, (z1 - 0.5f) / l / 2 + 0.5f));
                model.AddVertex(new Vector3(offsetX + x0, offsetY + t, offsetZ + z0 - 0.5f));
                model.AddTexCoord(new Vector2(x0 / w / 2 + 0.5f, (z0 - 0.5f) / l / 2 + 0.5f));
            }
        }

        private void LoadTextures()
        {
            textures["spaceship"] = CreateProceduralTexture(256, 256,
                (x, y) => new Vector3(
                    (float)Math.Sin(x * 0.1f) * 0.5f + 0.5f,
                    (float)Math.Cos(y * 0.1f) * 0.3f + 0.5f,
                    0.8f));

            textures["station"] = CreateProceduralTexture(256, 256,
                (x, y) => new Vector3(
                    ((x ^ y) % 64) / 64.0f,
                    ((x * 2 ^ y * 3) % 64) / 64.0f,
                    0.6f));

            textures["asteroid"] = CreateProceduralTexture(256, 256,
                (x, y) =>
                {
                    float noise = (float)(Math.Sin(x * 0.05f) * Math.Cos(y * 0.05f)) * 0.3f + 0.5f;
                    return new Vector3(noise * 0.7f, noise * 0.5f, noise * 0.3f);
                });

            textures["satellite"] = CreateProceduralTexture(256, 256,
                (x, y) => new Vector3(
                    ((x % 32 < 16) ^ (y % 32 < 16)) ? 0.9f : 0.3f,
                    ((x % 32 < 16) ^ (y % 32 < 16)) ? 0.9f : 0.3f,
                    0.9f));

            textures["comet"] = CreateProceduralTexture(256, 256,
                (x, y) =>
                {
                    float dist = (float)Math.Sqrt((x - 128) * (x - 128) + (y - 128) * (y - 128)) / 128.0f;
                    float intensity = Math.Max(0, 1 - dist * 2);
                    return new Vector3(0.8f, 0.8f * intensity, 0.6f * intensity);
                });
        }

        private int CreateProceduralTexture(int width, int height, Func<int, int, Vector3> colorFunc)
        {
            byte[] data = new byte[width * height * 4];

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int index = (y * width + x) * 4;
                    Vector3 color = colorFunc(x, y);

                    data[index] = (byte)(color.X * 255);
                    data[index + 1] = (byte)(color.Y * 255);
                    data[index + 2] = (byte)(color.Z * 255);
                    data[index + 3] = 255;
                }
            }

            int textureId = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, textureId);

            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba,
                         width, height, 0, PixelFormat.Rgba,
                         PixelType.UnsignedByte, data);

            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter,
                          (int)TextureMinFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter,
                          (int)TextureMagFilter.Linear);

            return textureId;
        }

        private void CreateSolarSystem()
        {
            // Создаем модели по умолчанию, если не загрузились OBJ
            Model defaultModel = CreateDefaultModel();
            int defaultTexture = CreateDefaultTexture();

            // Солнце - скорость вращения увеличена в 1.5 раза (0.15 вместо 0.1)
            celestialBodies.Add(new CelestialBody
            {
                Name = "Солнце",
                Model = models.ContainsKey("spaceship") ? models["spaceship"] : defaultModel,
                TextureId = textures.ContainsKey("spaceship") ? textures["spaceship"] : defaultTexture,
                Position = Vector3.Zero,
                Scale = 3.0f,
                RotationSpeed = 0.15f, // 0.1 * 1.5
                OrbitRadius = 0,
                OrbitSpeed = 0
            });

            // Планеты с увеличенными скоростями вращения
            celestialBodies.Add(new CelestialBody
            {
                Name = "Меркурий",
                Model = models.ContainsKey("station") ? models["station"] : defaultModel,
                TextureId = textures.ContainsKey("station") ? textures["station"] : defaultTexture,
                Position = new Vector3(10, 0, 0),
                Scale = 0.8f,
                RotationSpeed = 0.75f, // 0.5 * 1.5
                OrbitRadius = 10,
                OrbitSpeed = 0.8f
            });

            celestialBodies.Add(new CelestialBody
            {
                Name = "Венера",
                Model = models.ContainsKey("asteroid") ? models["asteroid"] : defaultModel,
                TextureId = textures.ContainsKey("asteroid") ? textures["asteroid"] : defaultTexture,
                Position = new Vector3(15, 2, 0),
                Scale = 1.2f,
                RotationSpeed = 0.45f, // 0.3 * 1.5
                OrbitRadius = 15,
                OrbitSpeed = 0.6f
            });

            celestialBodies.Add(new CelestialBody
            {
                Name = "Земля",
                Model = models.ContainsKey("satellite") ? models["satellite"] : defaultModel,
                TextureId = textures.ContainsKey("satellite") ? textures["satellite"] : defaultTexture,
                Position = new Vector3(20, 0, 0),
                Scale = 1.0f,
                RotationSpeed = 0.6f, // 0.4 * 1.5
                OrbitRadius = 20,
                OrbitSpeed = 0.5f
            });

            celestialBodies.Add(new CelestialBody
            {
                Name = "Марс",
                Model = models.ContainsKey("comet") ? models["comet"] : defaultModel,
                TextureId = textures.ContainsKey("comet") ? textures["comet"] : defaultTexture,
                Position = new Vector3(25, -1, 0),
                Scale = 0.9f,
                RotationSpeed = 0.525f, // 0.35 * 1.5
                OrbitRadius = 25,
                OrbitSpeed = 0.4f
            });

            celestialBodies.Add(new CelestialBody
            {
                Name = "Юпитер",
                Model = models.ContainsKey("spaceship") ? models["spaceship"] : defaultModel,
                TextureId = textures.ContainsKey("spaceship") ? textures["spaceship"] : defaultTexture,
                Position = new Vector3(35, 3, 0),
                Scale = 2.5f,
                RotationSpeed = 0.3f, // 0.2 * 1.5
                OrbitRadius = 35,
                OrbitSpeed = 0.2f
            });
        }

        private Model CreateDefaultModel()
        {
            var model = new Model();

            float size = 1.0f;

            model.AddVertex(new Vector3(-size, -size, size)); model.AddTexCoord(new Vector2(0, 0));
            model.AddVertex(new Vector3(size, -size, size)); model.AddTexCoord(new Vector2(1, 0));
            model.AddVertex(new Vector3(size, size, size)); model.AddTexCoord(new Vector2(1, 1));

            model.AddVertex(new Vector3(-size, -size, size)); model.AddTexCoord(new Vector2(0, 0));
            model.AddVertex(new Vector3(size, size, size)); model.AddTexCoord(new Vector2(1, 1));
            model.AddVertex(new Vector3(-size, size, size)); model.AddTexCoord(new Vector2(0, 1));

            model.Build();
            return model;
        }

        private int CreateDefaultTexture()
        {
            return CreateProceduralTexture(256, 256,
                (x, y) => new Vector3(0.5f, 0.5f, 0.5f));
        }

        protected override void OnRenderFrame(FrameEventArgs args)
        {
            base.OnRenderFrame(args);

            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

            UpdatePlanets((float)args.Time);

            Matrix4 projection = Matrix4.CreatePerspectiveFieldOfView(
                MathHelper.PiOver4, Size.X / (float)Size.Y, 0.1f, 1000.0f);
            Matrix4 view = camera.GetViewMatrix();

            shader.Use();
            shader.SetMatrix4("projection", projection);
            shader.SetMatrix4("view", view);

            foreach (var body in celestialBodies)
            {
                Matrix4 modelMatrix = Matrix4.CreateScale(body.Scale) *
                                    Matrix4.CreateRotationY(body.Rotation) *
                                    Matrix4.CreateTranslation(body.Position);

                shader.SetMatrix4("model", modelMatrix);
                shader.SetInt("texture0", 0);

                GL.ActiveTexture(TextureUnit.Texture0);
                GL.BindTexture(TextureTarget.Texture2D, body.TextureId);

                body.Model.Render();
            }

            Context.SwapBuffers();
        }

        private void UpdatePlanets(float deltaTime)
        {
            foreach (var body in celestialBodies)
            {
                if (body.OrbitRadius > 0)
                {
                    body.OrbitAngle += body.OrbitSpeed * deltaTime;
                    body.Position = new Vector3(
                        (float)Math.Cos(body.OrbitAngle) * body.OrbitRadius,
                        (float)Math.Sin(body.OrbitAngle) * 0.5f,
                        (float)Math.Sin(body.OrbitAngle) * body.OrbitRadius * 0.3f);
                }

                body.Rotation += body.RotationSpeed * deltaTime;
            }
        }

        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            base.OnUpdateFrame(args);

            if (!IsFocused)
                return;

            HandleInput((float)args.Time);
            UpdateCamera((float)args.Time);
        }

        private void HandleInput(float deltaTime)
        {
            var keyboard = KeyboardState;

            if (keyboard.IsKeyDown(Keys.W))
                camera.Move(Camera.MovementDirection.Forward, deltaTime, moveSpeed);
            if (keyboard.IsKeyDown(Keys.S))
                camera.Move(Camera.MovementDirection.Backward, deltaTime, moveSpeed);
            if (keyboard.IsKeyDown(Keys.A))
                camera.Move(Camera.MovementDirection.Left, deltaTime, moveSpeed);
            if (keyboard.IsKeyDown(Keys.D))
                camera.Move(Camera.MovementDirection.Right, deltaTime, moveSpeed);
            if (keyboard.IsKeyDown(Keys.Space))
                camera.Move(Camera.MovementDirection.Up, deltaTime, moveSpeed);
            if (keyboard.IsKeyDown(Keys.LeftShift))
                camera.Move(Camera.MovementDirection.Down, deltaTime, moveSpeed);

            if (keyboard.IsKeyDown(Keys.Escape))
                Close();
        }

        private void UpdateCamera(float deltaTime)
        {
            var mouse = MouseState;

            if (firstMove)
            {
                lastMousePos = new Vector2(mouse.X, mouse.Y);
                firstMove = false;
            }
            else
            {
                var delta = new Vector2(mouse.X - lastMousePos.X, mouse.Y - lastMousePos.Y);
                lastMousePos = new Vector2(mouse.X, mouse.Y);

                camera.Rotate(delta.X * mouseSensitivity, delta.Y * mouseSensitivity);
            }
        }

        protected override void OnResize(ResizeEventArgs e)
        {
            base.OnResize(e);
            GL.Viewport(0, 0, Size.X, Size.Y);
        }

        protected override void OnMouseWheel(MouseWheelEventArgs e)
        {
            base.OnMouseWheel(e);
            moveSpeed += e.OffsetY * 0.5f;
            moveSpeed = Math.Max(0.1f, Math.Min(moveSpeed, 20.0f));
        }
    }

    // Класс небесного тела
    class CelestialBody
    {
        public string Name { get; set; }
        public Model Model { get; set; }
        public int TextureId { get; set; }
        public Vector3 Position { get; set; }
        public float Scale { get; set; }
        public float Rotation { get; set; }
        public float RotationSpeed { get; set; }
        public float OrbitRadius { get; set; }
        public float OrbitSpeed { get; set; }
        public float OrbitAngle { get; set; }
    }

    // Класс камеры
    class Camera
    {
        public enum MovementDirection
        {
            Forward,
            Backward,
            Left,
            Right,
            Up,
            Down
        }

        private Vector3 position;
        private Vector3 front;
        private Vector3 up;
        private Vector3 right;
        private Vector3 worldUp;

        private float yaw;
        private float pitch;

        public Camera()
        {
            position = new Vector3(0, 5, 30);
            worldUp = Vector3.UnitY;
            front = -Vector3.UnitZ;
            yaw = -90.0f;
            pitch = 0.0f;
            UpdateVectors();
        }

        public Matrix4 GetViewMatrix()
        {
            return Matrix4.LookAt(position, position + front, up);
        }

        public void Move(MovementDirection direction, float deltaTime, float speed)
        {
            float velocity = speed * deltaTime;

            switch (direction)
            {
                case MovementDirection.Forward:
                    position += front * velocity;
                    break;
                case MovementDirection.Backward:
                    position -= front * velocity;
                    break;
                case MovementDirection.Left:
                    position -= right * velocity;
                    break;
                case MovementDirection.Right:
                    position += right * velocity;
                    break;
                case MovementDirection.Up:
                    position += up * velocity;
                    break;
                case MovementDirection.Down:
                    position -= up * velocity;
                    break;
            }
        }

        public void Rotate(float yawOffset, float pitchOffset)
        {
            yaw += yawOffset;
            pitch += pitchOffset;

            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            UpdateVectors();
        }

        private void UpdateVectors()
        {
            Vector3 newFront;
            newFront.X = (float)Math.Cos(MathHelper.DegreesToRadians(yaw)) *
                        (float)Math.Cos(MathHelper.DegreesToRadians(pitch));
            newFront.Y = (float)Math.Sin(MathHelper.DegreesToRadians(pitch));
            newFront.Z = (float)Math.Sin(MathHelper.DegreesToRadians(yaw)) *
                        (float)Math.Cos(MathHelper.DegreesToRadians(pitch));

            front = Vector3.Normalize(newFront);
            right = Vector3.Normalize(Vector3.Cross(front, worldUp));
            up = Vector3.Normalize(Vector3.Cross(right, front));
        }
    }

    // Класс шейдера
    class Shader
    {
        private int programId;

        public Shader()
        {
            string vertexShaderSource = @"
                #version 410 core
                layout(location = 0) in vec3 aPos;
                layout(location = 1) in vec2 aTexCoord;
                
                out vec2 TexCoord;
                
                uniform mat4 model;
                uniform mat4 view;
                uniform mat4 projection;
                
                void main()
                {
                    gl_Position = projection * view * model * vec4(aPos, 1.0);
                    TexCoord = aTexCoord;
                }";

            string fragmentShaderSource = @"
                #version 410 core
                in vec2 TexCoord;
                out vec4 FragColor;
                
                uniform sampler2D texture0;
                
                void main()
                {
                    FragColor = texture(texture0, TexCoord);
                }";

            int vertexShader = GL.CreateShader(ShaderType.VertexShader);
            GL.ShaderSource(vertexShader, vertexShaderSource);
            GL.CompileShader(vertexShader);

            int fragmentShader = GL.CreateShader(ShaderType.FragmentShader);
            GL.ShaderSource(fragmentShader, fragmentShaderSource);
            GL.CompileShader(fragmentShader);

            programId = GL.CreateProgram();
            GL.AttachShader(programId, vertexShader);
            GL.AttachShader(programId, fragmentShader);
            GL.LinkProgram(programId);

            GL.DeleteShader(vertexShader);
            GL.DeleteShader(fragmentShader);
        }

        public void Use()
        {
            GL.UseProgram(programId);
        }

        public void SetMatrix4(string name, Matrix4 matrix)
        {
            int location = GL.GetUniformLocation(programId, name);
            GL.UniformMatrix4(location, false, ref matrix);
        }

        public void SetInt(string name, int value)
        {
            int location = GL.GetUniformLocation(programId, name);
            GL.Uniform1(location, value);
        }
    }
}
