#pragma once
#include "Core.h"

namespace NekoEngine
{

    class Integration
    {
    public:
        struct State
        {
            glm::vec3 position;
            glm::vec3 velocity;
            glm::vec3 acceleration;
        };

        struct Derivative
        {
            glm::vec3 acceleration;
            glm::vec3 velocity;
        };

    public:
        static void RK2(State& state, float t, float dt);
        static void RK4(State& state, float t, float dt);

        static Derivative Evaluate(State& initial, float dt, float t, const Derivative& derivative);
    };

} // NekoEngine
