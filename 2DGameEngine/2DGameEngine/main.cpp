
#include <iostream>
#include <chrono>

#include "flecs/flecs.h"
#include "raylib/raylib.h"
#include "raylib/raymath.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define GLSL_VERSION            330

#include "PerlinNoise.hpp"

#include <vector>
#include <string>
#include <map>

#include "CubeMeshData.h"
#include "DrawMeshInstancedFlattenedTransforms.h"

#define ThreeDimensionalStdVector(_name, _type, _size) std::vector<std::vector<std::vector<_type>>> _name(_size, std::vector<std::vector<_type>>(_size, std::vector<_type>(_size)));
#define TwoDimensionalStdVector(_name, _type, _size) std::vector<std::vector<_type>> _name(_size, std::vector<_type>(_size));

class VerticesPerFace {
public:

};

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, std::vector<float16>>& transformOfVerticesOfFaceInParticularDir, Vector3 position);
Mesh PlaneFacingDir(Vector3 dir);

bool ShouldDrawChunk(Vector3 curChunkPos, Vector3 cameraPos, Vector3 cameraLookDir);

Vector3 up = { 0, 1, 0 };
Vector3 down = { 0, -1, 0 };
Vector3 front = { 0, 0, 1 };
Vector3 back = { 0, 0, -1 };
Vector3 right = { 1, 0, 0 };
Vector3 left = { -1, 0, 0 };

const int numChunks = 10;
const int chunkSize = 10;

Color randColors[9] = {LIGHTGRAY, BLACK, RED, YELLOW, PINK, GREEN, BLUE, PURPLE, GOLD};

void SetRandomColour(float* ambient, int x, int y) {
    Color curColor = randColors[(x + y) % 9];
    ambient[0] = curColor.r;
    ambient[1] = curColor.g;
    ambient[2] = curColor.b;
    ambient[3] = curColor.a;
}

bool once = true;

