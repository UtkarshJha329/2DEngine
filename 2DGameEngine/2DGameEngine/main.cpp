
#include <iostream>

#include "flecs/flecs.h"
#include "raylib/raylib.h"
#include "raylib/raymath.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define GLSL_VERSION            330

#include "PerlinNoise.hpp"

#include <vector>
#include <string>

#include "CubeMeshData.h"
#include "DrawMeshInstancedFlattenedTransforms.h"

#define ThreeDimensionalStdVector(_name, _type, _size) std::vector<std::vector<std::vector<_type>>> _name(_size, std::vector<std::vector<_type>>(_size, std::vector<_type>(_size)));
#define TwoDimensionalStdVector(_name, _type, _size) std::vector<std::vector<_type>> _name(_size, std::vector<_type>(_size));

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, Mesh>& meshFacingParticularDir
    , std::unordered_map<BlockFaceDirection, std::vector<float16>>& transformOfVerticesOfFaceInParticularDir
    , Vector3 position);
Mesh PlaneFacingDir(Vector3 dir);

Vector3 up = { 0, 1, 0 };
Vector3 down = { 0, -1, 0 };
Vector3 front = { 0, 0, 1 };
Vector3 back = { 0, 0, -1 };
Vector3 right = { 1, 0, 0 };
Vector3 left = { -1, 0, 0 };


