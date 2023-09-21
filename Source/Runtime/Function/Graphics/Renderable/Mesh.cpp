#include "Mesh.h"
#include "meshoptimizer.h"
#include "Engine.h"
namespace NekoEngine
{
    Mesh::Mesh()
            : indices()
            , vertices()
            , vertexBuffer(nullptr)
            , indexBuffer(nullptr)
            , boundingBox(nullptr)
    {
    }

    Mesh::Mesh(const Mesh& mesh)
            : name(mesh.name)
            , indices(mesh.indices)
            , vertices(mesh.vertices)
            , vertexBuffer(mesh.vertexBuffer)
            , indexBuffer(mesh.indexBuffer)
            , material(mesh.material)
            , boundingBox(mesh.boundingBox)
    {
    }

//    Mesh::Mesh(SharedPtr<VertexBuffer>& vertexBuffer, SharedPtr<IndexBuffer>& indexBuffer, const SharedPtr<BoundingBox>& boundingBox)
//            : vertexBuffer(vertexBuffer)
//            , indexBuffer(indexBuffer)
//            , material(nullptr)
//            , boundingBox(boundingBox)
//    {
//    }

    Mesh::Mesh(const std::vector<uint32_t>& _indices, const std::vector<Vertex>& _vertices, float optimiseThreshold)
    {
        indices  = _indices;
        vertices = _vertices;


        size_t indexCount         = indices.size();
        size_t target_index_count = size_t(indices.size() * optimiseThreshold);

        float target_error = 1e-3f;
        float* resultError = nullptr;

        //TODO: Check
        auto newIndexCount = meshopt_simplify(indices.data(), indices.data(), indices.size(), (const float*)(&vertices[0]), vertices.size(), sizeof(Vertex), target_index_count, target_error, 0, resultError);

        auto newVertexCount = meshopt_optimizeVertexFetch( // return vertices (not vertex attribute values)
                (vertices.data()),
                (unsigned int*)(indices.data()),
                newIndexCount, // total new indices (not faces)
                (vertices.data()),
                (size_t)vertices.size(), // total vertices (not vertex attribute values)
                sizeof(Vertex)   // vertex stride
        );


        boundingBox = MakeShared<BoundingBox>();

        for(auto& vertex : vertices)
        {
            boundingBox->Merge(vertex.position);
        }

        indexBuffer = SharedPtr<IndexBuffer>(GET_RHI_FACTORY()->CreateIndexBuffer(indices.data(), (uint32_t)newIndexCount));

        vertexBuffer = SharedPtr<VertexBuffer>(GET_RHI_FACTORY()->CreateVertexBuffer(BufferUsage::STATIC));
        vertexBuffer->SetData((uint32_t)(sizeof(Vertex) * newVertexCount), vertices.data());
    }

    void Mesh::GenerateNormals(Vertex* _vertices, uint32_t vertexCount, uint32_t* _indices, uint32_t indexCount)
    {
        auto* normals = new FVector3[vertexCount];

        for(uint32_t i = 0; i < vertexCount; ++i)
        {
            normals[i] = FVector3();
        }

        if(_indices)
        {
            for(uint32_t i = 0; i < indexCount; i += 3)
            {
                const int a = _indices[i];
                const int b = _indices[i + 1];
                const int c = _indices[i + 2];

                const FVector3 _normal = glm::cross((_vertices[b].position - _vertices[a].position),
                                                    (_vertices[c].position - _vertices[a].position));

                normals[a] += _normal;
                normals[b] += _normal;
                normals[c] += _normal;
            }
        }
        else
        {
            // It's just a list of triangles, so generate face normals
            for(uint32_t i = 0; i < vertexCount; i += 3)
            {
                FVector3 &a = _vertices[i].position;
                FVector3 &b = _vertices[i + 1].position;
                FVector3 &c = _vertices[i + 2].position;

                const FVector3 _normal = glm::cross(b - a, c - a);

                normals[i] = _normal;
                normals[i + 1] = _normal;
                normals[i + 2] = _normal;
            }
        }

        for(uint32_t i = 0; i < vertexCount; ++i)
        {
            glm::normalize(normals[i]);
            _vertices[i].normal = normals[i];
        }

        delete[] normals;
    }

    void Mesh::GenerateTangents(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t numIndices)
    {
        FVector3* tangents = new FVector3[vertexCount];

        for(uint32_t i = 0; i < vertexCount; ++i)
        {
            tangents[i] = FVector3();
        }

        if(indices)
        {
            for(uint32_t i = 0; i < numIndices; i += 3)
            {
                int a = indices[i];
                int b = indices[i + 1];
                int c = indices[i + 2];

                const FVector3 tangent = GenerateTangent(vertices[a].position, vertices[b].position, vertices[c].position, vertices[a].texCoords, vertices[b].texCoords, vertices[c].texCoords);

                tangents[a] += tangent;
                tangents[b] += tangent;
                tangents[c] += tangent;
            }
        }
        else
        {
            for(uint32_t i = 0; i < vertexCount; i += 3)
            {
                const FVector3 tangent = GenerateTangent(vertices[i].position, vertices[i + 1].position, vertices[i + 2].position, vertices[i].texCoords, vertices[i + 1].texCoords,
                                                          vertices[i + 2].texCoords);

                tangents[i] += tangent;
                tangents[i + 1] += tangent;
                tangents[i + 2] += tangent;
            }
        }
        for(uint32_t i = 0; i < vertexCount; ++i)
        {
            glm::normalize(tangents[i]);
            vertices[i].tangent = tangents[i];
        }

        delete[] tangents;
    }