int main()
{

    flecs::world world;

    auto e = world.entity();

    InitWindow(800, 450, "raylib [core] example - basic window");

    Camera camera = { { 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

    Vector3 position = { 0.0f, 0.0f, 0.0f };

    Texture2D textureLoad = LoadTexture("texture_test_small.png");

    Color colours[9] = { MAGENTA, WHITE, LIGHTGRAY, YELLOW, BLUE, RED, GREEN, ORANGE, VIOLET };

    std::vector<Color> perChunkColours;

    Vector3 curPos = { 0, 0, 0 };

    const int numChunksWidth = (2 * numChunks) + 1;
    std::unordered_map<BlockFaceDirection, Mesh> chunkMeshFacingParticularDir;

    Mesh planeFacingUp = { 0 };
    Mesh planeFacingDown = { 0 };
    Mesh planeFacingFront = { 0 };
    Mesh planeFacingBack = { 0 };
    Mesh planeFacingRight = { 0 };
    Mesh planeFacingLeft = { 0 };

    planeFacingUp = PlaneFacingDir(up);

    planeFacingDown = PlaneFacingDir(down);
    planeFacingFront = PlaneFacingDir(front);
    planeFacingBack = PlaneFacingDir(back);
    planeFacingRight = PlaneFacingDir(right);
    planeFacingLeft = PlaneFacingDir(left);

    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::UP, planeFacingUp });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::DOWN, planeFacingDown });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::FRONT, planeFacingFront });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::BACK, planeFacingBack });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::RIGHT, planeFacingRight });
    chunkMeshFacingParticularDir.insert({ BlockFaceDirection::LEFT, planeFacingLeft });

    std::vector<std::vector<std::unordered_map<BlockFaceDirection, std::vector<float16>>>> chunkTransformOfVerticesOfFaceInParticularDir(numChunksWidth, std::vector<std::unordered_map<BlockFaceDirection, std::vector<float16>>>(numChunksWidth));
    Vector3 curChunkPos = { 0, 0, 0 };
    for (int i = -numChunks; i <= numChunks; i++)
    {
        curChunkPos.x = i * chunkSize * 2;

        for (int j = -numChunks; j <= numChunks; j++)
        {
            curChunkPos.z = j * 2 * chunkSize;
            GenMeshCustom(chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks], curChunkPos);

        }
    }


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
    
    DisableCursor();

    while (!WindowShouldClose())
    {
        auto curTime = std::chrono::high_resolution_clock::now();

        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
        cameraDir = Vector3Normalize(cameraDir);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawFPS(40, 40);

            BeginMode3D(camera);

            std::vector<float16> drawingTransformsUp;
            std::vector<float16> drawingTransformsDown;
            std::vector<float16> drawingTransformsFront;
            std::vector<float16> drawingTransformsBack;
            std::vector<float16> drawingTransformsRight;
            std::vector<float16> drawingTransformsLeft;

            for (int i = -numChunks; i <= numChunks; i++)
            {
                for (int j = -numChunks; j <= numChunks; j++)
                {
                    Vector3 curChunkPos = { i * ((chunkSize * 2) + 1), 0, (j * ((chunkSize * 2) + 1))};

                    if (ShouldDrawChunk(curChunkPos, camera.position, cameraDir)) {

                        drawingTransformsUp.insert(drawingTransformsUp.end()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::UP].begin()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::UP].end());

                        drawingTransformsDown.insert(drawingTransformsDown.end()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::DOWN].begin()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::DOWN].end());

                        drawingTransformsFront.insert(drawingTransformsFront.end()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::FRONT].begin()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::FRONT].end());

                        drawingTransformsBack.insert(drawingTransformsBack.end()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::BACK].begin()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::BACK].end());

                        drawingTransformsRight.insert(drawingTransformsRight.end()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::RIGHT].begin()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::RIGHT].end());

                        drawingTransformsLeft.insert(drawingTransformsLeft.end()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::LEFT].begin()
                            , chunkTransformOfVerticesOfFaceInParticularDir[i + numChunks][j + numChunks][BlockFaceDirection::LEFT].end());

                    }
                }
            }

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::UP]
                , instancedMaterial
                , drawingTransformsUp.data()
                , drawingTransformsUp.size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::DOWN]
                , instancedMaterial
                , drawingTransformsDown.data()
                , drawingTransformsDown.size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::FRONT]
                , instancedMaterial
                , drawingTransformsFront.data()
                , drawingTransformsFront.size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::BACK]
                , instancedMaterial
                , drawingTransformsBack.data()
                , drawingTransformsBack.size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::RIGHT]
                , instancedMaterial
                , drawingTransformsRight.data()
                , drawingTransformsRight.size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::LEFT]
                , instancedMaterial
                , drawingTransformsLeft.data()
                , drawingTransformsLeft.size());


            DrawGrid(10, 1.0);

            EndMode3D();
        EndDrawing();
    }

    UnloadShader(instanceShader);
    UnloadTexture(textureLoad);

    CloseWindow();
    return 0;
}

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, std::vector<float16>> &transformOfVerticesOfFaceInParticularDir
    , Vector3 position)
{
    const siv::PerlinNoise::seed_type seed = 123456u;

    const siv::PerlinNoise perlin{ seed };

    float scale = 0.1f;

    for (int y = -chunkSize; y <= chunkSize; y++)
    {
        for (int x = -chunkSize; x <= chunkSize; x++)
        {
            for (int z = -chunkSize; z <= chunkSize; z++)
            {
                int _x = x + chunkSize + position.x;
                int _y = y + chunkSize + position.y;
                int _z = z + chunkSize + position.z;

                float curNoise = perlin.noise3D_01((double)_x * scale, (double)_z * scale, (double)_y * scale);

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

                    float curNoiseTop = perlin.noise3D_01((double)_x * scale, (double)_z * scale, (double)(_y + 1) * scale);

                    if (curNoiseTop < emptyThreshold || y == chunkSize) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].push_back(curTransformFlattened);
                    }

                    float curNoiseBottom = perlin.noise3D_01((double)_x * scale, (double)_z * scale, (double)(_y - 1) * scale);

                    if (curNoiseBottom < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].push_back(curTransformFlattened);
                    }

                    float curNoiseFront = perlin.noise3D_01((double)_x * scale, (double)(_z + 1) * scale, (double)_y * scale);

                    if (curNoiseFront < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].push_back(curTransformFlattened);
                    }

                    float curNoiseBack = perlin.noise3D_01((double)_x * scale, (double)(_z - 1) * scale, (double)_y * scale);

                    if (curNoiseBack < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].push_back(curTransformFlattened);
                    }

                    float curNoiseRight = perlin.noise3D_01((double)(_x + 1) * scale, (double)_z * scale, (double)_y * scale);

                    if (curNoiseRight < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].push_back(curTransformFlattened);
                    }

                    float curNoiseLeft = perlin.noise3D_01((double)(_x - 1) * scale, (double)_z * scale, (double)_y * scale);

                    if (curNoiseLeft < emptyThreshold) {
                        transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].push_back(curTransformFlattened);
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
}

bool ShouldDrawChunk(Vector3 curChunkPos, Vector3 cameraPos, Vector3 cameraLookDir) {

    float dotProduct = Vector3DotProduct(cameraLookDir, Vector3Normalize(Vector3Subtract(curChunkPos, cameraPos)));
    return dotProduct > 0;
}