int main()
{

    flecs::world world;

    auto e = world.entity();

    InitWindow(800, 450, "raylib [core] example - basic window");

    Camera camera = { { 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    // Model drawing position
    Vector3 position = { 0.0f, 0.0f, 0.0f };

    // We generate a checked image for texturing
    //Image checked = GenImageChecked(2, 2, 1, 1, RED, GREEN);
    //checked.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;;
    //checked.mipmaps = 2;
    //Texture2D texture = LoadTextureFromImage(checked);
    Texture2D textureLoad = LoadTexture("texture_test_small.png");
    //textureLoad.mipmaps = 4;
    //GenTextureMipmaps(&textureLoad);
    //texture.mipmaps = 4;
    //texture.mipmaps = 0;
    //GenTextureMipmaps(&texture);
    //UnloadImage(checked);

    Color colours[9] = { MAGENTA, WHITE, LIGHTGRAY, YELLOW, BLUE, RED, GREEN, ORANGE, VIOLET };

    const int numChunks = 3;
    //Model genModel[(numChunks * 2) + 1][(numChunks * 2) + 1];
    TwoDimensionalStdVector(genModel, Model, (numChunks * 2) + 1);
    std::vector<Color> perChunkColours;

    Vector3 curPos = { 0, 0, 0 };

    std::unordered_map<BlockFaceDirection, Mesh> meshFacingParticularDir;
    std::unordered_map<BlockFaceDirection, std::vector<float16>> transformOfVerticesOfFaceInParticularDir;
    GenMeshCustom(meshFacingParticularDir, transformOfVerticesOfFaceInParticularDir, curPos);

    // Load lighting instanceShader
    Shader instanceShader = LoadShader(TextFormat("Shaders/lighting_instancing.vert", GLSL_VERSION),
        TextFormat("Shaders/lighting.frag", GLSL_VERSION));
    // Get instanceShader locations
    instanceShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instanceShader, "mvp");
    instanceShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(instanceShader, "viewPos");
    instanceShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(instanceShader, "instanceTransform");

    // Set instanceShader value: ambient light level
    int ambientLoc = GetShaderLocation(instanceShader, "ambient");
    float ambientValue[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    SetShaderValue(instanceShader, ambientLoc, ambientValue, SHADER_UNIFORM_VEC4);

    // Create one light
    Vector3 lightPos = {50.0f, 50.0f, 0.0f};
    CreateLight(LIGHT_DIRECTIONAL, lightPos, Vector3Zero(), WHITE, instanceShader);


    Material instancedMaterial = LoadMaterialDefault();
    instancedMaterial.shader = instanceShader;
    instancedMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = textureLoad;
    
    //for (int i = 0; i < 18; i++)
    //{
    //    std::cout << "PRINTING BEFORE RENDERING: " << meshFacingParticularDir[BlockFaceDirection::UP].vertices[i] << std::endl;
    //}

    //const int chunkSize = 0;

    //for (int x = -numChunks; x <= numChunks; x++)
    //{
    //    for (int y = -numChunks; y <= numChunks; y++)
    //    {
    //        Vector3 curPos = { x * chunkSize, 0, y * chunkSize};

    //        genModel[x + numChunks][y + numChunks] = LoadModelFromMesh();
    //        genModel[x + numChunks][y + numChunks].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textureLoad;
    //        perChunkColours.push_back(colours[GetRandomValue(0, 9)]);
    //    }

    //}

    std::cout << "UP : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].size() << std::endl;
    std::cout << "DOWN : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].size() << std::endl;
    std::cout << "FRONT : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].size() << std::endl;
    std::cout << "BACK : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].size() << std::endl;
    std::cout << "RIGHT : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].size() << std::endl;
    std::cout << "LEFT : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].size() << std::endl;

    DisableCursor();

    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
        cameraDir = Vector3Normalize(cameraDir);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        DrawFPS(40, 40);

        //std::string curNumMeshes = "Total Num Chunks -\n" + std::to_string(((numChunks * 2) + 1) * ((numChunks * 2) + 1));
        //DrawText(curNumMeshes.c_str(), 300, 40, 30, MAGENTA);

            BeginMode3D(camera);
            //DrawModelWires(genModel, offsetPos, 1.0f, WHITE);
            //for (int x = -numChunks; x <= numChunks; x++)
            //{
            //    for (int y = -numChunks; y <= numChunks; y++)
            //    {
            //        Vector3 curPos = { x * chunkSize, 0, y * chunkSize };
            //        DrawModel(genModel[x + numChunks][y + numChunks], curPos, 1.0f, WHITE/*perChunkColours[((x + numChunks) * (numChunks)) + (y + numChunks)]*/);
            //    }

            //}

            //rlEnableShader(instancedMaterial.shader.id);

            DrawMeshInstancedFlattenedTransforms(meshFacingParticularDir[BlockFaceDirection::UP]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].size());

            DrawMeshInstancedFlattenedTransforms(meshFacingParticularDir[BlockFaceDirection::DOWN]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].size());

            DrawMeshInstancedFlattenedTransforms(meshFacingParticularDir[BlockFaceDirection::FRONT]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].size());

            DrawMeshInstancedFlattenedTransforms(meshFacingParticularDir[BlockFaceDirection::BACK]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].size());

            DrawMeshInstancedFlattenedTransforms(meshFacingParticularDir[BlockFaceDirection::RIGHT]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].size());

            DrawMeshInstancedFlattenedTransforms(meshFacingParticularDir[BlockFaceDirection::LEFT]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].size());

            DrawGrid(10, 1.0);

            EndMode3D();
        EndDrawing();
    }

    UnloadShader(instanceShader);
    UnloadTexture(textureLoad);

    CloseWindow();
    return 0;
}

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, Mesh> &meshFacingParticularDir
    , std::unordered_map<BlockFaceDirection, std::vector<float16>> &transformOfVerticesOfFaceInParticularDir
    , Vector3 position)
{
    const int chunkSize = 10;
    const int numChunks = 10;

    const siv::PerlinNoise::seed_type seed = 123456u;

    const siv::PerlinNoise perlin{ seed };

    Mesh planeFacingUp = { 0 };
    Mesh planeFacingDown = { 0 };
    Mesh planeFacingFront = { 0 };
    Mesh planeFacingBack = { 0 };
    Mesh planeFacingRight = { 0 };
    Mesh planeFacingLeft = { 0 };

    planeFacingUp = PlaneFacingDir(up);

    //for (int i = 0; i < 18; i++)
    //{
    //    std::cout << "PRINTING AFTER EXITING FUNCTION : " << planeFacingUp.vertices[i] << std::endl;
    //}

    planeFacingDown = PlaneFacingDir(down);
    planeFacingFront = PlaneFacingDir(front);
    planeFacingBack = PlaneFacingDir(back);
    planeFacingRight = PlaneFacingDir(right);
    planeFacingLeft = PlaneFacingDir(left);

    meshFacingParticularDir.insert({ BlockFaceDirection::UP, planeFacingUp });
    meshFacingParticularDir.insert({ BlockFaceDirection::DOWN, planeFacingDown });
    meshFacingParticularDir.insert({ BlockFaceDirection::FRONT, planeFacingFront });
    meshFacingParticularDir.insert({ BlockFaceDirection::BACK, planeFacingBack });
    meshFacingParticularDir.insert({ BlockFaceDirection::RIGHT, planeFacingRight });
    meshFacingParticularDir.insert({ BlockFaceDirection::LEFT, planeFacingLeft });

    for (int i = -numChunks; i <= numChunks; i++)
    {
        position.x = i * chunkSize;

        for (int j = -numChunks; j <= numChunks; j++)
        {
            position.z = j * chunkSize;

            for (int y = -chunkSize; y <= chunkSize; y++)
            {
                for (int x = -chunkSize; x <= chunkSize; x++)
                {
                    for (int z = -chunkSize; z <= chunkSize; z++)
                    {
                        int _x = x + chunkSize + position.x;
                        int _y = y + chunkSize + position.y;
                        int _z = z + chunkSize + position.z;

                        float curNoise = perlin.noise3D_01((double)_x * 0.1f, (double)_z * 0.1f, (double)_y * 0.1f);

                        float emptyThreshold = 0.5f;
                        bool curVoxelIsEmpty = curNoise < emptyThreshold ? true : false;

                        if (!curVoxelIsEmpty) {

                            Matrix translation = MatrixTranslate((float)_x, (float)_y, (float)_z);
                            Matrix rotation = MatrixRotate({ 0,1,0 }, 0);
                            //Matrix scale = MatrixScale(1.0f, 1.0f, 1.0f);

                            Matrix curTransform = MatrixMultiply(MatrixIdentity(), rotation);
                            curTransform = MatrixMultiply(curTransform, translation);

                            //Matrix curTransform = MatrixMultiply(rotation, translation);
                            float16 curTransformFlattened = MatrixToFloatV(curTransform);

                            float curNoiseTop = perlin.noise3D_01((double)_x * 0.1f, (double)_z * 0.1f, (double)(_y + 1) * 0.1f);

                            if (curNoiseTop < emptyThreshold || y == chunkSize) {
                                transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curTransformFlattened);
                            }

                            float curNoiseBottom = perlin.noise3D_01((double)_x * 0.1f, (double)_z * 0.1f, (double)(_y - 1) * 0.1f);

                            if (curNoiseBottom < emptyThreshold) {
                                transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curTransformFlattened);
                            }

                            float curNoiseFront = perlin.noise3D_01((double)_x * 0.1f, (double)(_z + 1) * 0.1f, (double)_y * 0.1f);

                            if (curNoiseFront < emptyThreshold) {
                                transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curTransformFlattened);
                            }

                            float curNoiseBack = perlin.noise3D_01((double)_x * 0.1f, (double)(_z - 1) * 0.1f, (double)_y * 0.1f);

                            if (curNoiseBack < emptyThreshold) {
                                transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curTransformFlattened);
                            }

                            float curNoiseRight = perlin.noise3D_01((double)(_x + 1) * 0.1f, (double)_z * 0.1f, (double)_y * 0.1f);

                            if (curNoiseRight < emptyThreshold) {
                                transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curTransformFlattened);
                            }

                            float curNoiseLeft = perlin.noise3D_01((double)(_x - 1) * 0.1f, (double)_z * 0.1f, (double)_y * 0.1f);

                            if (curNoiseLeft < emptyThreshold) {
                                transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].push_back(curTransformFlattened);
                            }

                        }

                    }
                }
            }
        }
    }
}

