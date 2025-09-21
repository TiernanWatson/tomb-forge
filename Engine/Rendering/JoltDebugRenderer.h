#pragma once

#include <vector>

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include "Core/Graphics/Line.h"
#include "Core/Maths/Transform.h"
#include "Engine/Rendering/Graphics.h"
#include "Engine/Rendering/Shader.h"

namespace TombForge
{
    class JoltDebugRenderer : public JPH::DebugRendererSimple
    {
    public:
        void SubmitLines(const Transform& transform, float fovY, float aspect, float near, float far);
        void DrawColoredLine(glm::vec3 from, glm::vec3 to, glm::vec4 color);
        void DrawColoredLine(glm::vec3 from, glm::vec3 to, glm::vec4 color, float time);

        inline void ClearLines()
        {
            m_lineBuffer.clear();
        }

    private:
        virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
        virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
        virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;

        std::vector<Line> m_lineBuffer{}; // Holds all the lines to be drawn to increase efficiency by submitting in one go
        std::vector<Line> m_timedLines{}; // Lines that disappear after some time
    };
}

