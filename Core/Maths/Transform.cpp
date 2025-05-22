#include "Transform.h"

#include <glm/gtx/transform.hpp>

namespace TombForge
{
    glm::mat4 Transform::AsMatrix() const
    {
        return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scale);
    }

    glm::vec3 Transform::EulerRotation() const
    {
        return glm::eulerAngles(rotation);
    }

    glm::vec3 Transform::ForwardVector() const
    {
        return rotation * glm::vec3{ 0.0f, 0.0f, -1.0f };
    }

    void Transform::SetEulers(glm::vec3 eulerRotations)
    {
        rotation = glm::quat(eulerRotations);
    }

    void Transform::SetEulers(float x, float y, float z)
    {
        rotation = glm::quat({ x, y, z });
    }

    Transform Transform::operator*(const Transform& t2)
    {
        glm::vec3 pos = position + t2.position;
        glm::vec3 scale = scale * t2.scale;
        glm::quat rot = rotation * t2.rotation;
        return Transform(rot, pos, scale);
    }
}