    FVector3 Mesh::GenerateTangent(const FVector3& a, const FVector3& b, const FVector3& c, const glm::vec2& ta, const glm::vec2& tb, const glm::vec2& tc)
    {
        const glm::vec2 coord1 = tb - ta;
        const glm::vec2 coord2 = tc - ta;

        const FVector3 vertex1 = b - a;
        const FVector3 vertex2 = c - a;

        const FVector3 axis = FVector3(vertex1 * coord2.y - vertex2 * coord1.y);

        const float factor = 1.0f / (coord1.x * coord2.y - coord2.x * coord1.y);

        return axis * factor;
    }

    FVector3* Mesh::GenerateNormals(uint32_t numVertices, FVector3* vertices, uint32_t* indices, uint32_t numIndices)
    {
        FVector3* normals = new FVector3[numVertices];

        for(uint32_t i = 0; i < numVertices; ++i)
        {
            normals[i] = FVector3();
        }

        if(indices)
        {
            for(uint32_t i = 0; i < numIndices; i += 3)
            {
                const int a = indices[i];
                const int b = indices[i + 1];
                const int c = indices[i + 2];

                const FVector3 _normal = glm::cross((vertices[b] - vertices[a]), (vertices[c] - vertices[a]));

                normals[a] += _normal;
                normals[b] += _normal;
                normals[c] += _normal;
            }
        }
        else
        {
            // It's just a list of triangles, so generate face normals
            for(uint32_t i = 0; i < numVertices; i += 3)
            {
                FVector3& a = vertices[i];
                FVector3& b = vertices[i + 1];
                FVector3& c = vertices[i + 2];

                const FVector3 _normal = glm::cross(b - a, c - a);

                normals[i]     = _normal;
                normals[i + 1] = _normal;
                normals[i + 2] = _normal;
            }
        }

        for(uint32_t i = 0; i < numVertices; ++i)
        {
            glm::normalize(normals[i]);
        }

        return normals;
    }

    FVector3* Mesh::GenerateTangents(uint32_t numVertices, FVector3* vertices, uint32_t* indices, uint32_t numIndices, glm::vec2* texCoords)
        {
            if(!texCoords)
            {
                return nullptr;
            }

            FVector3* tangents = new FVector3[numVertices];

            for(uint32_t i = 0; i < numVertices; ++i)
            {
                tangents[i] = FVector3();
            }

            if(indices)
            {
                for(uint32_t i = 0; i < numIndices; i += 3)
                {
                    int a = indices[i];
                    int b = indices[i + 1];
                    int c = indices[i + 2];

                    const FVector3 tangent = GenerateTangent(vertices[a], vertices[b], vertices[c], texCoords[a], texCoords[b], texCoords[c]);

                    tangents[a] += tangent;
                    tangents[b] += tangent;
                    tangents[c] += tangent;
                }
            }
            else
            {
                for(uint32_t i = 0; i < numVertices; i += 3)
                {
                    const FVector3 tangent = GenerateTangent(vertices[i], vertices[i + 1], vertices[i + 2], texCoords[i], texCoords[i + 1],
                                                              texCoords[i + 2]);

                    tangents[i] += tangent;
                    tangents[i + 1] += tangent;
                    tangents[i + 2] += tangent;
                }
            }
            for(uint32_t i = 0; i < numVertices; ++i)
            {
                glm::normalize(tangents[i]);
            }

            return tangents;
        }

    void Mesh::GenerateTangentsAndBitangents(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t numIndices)
    {
        for(uint32_t i = 0; i < vertexCount; i++)
        {
            vertices[i].tangent   = glm::vec3(0.0f);
            vertices[i].biTangent = glm::vec3(0.0f);
        }

        for(uint32_t i = 0; i < numIndices; i += 3)
        {
            glm::vec3 v0 = vertices[indices[i]].position;
            glm::vec3 v1 = vertices[indices[i + 1]].position;
            glm::vec3 v2 = vertices[indices[i + 2]].position;

            glm::vec2 uv0 = vertices[indices[i]].texCoords;
            glm::vec2 uv1 = vertices[indices[i + 1]].texCoords;
            glm::vec2 uv2 = vertices[indices[i + 2]].texCoords;

            glm::vec3 n0 = vertices[indices[i]].normal;
            glm::vec3 n1 = vertices[indices[i + 1]].normal;
            glm::vec3 n2 = vertices[indices[i + 2]].normal;

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;

            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            glm::vec3 tangent   = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
            glm::vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

            // Store tangent and bitangent for each vertex of the triangle
            vertices[indices[i]].tangent += tangent;
            vertices[indices[i + 1]].tangent += tangent;
            vertices[indices[i + 2]].tangent += tangent;

            vertices[indices[i]].biTangent += bitangent;
            vertices[indices[i + 1]].biTangent += bitangent;
            vertices[indices[i + 2]].biTangent += bitangent;
        }

        // Normalize the tangent and bitangent vectors
        for(uint32_t i = 0; i < vertexCount; i++)
        {
            vertices[i].tangent   = glm::normalize(vertices[i].tangent);
            vertices[i].biTangent = glm::normalize(vertices[i].biTangent);
        }
    }
} // NekoEngine