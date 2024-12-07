
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

void RecalculateInstancePositions(std::vector<float16>& recalculatedInstanceTransformsList
    , std::vector<float16>& currentInstanceTransformsList
    , std::map<std::tuple<int, int, BlockFaceDirection>, int>& numVerticesPerChunkPerFace
    , BlockFaceDirection curFaceDir
    , Vector3 cameraPos, Vector3 cameraLookDir);

static void GenMeshCustom(std::unordered_map<BlockFaceDirection, std::vector<float16>>& transformOfVerticesOfFaceInParticularDir
    , Vector3 position);
Mesh PlaneFacingDir(Vector3 dir);

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

    //Model genModel[(numChunks * 2) + 1][(numChunks * 2) + 1];
    //TwoDimensionalStdVector(genModel, Model, (numChunks * 2) + 1);
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

    std::unordered_map<BlockFaceDirection, std::vector<float16>> transformOfVerticesOfFaceInParticularDir;
    std::map<std::tuple<int, int, BlockFaceDirection>, int> numVerticesPerChunkPerFace;

    Vector3 curChunkPos = { 0, 0, 0 };
    for (int i = -numChunks; i <= numChunks; i++)
    {
        curChunkPos.x = i * chunkSize * 2;

        for (int j = -numChunks; j <= numChunks; j++)
        {
            curChunkPos.z = j * 2 * chunkSize;

            int oldNumVerticesUp = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].size();
            int oldNumVerticesDown = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].size();
            int oldNumVerticesFront = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].size();
            int oldNumVerticesBack = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].size();
            int oldNumVerticesRight = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].size();
            int oldNumVerticesLeft = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].size();

            //std::cout << "Chunk coords : " << curChunkPos.x << ", " << curChunkPos.z << std::endl;
            GenMeshCustom(transformOfVerticesOfFaceInParticularDir, curChunkPos);
            
            numVerticesPerChunkPerFace[{i, j, BlockFaceDirection::UP}] = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].size() - oldNumVerticesUp;
            numVerticesPerChunkPerFace[{ i, j, BlockFaceDirection::DOWN}] = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].size() - oldNumVerticesDown;
            numVerticesPerChunkPerFace[{ i, j, BlockFaceDirection::FRONT}]  = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].size() - oldNumVerticesFront;
            numVerticesPerChunkPerFace[{ i, j, BlockFaceDirection::BACK}] = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].size() - oldNumVerticesBack;
            numVerticesPerChunkPerFace[{ i, j, BlockFaceDirection::RIGHT}]  = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].size() - oldNumVerticesRight;
            numVerticesPerChunkPerFace[{ i, j, BlockFaceDirection::LEFT}] = transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].size() - oldNumVerticesLeft;


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

    //std::cout << "UP : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].size() << std::endl;
    //std::cout << "DOWN : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].size() << std::endl;
    //std::cout << "FRONT : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].size() << std::endl;
    //std::cout << "BACK : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].size() << std::endl;
    //std::cout << "RIGHT : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].size() << std::endl;
    //std::cout << "LEFT : " << transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT].size() << std::endl;

    DisableCursor();

    std::vector<float16> reclaculatedChunksTransformUp;
    std::vector<float16> reclaculatedChunksTransformDown;
    std::vector<float16> reclaculatedChunksTransformFront;
    std::vector<float16> reclaculatedChunksTransformBack;
    std::vector<float16> reclaculatedChunksTransformRight;
    std::vector<float16> reclaculatedChunksTransformLeft;

    while (!WindowShouldClose())
    {
        auto curTime = std::chrono::high_resolution_clock::now();

        UpdateCamera(&camera, CAMERA_FREE);

        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(instancedMaterial.shader, instancedMaterial.shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        Vector3 cameraDir = Vector3Subtract(camera.target, camera.position);
        cameraDir = Vector3Normalize(cameraDir);

        //RecalculateInstancePositions(reclaculatedChunksTransformUp
        //    , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP]
        //    , numVerticesPerChunkPerFace, BlockFaceDirection::UP
        //    , position, cameraDir);

        //RecalculateInstancePositions(reclaculatedChunksTransformDown
        //    , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN]
        //    , numVerticesPerChunkPerFace, BlockFaceDirection::DOWN
        //    , position, cameraDir);

        //RecalculateInstancePositions(reclaculatedChunksTransformFront
        //    , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT]
        //    , numVerticesPerChunkPerFace, BlockFaceDirection::FRONT
        //    , position, cameraDir);

        //RecalculateInstancePositions(reclaculatedChunksTransformBack
        //    , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK]
        //    , numVerticesPerChunkPerFace, BlockFaceDirection::BACK
        //    , position, cameraDir);

        //RecalculateInstancePositions(reclaculatedChunksTransformRight
        //    , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT]
        //    , numVerticesPerChunkPerFace, BlockFaceDirection::RIGHT
        //    , position, cameraDir);

        //RecalculateInstancePositions(reclaculatedChunksTransformLeft
        //    , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::LEFT]
        //    , numVerticesPerChunkPerFace, BlockFaceDirection::LEFT
        //    , position, cameraDir);

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

            //DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::UP]
            //    , instancedMaterial
            //    , reclaculatedChunksTransformUp.data()
            //    , reclaculatedChunksTransformUp.size());

            //DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::DOWN]
            //    , instancedMaterial
            //    , reclaculatedChunksTransformDown.data()
            //    , reclaculatedChunksTransformDown.size());

            //DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::FRONT]
            //    , instancedMaterial
            //    , reclaculatedChunksTransformFront.data()
            //    , reclaculatedChunksTransformFront.size());

            //DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::BACK]
            //    , instancedMaterial
            //    , reclaculatedChunksTransformBack.data()
            //    , reclaculatedChunksTransformBack.size());

            //DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::RIGHT]
            //    , instancedMaterial
            //    , reclaculatedChunksTransformRight.data()
            //    , reclaculatedChunksTransformRight.size());

            //DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::LEFT]
            //    , instancedMaterial
            //    , reclaculatedChunksTransformLeft.data()
            //    , reclaculatedChunksTransformLeft.size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::UP]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::UP].size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::DOWN]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::DOWN].size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::FRONT]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::FRONT].size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::BACK]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::BACK].size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::RIGHT]
                , instancedMaterial
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].data()
                , transformOfVerticesOfFaceInParticularDir[BlockFaceDirection::RIGHT].size());

            DrawMeshInstancedFlattenedTransforms(chunkMeshFacingParticularDir[BlockFaceDirection::LEFT]
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

void RecalculateInstancePositions(std::vector<float16>& recalculatedInstanceTransformsList
    , std::vector<float16>& currentInstanceTransformsList
    , std::map<std::tuple<int, int, BlockFaceDirection>, int>& numVerticesPerChunkPerFace
    , BlockFaceDirection curFaceDir
    , Vector3 cameraPos, Vector3 cameraLookDir) {

    int numVerticesCrossed = 0;

    Vector3 curChunkPos = { 0, 0, 0 };
    for (int i = -numChunks; i <= numChunks; i++)
    {
        curChunkPos.x = i * chunkSize * 2;

        for (int j = -numChunks; j <= numChunks; j++)
        {
            curChunkPos.z = j * 2 * chunkSize;

            int curNumVertices = numVerticesPerChunkPerFace[{i, j, curFaceDir}];

            float dotProduct = Vector3DotProduct(cameraLookDir, Vector3Normalize(Vector3Subtract(curChunkPos, cameraPos)));
            if (dotProduct >= 0 /*curChunkPos.z > cameraPos.z && curChunkPos.x > cameraPos.x*/) {

                for (int k = numVerticesCrossed; k < numVerticesCrossed + curNumVertices; k++)
                {
                    recalculatedInstanceTransformsList.push_back(currentInstanceTransformsList[k]);
                }
            }
            numVerticesCrossed += curNumVertices;
        }
    }
}