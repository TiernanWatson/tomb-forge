#include "Engine/Rendering/JoltDebugRenderer.h"

#include <string_view>

#include <glm/gtc/matrix_transform.hpp>

#include "Engine/Rendering/Graphics.h"
#include "Engine/Rendering/ShaderCache.h"

namespace TombForge
{
    void JoltDebugRenderer::SubmitLines(const Transform& transform, float fovY, float aspect, float near, float far)
    {
        if (m_lineBuffer.empty() && m_timedLines.empty())
        {
            return;
        }

        Graphics& graphics = Graphics::Get();

        const glm::mat4 cameraView = glm::inverse(transform.AsMatrix());
        const glm::mat4 projection = glm::perspective(fovY, aspect, near, far);

        graphics.UseShader(ShaderCache::Get().GetLineShader()->GetHandle());
        graphics.SetMatrix4("view", cameraView);
        graphics.SetMatrix4("projection", projection);
        graphics.SetMatrix4("model", glm::mat4{ 1.0f });

        if (!m_lineBuffer.empty())
        {
            graphics.DrawLines(m_lineBuffer);
        }

        if (!m_timedLines.empty())
        {
            graphics.DrawLines(m_timedLines);
        }
    }

    void JoltDebugRenderer::DrawColoredLine(glm::vec3 from, glm::vec3 to, glm::vec4 color)
    {
        m_lineBuffer.emplace_back(LineVertex{ from, color }, LineVertex{ to, color });
    }

    void JoltDebugRenderer::DrawColoredLine(glm::vec3 from, glm::vec3 to, glm::vec4 color, float time)
    {
        m_timedLines.emplace_back(LineVertex{ from, color }, LineVertex{ to, color });
    }

    void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
    {
        glm::vec3 p0{ inFrom.GetX(), inFrom.GetY(), inFrom.GetZ() };
        glm::vec3 p1{ inTo.GetX(), inTo.GetY(), inTo.GetZ() };

        glm::vec4 c0{ inColor.r, inColor.g, inColor.b, inColor.a };

        m_lineBuffer.emplace_back(LineVertex{ p0, c0 }, LineVertex{ p1, c0 });
    }

    void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
    {
        Graphics::Get().UseShader(ShaderCache::Get().GetLineShader()->GetHandle());

        glm::vec3 p0{ inV1.GetX(), inV1.GetY(), inV1.GetZ() };
        glm::vec3 p1{ inV2.GetX(), inV2.GetY(), inV2.GetZ() };
        glm::vec3 p2{ inV3.GetX(), inV3.GetY(), inV3.GetZ() };

        Graphics::Get().DrawTriangle(::TombForge::Triangle{ p0, p1, p2 });
    }

    void JoltDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
    {
        // todo: implement
    }
}
