#pragma once

#include "raylib/raymath.h"

#include <unordered_map>

enum class BlockType
{
    AIR,
    DIRT,
    STONE,
    GRASS,
    COBBLESTONE,
    OAK_WOOD,
    LEAF
};

class BlockObject
{
public:

    BlockObject(BlockType _type, Vector2 _texCoordsUp, Vector2 _texCoordsSide, Vector2 _texCoordsBottom, bool _isSolid) {
        type = _type;
        texCoordsUp = _texCoordsUp;
        texCoordsSide = _texCoordsSide;
        texCoordsBottom = _texCoordsBottom;
        isSolid = _isSolid;
    }

    BlockType type;
    Vector2 texCoordsUp;
    Vector2 texCoordsSide;
    Vector2 texCoordsBottom;
    bool isSolid;
};

static class BlocksData
{
public:
    inline static const std::unordered_map<BlockType, BlockObject> blockObjectsData =
    {
        {BlockType::AIR,   {
                    BlockType::AIR,
                    Vector2{1, 14},
                    Vector2{1, 14},
                    Vector2{1, 14},
                    false
                }
        },
        {BlockType::DIRT,  {
                    BlockType::DIRT,
                    Vector2{7, 15},
                    Vector2{1, 15},
                    Vector2{2, 15},
                    true
                }
        },
        {BlockType::STONE,  {
                   BlockType::STONE,
                   Vector2{0, 15},
                   Vector2{0, 15},
                   Vector2{0, 15},
                   true
               }
        },
        {BlockType::GRASS,  {
                   BlockType::GRASS,
                   Vector2{6, 1},
                   Vector2{6, 1},
                   Vector2{6, 1},
                   true
               }
        }

    };
};
