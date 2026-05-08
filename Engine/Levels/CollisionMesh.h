#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Geometry/IndexedTriangle.h>

#include "Engine/Assets/AssetId.h"

namespace TombForge
{
    struct CollisionMesh : public AssetBase
    {
        JPH::VertexList vertices{};
        JPH::IndexedTriangleList indices{};
    };
}
