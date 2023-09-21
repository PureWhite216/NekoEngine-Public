#include <glm/gtx/norm.hpp>
#include "LuaBinding.h"
#include "Transform.h"
#include "sol/sol.hpp"

namespace NekoEngine
{
    void BindMathsLua(sol::state &state)
    {
        state.new_usertype<glm::vec2>(
                "Vector2",
                sol::constructors<glm::vec2(float, float)>(),
                "x", &glm::vec2::x,
                "y", &glm::vec2::y,
                sol::meta_function::addition, [](const glm::vec2 &a, const glm::vec2 &b)
                { return a + b; },
                sol::meta_function::multiplication, [](const glm::vec2 &a, const glm::vec2 &b)
                { return a * b; },
                sol::meta_function::subtraction, [](const glm::vec2 &a, const glm::vec2 &b)
                { return a - b; },
                sol::meta_function::division, [](const glm::vec2 &a, const glm::vec2 &b)
                { return a / b; },
                sol::meta_function::equal_to, [](const glm::vec2 &a, const glm::vec2 &b)
                { return a == b; },
                "Length", [](const glm::vec2 &v)
                { return glm::length(v); },
                "Distance", [](const glm::vec2 &a, const glm::vec2 &b)
                { return glm::distance(a, b); },
                "Distance2", [](const glm::vec2 &a, const glm::vec2 &b)
                { return glm::distance2(a, b); });

        auto mult_overloads = sol::overload(
                [](const glm::vec3 &v1, const glm::vec3 &v2)->glm::vec3
                { return v1 * v2; },
                [](const glm::vec3 &v1, float f)->glm::vec3
                { return v1 * f; },
                [](float f, const glm::vec3 &v1)->glm::vec3
                { return f * v1; });

        state.new_usertype<glm::vec3>(
                "Vector3",
                sol::constructors<sol::types<>, sol::types<float, float, float>>(),
                "x", &glm::vec3::x,
                "y", &glm::vec3::y,
                "z", &glm::vec3::z,
                sol::meta_function::addition, [](const glm::vec3 &a, const glm::vec3 &b)
                { return a + b; },
                sol::meta_function::multiplication, mult_overloads,
                sol::meta_function::subtraction, [](const glm::vec3 &a, const glm::vec3 &b)
                { return a - b; },
                sol::meta_function::unary_minus, [](glm::vec3 &v)->glm::vec3
                { return -v; },
                sol::meta_function::division, [](const glm::vec3 &a, const glm::vec3 &b)
                { return a / b; },
                sol::meta_function::equal_to, [](const glm::vec3 &a, const glm::vec3 &b)
                { return a == b; },
                "Normalise", [](glm::vec3 &v)
                { return glm::normalize(v); },
                "Length", [](const glm::vec3 &v)
                { return glm::length(v); },
                "Distance", [](const glm::vec3 &a, const glm::vec3 &b)
                { return glm::distance(a, b); },
                "Distance2", [](const glm::vec3 &a, const glm::vec3 &b)
                { return glm::distance2(a, b); });

        state.new_usertype<glm::vec4>(
                "Vector4",
                sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>(),
                "x", &glm::vec4::x,
                "y", &glm::vec4::y,
                "z", &glm::vec4::z,
                "w", &glm::vec4::w,
                sol::meta_function::addition, [](const glm::vec4 &a, const glm::vec4 &b)
                { return a + b; },
                sol::meta_function::multiplication, [](const glm::vec4 &a, const glm::vec4 &b)
                { return a * b; },
                sol::meta_function::multiplication,
                sol::overload([](const glm::vec4 &v1, const glm::vec4 &v2)->glm::vec4
                              { return v1 * v2; },
                              [](const glm::vec4 &v1, float f)->glm::vec4
                              { return v1 * f; },
                              [](float f, const glm::vec4 &v1)->glm::vec4
                              { return f * v1; }),
                sol::meta_function::multiplication, [](float a, const glm::vec4 &b)
                { return a * b; },
                sol::meta_function::subtraction, [](const glm::vec4 &a, const glm::vec4 &b)
                { return a - b; },
                sol::meta_function::division, [](const glm::vec4 &a, const glm::vec4 &b)
                { return a / b; },
                sol::meta_function::equal_to, [](const glm::vec4 &a, const glm::vec4 &b)
                { return a == b; },
                "Normalise", [](glm::vec4 &v)
                { return glm::normalize(v); },
                "Length", [](const glm::vec4 &v)
                { return glm::length(v); },
                "Distance", [](const glm::vec4 &a, const glm::vec4 &b)
                { return glm::distance(a, b); },
                "Distance2", [](const glm::vec4 &a, const glm::vec4 &b)
                { return glm::distance2(a, b); });

        state.new_usertype<glm::quat>(
                "Quaternion",
                sol::constructors<glm::quat(float, float, float, float), glm::quat(glm::vec3)>(),
                "x", &glm::quat::x,
                "y", &glm::quat::y,
                "z", &glm::quat::z,
                "w", &glm::quat::w,
                sol::meta_function::addition, [](const glm::quat &a, const glm::quat &b)
                { return a + b; },
                sol::meta_function::multiplication, [](const glm::quat &a, const glm::quat &b)
                { return a * b; },
                sol::meta_function::subtraction, [](const glm::quat &a, const glm::quat &b)
                { return a - b; },
                sol::meta_function::equal_to, [](const glm::quat &a, const glm::quat &b)
                { return a == b; },
                "Normalise", [](glm::quat &q)
                { return glm::normalize(q); });

        state.new_usertype<glm::mat3>("Matrix3",
                                      sol::constructors<glm::mat3(float, float, float, float, float, float, float,
                                                                  float, float), glm::mat3()>(),
                                      sol::meta_function::multiplication, [](const glm::mat3 &a, const glm::mat3 &b)
                                      { return a * b; });

        state.new_usertype<glm::mat4>(
                "Matrix4",
                sol::constructors<glm::mat4(float), glm::mat4()>(),
                sol::meta_function::multiplication, [](const glm::mat4 &a, const glm::mat4 &b)
                { return a * b; },
                sol::meta_function::addition, [](const glm::mat4 &a, const glm::mat4 &b)
                { return a + b; },
                sol::meta_function::subtraction, [](const glm::mat4 &a, const glm::mat4 &b)
                { return a - b; });

        state.new_usertype<Transform>("Transform",
                                             sol::constructors<Transform(glm::mat4), Transform(), Transform(glm::vec3)>(),
                                             "LocalScale", &Transform::GetLocalScale,
                                             "LocalOrientation", &Transform::GetLocalOrientation,
                                             "LocalPosition", &Transform::GetLocalPosition,
                                             "ApplyTransform", &Transform::ApplyTransform,
                                             "UpdateMatrices", &Transform::UpdateMatrices,
                                             "SetLocalTransform", &Transform::SetLocalTransform,
                                             "SetLocalPosition", &Transform::SetLocalPosition,
                                             "SetLocalScale", &Transform::SetLocalScale,
                                             "SetLocalOrientation", &Transform::SetLocalOrientation,
                                             "GetWorldPosition", &Transform::GetWorldPosition,
                                             "GetWorldOrientation", &Transform::GetWorldOrientation,
                                             "GetForwardDirection", &Transform::GetForwardDirection,
                                             "GetRightDirection", &Transform::GetRightDirection);
    }
} // NekoEngine