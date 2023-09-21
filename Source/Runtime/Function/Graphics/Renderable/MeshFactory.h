#pragma once
#include "Core.h"
#include "Mesh.h"
namespace NekoEngine
{
    namespace MeshFactory
    {
        Mesh* CreatePrimative(PrimitiveType type);
        Mesh* CreateQuad();
        Mesh* CreateQuad(float x, float y, float width, float height);
        Mesh* CreateQuad(const FVector2& position, const FVector2& size);
        Mesh* CreateCube();
        Mesh* CreatePyramid();
        Mesh* CreateSphere(uint32_t xSegments = 64, uint32_t ySegments = 64);
        Mesh* CreateCapsule(float radius = 0.5f, float midHeight = 2.0f, int radialSegments = 64, int rings = 8);
        Mesh* CreatePlane(float width, float height, const FVector3& normal);
        Mesh* CreateCylinder(float bottomRadius = 0.5f, float topRadius = 0.5f, float height = 1.0f, int radialSegments = 64, int rings = 8);
        Mesh* CreateTerrain();
    }
}