Mesh PlaneFacingDir(Vector3 dir) {

    Mesh curMesh = { 0 };

    curMesh.vertices = (float*)MemAlloc(18 * sizeof(float));
    curMesh.texcoords = (float*)MemAlloc(12 * sizeof(float));
    curMesh.indices = (unsigned short*)MemAlloc(6 * sizeof(unsigned short*));

    if (dir.x == 0 && dir.y == 1 && dir.z == 0) {
        FaceIndicesTop(curMesh.indices, 0);
        FaceVerticesTop(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == -1 && dir.z == 0) {
        FaceIndicesBottom(curMesh.indices, 0);
        FaceVerticesBottom(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == 0 && dir.z == 1) {
        FaceIndicesFront(curMesh.indices, 0);
        FaceVerticesFront(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 0 && dir.y == 0 && dir.z == -1) {
        FaceIndicesBack(curMesh.indices, 0);
        FaceVerticesBack(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == 1 && dir.y == 0 && dir.z == 0) {
        FaceIndicesRight(curMesh.indices, 0);
        FaceVerticesRight(curMesh.vertices, 0, 0, 0);
    }
    else if (dir.x == -1 && dir.y == 0 && dir.z == 0) {
        FaceIndicesLeft(curMesh.indices, 0);
        FaceVerticesLeft(curMesh.vertices, 0, 0, 0);
    }
    TexCoords(curMesh.texcoords);

    curMesh.triangleCount = 6 / 3;
    curMesh.vertexCount = 18 / 3;

    UploadMesh(&curMesh, false);
    
    return curMesh;

    if (dir.x == 0 && dir.y == 1 && dir.z == 0) {

        for (int i = 0; i < 18; i++)
        {
            std::cout << "After uploading mesh to gpu but inside function: " << curMesh.vertices[i] << std::endl;
        }
    }


}

