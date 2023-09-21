#pragma once

#include "Core.h"
#include "Maths.h"
#include "MeshFactory.h"

namespace NekoEngine
{
    Mesh* MeshFactory::CreatePrimative(PrimitiveType type)
    {
        switch(type)
        {
            case PrimitiveType::Cube:
                return CreateCube();
            case PrimitiveType::Plane:
                return CreatePlane(1.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            case PrimitiveType::Quad:
                return CreateQuad();
            case PrimitiveType::Sphere:
                return CreateSphere();
            case PrimitiveType::Pyramid:
                return CreatePyramid();
            case PrimitiveType::Capsule:
                return CreateCapsule();
            case PrimitiveType::Cylinder:
                return CreateCylinder();
            case PrimitiveType::Terrain:
                // return CreateTerrain(); //TODO: Create Terrain
            default:
            LOG("Trying to create primitive of unknown type");

                return nullptr;
        }
    }

    Mesh* MeshFactory::CreatePlane(float width, float height, const glm::vec3 &normal)
    {
        glm::vec3 vec = normal * 90.0f;
        glm::quat rotation =
                glm::quat(vec.z, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::quat(vec.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::quat(vec.x, glm::vec3(0.0f, 0.0f, 1.0f));

        std::vector<Vertex> data(4);

        data[0].position = rotation * glm::vec3(-width * 0.5f, -1.0f, -height * 0.5f);
        data[0].normal = normal;
        data[0].texCoords = glm::vec2(0.0f, 0.0f);

        data[1].position = rotation * glm::vec3(-width * 0.5f, -1.0f, height * 0.5f);
        data[1].normal = normal;
        data[1].texCoords = glm::vec2(0.0f, 1.0f);

        data[2].position = rotation * glm::vec3(width * 0.5f, 1.0f, height * 0.5f);
        data[2].normal = normal;
        data[2].texCoords = glm::vec2(1.0f, 1.0f);

        data[3].position = rotation * glm::vec3(width * 0.5f, 1.0f, -height * 0.5f);
        data[3].normal = normal;
        data[3].texCoords = glm::vec2(1.0f, 0.0f);

        std::vector<uint32_t> indices = {
                0, 1, 2,
                2, 3, 0
        };
        return new Mesh(indices, data);
    }

    Mesh* MeshFactory::CreateCube()
    {
        //    v6----- v5
        //   /|      /|
        //  v1------v0|
        //  | |     | |
        //  | |v7---|-|v4
        //  |/      |/
        //  v2------v3
        std::vector<Vertex> data(24);

        data[0].position = glm::vec3(0.5f, 0.5f, 0.5f);
        data[0].color = glm::vec4(0.0f);
        data[0].normal = glm::vec3(0.0f, 0.0f, 1.0f);

        data[1].position = glm::vec3(-0.5f, 0.5f, 0.5f);
        data[1].color = glm::vec4(0.0f);
        data[1].normal = glm::vec3(0.0f, 0.0f, 1.0f);

        data[2].position = glm::vec3(-0.5f, -0.5f, 0.5f);
        data[2].color = glm::vec4(0.0f);
        data[2].normal = glm::vec3(0.0f, 0.0f, 1.0f);

        data[3].position = glm::vec3(0.5f, -0.5f, 0.5f);
        data[3].color = glm::vec4(0.0f);
        data[3].normal = glm::vec3(0.0f, 0.0f, 1.0f);

        data[4].position = glm::vec3(0.5f, 0.5f, 0.5f);
        data[4].color = glm::vec4(0.0f);
        data[4].normal = glm::vec3(1.0f, 0.0f, 0.0f);

        data[5].position = glm::vec3(0.5f, -0.5f, 0.5f);
        data[5].color = glm::vec4(0.0f);
        data[5].normal = glm::vec3(1.0f, 0.0f, 0.0f);

        data[6].position = glm::vec3(0.5f, -0.5f, -0.5f);
        data[6].color = glm::vec4(0.0f);
        data[6].normal = glm::vec3(1.0f, 0.0f, 0.0f);

        data[7].position = glm::vec3(0.5f, 0.5f, -0.5f);
        data[7].color = glm::vec4(0.0f);
        data[7].normal = glm::vec3(1.0f, 0.0f, 0.0f);

        data[8].position = glm::vec3(0.5f, 0.5f, 0.5f);
        data[8].color = glm::vec4(0.0f);
        data[8].normal = glm::vec3(0.0f, 1.0f, 0.0f);

        data[9].position = glm::vec3(0.5f, 0.5f, -0.5f);
        data[9].color = glm::vec4(0.0f);
        data[9].normal = glm::vec3(0.0f, 1.0f, 0.0f);

        data[10].position = glm::vec3(-0.5f, 0.5f, -0.5f);
        data[10].color = glm::vec4(0.0f);
        data[10].normal = glm::vec3(0.0f, 1.0f, 0.0f);

        data[11].position = glm::vec3(-0.5f, 0.5f, 0.5f);
        data[11].color = glm::vec4(0.0f);
        data[11].normal = glm::vec3(0.0f, 1.0f, 0.0f);

        data[12].position = glm::vec3(-0.5f, 0.5f, 0.5f);
        data[12].color = glm::vec4(0.0f);
        data[12].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

        data[13].position = glm::vec3(-0.5f, 0.5f, -0.5f);
        data[13].color = glm::vec4(0.0f);
        data[13].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

        data[14].position = glm::vec3(-0.5f, -0.5f, -0.5f);
        data[14].color = glm::vec4(0.0f);
        data[14].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

        data[15].position = glm::vec3(-0.5f, -0.5f, 0.5f);
        data[15].color = glm::vec4(0.0f);
        data[15].normal = glm::vec3(-1.0f, 0.0f, 0.0f);

        data[16].position = glm::vec3(-0.5f, -0.5f, -0.5f);
        data[16].color = glm::vec4(0.0f);
        data[16].normal = glm::vec3(0.0f, -1.0f, 0.0f);

        data[17].position = glm::vec3(0.5f, -0.5f, -0.5f);
        data[17].color = glm::vec4(0.0f);
        data[17].normal = glm::vec3(0.0f, -1.0f, 0.0f);

        data[18].position = glm::vec3(0.5f, -0.5f, 0.5f);
        data[18].color = glm::vec4(0.0f);
        data[18].normal = glm::vec3(0.0f, -1.0f, 0.0f);

        data[19].position = glm::vec3(-0.5f, -0.5f, 0.5f);
        data[19].color = glm::vec4(0.0f);
        data[19].normal = glm::vec3(0.0f, -1.0f, 0.0f);

        data[20].position = glm::vec3(0.5f, -0.5f, -0.5f);
        data[20].color = glm::vec4(0.0f);
        data[20].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        data[21].position = glm::vec3(-0.5f, -0.5f, -0.5f);
        data[21].color = glm::vec4(0.0f);
        data[21].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        data[22].position = glm::vec3(-0.5f, 0.5f, -0.5f);
        data[22].color = glm::vec4(0.0f);
        data[22].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        data[23].position = glm::vec3(0.5f, 0.5f, -0.5f);
        data[23].color = glm::vec4(0.0f);
        data[23].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        for(int i = 0; i < 6; i++)
        {
            data[i * 4 + 0].texCoords = glm::vec2(0.0f, 0.0f);
            data[i * 4 + 1].texCoords = glm::vec2(1.0f, 0.0f);
            data[i * 4 + 2].texCoords = glm::vec2(1.0f, 1.0f);
            data[i * 4 + 3].texCoords = glm::vec2(0.0f, 1.0f);
        }

        std::vector<uint32_t> indices = {
                0, 1, 2,
                0, 2, 3,
                4, 5, 6,
                4, 6, 7,
                8, 9, 10,
                8, 10, 11,
                12, 13, 14,
                12, 14, 15,
                16, 17, 18,
                16, 18, 19,
                20, 21, 22,
                20, 22, 23
        };

        return new Mesh(indices, data);
    }

    Mesh* MeshFactory::CreateQuad()
    {
        std::vector<Vertex> data(4);

        data[0].position = glm::vec3(-1.0f, -1.0f, 0.0f);
        data[0].texCoords = glm::vec2(0.0f, 0.0f);
        data[0].color = glm::vec4(0.0f);

        data[1].position = glm::vec3(1.0f, -1.0f, 0.0f);
        data[1].color = glm::vec4(0.0f);
        data[1].texCoords = glm::vec2(1.0f, 0.0f);

        data[2].position = glm::vec3(1.0f, 1.0f, 0.0f);
        data[2].color = glm::vec4(0.0f);
        data[2].texCoords = glm::vec2(1.0f, 1.0f);

        data[3].position = glm::vec3(-1.0f, 1.0f, 0.0f);
        data[3].color = glm::vec4(0.0f);
        data[3].texCoords = glm::vec2(0.0f, 1.0f);

        std::vector<uint32_t> indices = {
                0,
                1,
                2,
                2,
                3,
                0,
        };

        return new Mesh(indices, data);
    }

    Mesh* MeshFactory::CreateSphere(uint32_t xSegments, uint32_t ySegments)
    {
        auto data = std::vector<Vertex>();

        float sectorCount = static_cast<float>(xSegments);
        float stackCount = static_cast<float>(ySegments);
        float sectorStep = 2 * Maths::M_PI / sectorCount;
        float stackStep = Maths::M_PI / stackCount;
        float radius = 0.5f;

        for(int i = 0; i <= stackCount; ++i)
        {
            float stackAngle = Maths::M_PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
            float xy = radius * cos(stackAngle);        // r * cos(u)
            float z = radius * sin(stackAngle);        // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but different tex coords
            for(int j = 0; j <= sectorCount; ++j)
            {
                float sectorAngle = j * sectorStep; // starting from 0 to 2pi

                // vertex position (x, y, z)
                float x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
                float y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)

                // vertex tex coord (s, t) range between [0, 1]
                float s = static_cast<float>(j / sectorCount);
                float t = static_cast<float>(i / stackCount);

                Vertex vertex;
                vertex.position = glm::vec3(x, y, z);
                vertex.texCoords = glm::vec2(s, t);
                vertex.normal = glm::normalize(glm::vec3(x, y, z));

                data.emplace_back(vertex);
            }
        }

        std::vector<uint32_t> indices;
        uint32_t k1, k2;
        for(uint32_t i = 0; i < stackCount; ++i)
        {
            k1 = i * (static_cast<uint32_t>(sectorCount) + 1U); // beginning of current stack
            k2 = k1 + static_cast<uint32_t>(sectorCount) + 1U;  // beginning of next stack

            for(uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
            {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                if(i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                // k1+1 => k2 => k2+1
                if(i != (stackCount - 1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }

        return new Mesh(indices, data);
    }

    Mesh* MeshFactory::CreateCapsule(float radius, float midHeight, int radialSegments, int rings)
    {
        int i, j, prevrow, thisrow, point;
        float x, y, z, u, v, w;
        float onethird = 1.0f / 3.0f;
        float twothirds = 2.0f / 3.0f;

        std::vector<Vertex> data;
        std::vector<uint32_t> indices;

        point = 0;

        /* top hemisphere */
        thisrow = 0;
        prevrow = 0;
        for(j = 0; j <= (rings + 1); j++)
        {
            v = float(j);

            v /= (rings + 1);
            w = sin(0.5f * Maths::M_PI * v);
            y = radius * cos(0.5f * Maths::M_PI * v);

            for(i = 0; i <= radialSegments; i++)
            {
                u = float(i);
                u /= radialSegments;

                x = sin(u * (Maths::M_PI * 2.0f));
                z = cos(u * (Maths::M_PI * 2.0f));

                glm::vec3 p = glm::vec3(x * radius * w, y, z * radius * w);

                Vertex vertex;
                vertex.position = p + glm::vec3(0.0f, 0.5f * midHeight, 0.0f);
                vertex.normal = glm::normalize(p);
                vertex.texCoords = glm::vec2(u, onethird * v);
                data.emplace_back(vertex);
                point++;

                if(i > 0 && j > 0)
                {
                    indices.push_back(thisrow + i - 1);
                    indices.push_back(prevrow + i);
                    indices.push_back(prevrow + i - 1);

                    indices.push_back(thisrow + i - 1);
                    indices.push_back(thisrow + i);
                    indices.push_back(prevrow + i);
                };
            };

            prevrow = thisrow;
            thisrow = point;
        };

        /* cylinder */
        thisrow = point;
        prevrow = 0;
        for(j = 0; j <= (rings + 1); j++)
        {
            v = float(j);
            v /= (rings + 1);

            y = midHeight * v;
            y = (midHeight * 0.5f) - y;

            for(i = 0; i <= radialSegments; i++)
            {
                u = float(i);
                u /= radialSegments;

                x = sin(u * (Maths::M_PI * 2.0f));
                z = cos(u * (Maths::M_PI * 2.0f));

                glm::vec3 p = glm::vec3(x * radius, y, z * radius);

                Vertex vertex;
                vertex.position = p;
                // vertex.normal = glm::vec3(x, z, 0.0f);
                vertex.normal = glm::vec3(x, 0.0f, z);
                // vertex.texCoords = glm::vec2(u, onethird + (v * onethird));
                vertex.texCoords = glm::vec2(u, v * 0.5f);
                data.emplace_back(vertex);

                point++;

                if(i > 0 && j > 0)
                {
                    indices.push_back(thisrow + i - 1);
                    indices.push_back(prevrow + i);
                    indices.push_back(prevrow + i - 1);

                    indices.push_back(thisrow + i - 1);
                    indices.push_back(thisrow + i);
                    indices.push_back(prevrow + i);
                };
            };

            prevrow = thisrow;
            thisrow = point;
        };

        /* bottom hemisphere */
        thisrow = point;
        prevrow = 0;

        for(j = 0; j <= (rings + 1); j++)
        {
            v = float(j);

            v /= (rings + 1);
            v += 1.0f;
            w = sin(0.5f * Maths::M_PI * v);
            y = radius * cos(0.5f * Maths::M_PI * v);

            for(i = 0; i <= radialSegments; i++)
            {
                float u2 = float(i);
                u2 /= radialSegments;

                x = sin(u2 * (Maths::M_PI * 2.0f));
                z = cos(u2 * (Maths::M_PI * 2.0f));

                glm::vec3 p = glm::vec3(x * radius * w, y, z * radius * w);

                Vertex vertex;
                vertex.position = p + glm::vec3(0.0f, -0.5f * midHeight, 0.0f);
                vertex.normal = glm::normalize(p);
                vertex.texCoords = glm::vec2(u2, twothirds + ((v - 1.0f) * onethird));
                data.emplace_back(vertex);

                point++;

                if(i > 0 && j > 0)
                {
                    indices.push_back(thisrow + i - 1);
                    indices.push_back(prevrow + i);
                    indices.push_back(prevrow + i - 1);

                    indices.push_back(thisrow + i - 1);
                    indices.push_back(thisrow + i);
                    indices.push_back(prevrow + i);
                };
            };

            prevrow = thisrow;
            thisrow = point;
        }

        return new Mesh(indices, data);
    }

    Mesh* MeshFactory::CreatePyramid()
    {
        std::vector<Vertex> data(18);

        data[0].position = glm::vec3(1.0f, 1.0f, -1.0f);
        data[0].color = glm::vec4(0.0f);
        data[0].texCoords = glm::vec2(0.24f, 0.20f);
        data[0].normal = glm::vec3(0.0f, 0.8948f, 0.4464f);

        data[1].position = glm::vec3(-1.0f, 1.0f, -1.0f);
        data[1].color = glm::vec4(0.0f);
        data[1].texCoords = glm::vec2(0.24f, 0.81f);
        data[1].normal = glm::vec3(0.0f, 0.8948f, 0.4464f);

        data[2].position = glm::vec3(0.0f, 0.0f, 1.0f);
        data[2].color = glm::vec4(0.0f);
        data[2].texCoords = glm::vec2(0.95f, 0.50f);
        data[2].normal = glm::vec3(0.0f, 0.8948f, 0.4464f);

        data[3].position = glm::vec3(-1.0f, 1.0f, -1.0f);
        data[3].color = glm::vec4(0.0f);
        data[3].texCoords = glm::vec2(0.24f, 0.21f);
        data[3].normal = glm::vec3(-0.8948f, 0.0f, 0.4464f);

        data[4].position = glm::vec3(-1.0f, -1.0f, -1.0f);
        data[4].color = glm::vec4(0.0f);
        data[4].texCoords = glm::vec2(0.24f, 0.81f);
        data[4].normal = glm::vec3(-0.8948f, 0.0f, 0.4464f);

        data[5].position = glm::vec3(0.0f, 0.0f, 1.0f);
        data[5].color = glm::vec4(0.0f);
        data[5].texCoords = glm::vec2(0.95f, 0.50f);
        data[5].normal = glm::vec3(-0.8948f, 0.0f, 0.4464f);

        data[6].position = glm::vec3(1.0f, 1.0f, -1.0f);
        data[6].color = glm::vec4(0.0f);
        data[6].texCoords = glm::vec2(0.24f, 0.81f);
        data[6].normal = glm::vec3(0.8948f, 0.0f, 0.4475f);

        data[7].position = glm::vec3(0.0f, 0.0f, 1.0f);
        data[7].color = glm::vec4(0.0f);
        data[7].texCoords = glm::vec2(0.95f, 0.50f);
        data[7].normal = glm::vec3(0.8948f, 0.0f, 0.4475f);

        data[8].position = glm::vec3(1.0f, -1.0f, -1.0f);
        data[8].color = glm::vec4(0.0f);
        data[8].texCoords = glm::vec2(0.24f, 0.21f);
        data[8].normal = glm::vec3(0.8948f, 0.0f, 0.4475f);

        data[9].position = glm::vec3(-1.0f, -1.0f, -1.0f);
        data[9].color = glm::vec4(0.0f);
        data[9].texCoords = glm::vec2(0.24f, 0.21f);
        data[9].normal = glm::vec3(0.0f, -0.8948f, 0.448f);

        data[10].position = glm::vec3(1.0f, -1.0f, -1.0f);
        data[10].color = glm::vec4(0.0f);
        data[10].texCoords = glm::vec2(0.24f, 0.81f);
        data[10].normal = glm::vec3(0.0f, -0.8948f, 0.448f);

        data[11].position = glm::vec3(0.0f, 0.0f, 1.0f);
        data[11].color = glm::vec4(0.0f);
        data[11].texCoords = glm::vec2(0.95f, 0.50f);
        data[11].normal = glm::vec3(0.0f, -0.8948f, 0.448f);

        data[12].position = glm::vec3(-1.0f, 1.0f, -1.0f);
        data[12].color = glm::vec4(0.0f);
        data[12].texCoords = glm::vec2(0.0f, 0.0f);
        data[12].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        data[13].position = glm::vec3(1.0f, 1.0f, -1.0f);
        data[13].color = glm::vec4(0.0f);
        data[13].texCoords = glm::vec2(0.0f, 1.0f);
        data[13].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        data[14].position = glm::vec3(1.0f, -1.0f, -1.0f);
        data[14].color = glm::vec4(0.0f);
        data[14].texCoords = glm::vec2(1.0f, 1.0f);
        data[14].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        data[15].position = glm::vec3(-1.0f, -1.0f, -1.0f);
        data[15].color = glm::vec4(0.0f);
        data[15].texCoords = glm::vec2(0.96f, 0.0f);
        data[15].normal = glm::vec3(0.0f, 0.0f, -1.0f);

        data[16].position = glm::vec3(0.0f, 0.0f, 0.0f);
        data[16].color = glm::vec4(0.0f);
        data[16].texCoords = glm::vec2(0.0f, 0.0f);
        data[16].normal = glm::vec3(0.0f, 0.0f, 0.0f);

        data[17].position = glm::vec3(0.0f, 0.0f, 0.0f);
        data[17].color = glm::vec4(0.0f);
        data[17].texCoords = glm::vec2(0.0f, 0.0f);
        data[17].normal = glm::vec3(0.0f, 0.0f, 0.0f);

        std::vector<uint32_t> indices = {
                0, 1, 2,
                3, 4, 5,
                6, 7, 8,
                9, 10, 11,
                12, 13, 14,
                15, 12, 14
        };

        return new Mesh(indices, data);
    }

    Mesh* MeshFactory::CreateCylinder(float bottomRadius, float topRadius, float height, int radialSegments, int rings)
    {
        int i, j, prevrow, thisrow, point = 0;
        float x, y, z, u, v, radius;

        std::vector<Vertex> data;
        std::vector<uint32_t> indices;

        thisrow = 0;
        prevrow = 0;
        for(j = 0; j <= (rings + 1); j++)
        {
            v = float(j);
            v /= (rings + 1);

            radius = topRadius + ((bottomRadius - topRadius) * v);

            y = height * v;
            y = (height * 0.5f) - y;

            for(i = 0; i <= radialSegments; i++)
            {
                u = float(i);
                u /= radialSegments;

                x = sin(u * (Maths::M_PI * 2.0f));
                z = cos(u * (Maths::M_PI * 2.0f));

                glm::vec3 p = glm::vec3(x * radius, y, z * radius);

                Vertex vertex;
                vertex.position = p;
                vertex.normal = glm::vec3(x, 0.0f, z);
                vertex.texCoords = glm::vec2(u, v * 0.5f);
                data.emplace_back(vertex);

                point++;

                if(i > 0 && j > 0)
                {
                    indices.push_back(thisrow + i - 1);
                    indices.push_back(prevrow + i);
                    indices.push_back(prevrow + i - 1);

                    indices.push_back(thisrow + i - 1);
                    indices.push_back(thisrow + i);
                    indices.push_back(prevrow + i);
                };
            };

            prevrow = thisrow;
            thisrow = point;
        };

        // add top
        if(topRadius > 0.0f)
        {
            y = height * 0.5f;

            Vertex vertex;
            vertex.position = glm::vec3(0.0f, y, 0.0f);
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.texCoords = glm::vec2(0.25f, 0.75f);
            data.emplace_back(vertex);
            point++;

            for(i = 0; i <= radialSegments; i++)
            {
                float r = float(i);
                r /= radialSegments;

                x = sin(r * (Maths::M_PI * 2.0f));
                z = cos(r * (Maths::M_PI * 2.0f));

                u = ((x + 1.0f) * 0.25f);
                v = 0.5f + ((z + 1.0f) * 0.25f);

                glm::vec3 p = glm::vec3(x * topRadius, y, z * topRadius);
                Vertex vertex;
                vertex.position = p;
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertex.texCoords = glm::vec2(u, v);
                data.emplace_back(vertex);
                point++;

                if(i > 0)
                {
                    indices.push_back(point - 2);
                    indices.push_back(point - 1);
                    indices.push_back(thisrow);
                };
            };
        };

        // add bottom
        if(bottomRadius > 0.0f)
        {
            y = height * -0.5f;

            thisrow = point;

            Vertex vertex;
            vertex.position = glm::vec3(0.0f, y, 0.0f);
            vertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
            vertex.texCoords = glm::vec2(0.75f, 0.75f);
            data.emplace_back(vertex);
            point++;

            for(i = 0; i <= radialSegments; i++)
            {
                float r = float(i);
                r /= radialSegments;

                x = sin(r * (Maths::M_PI * 2.0f));
                z = cos(r * (Maths::M_PI * 2.0f));

                u = 0.5f + ((x + 1.0f) * 0.25f);
                v = 1.0f - ((z + 1.0f) * 0.25f);

                glm::vec3 p = glm::vec3(x * bottomRadius, y, z * bottomRadius);

                vertex.position = p;
                vertex.normal = glm::vec3(0.0f, -1.0f, 0.0f);
                vertex.texCoords = glm::vec2(u, v);
                data.emplace_back(vertex);
                point++;

                if(i > 0)
                {
                    indices.push_back(point - 1);
                    indices.push_back(point - 2);
                    indices.push_back(thisrow);
                };
            };
        };

        return new Mesh(indices, data);
    }
}