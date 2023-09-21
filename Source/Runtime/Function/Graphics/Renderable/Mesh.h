#pragma once
#include "RHI/IndexBuffer.h"
#include "RHI/VertexBuffer.h"
#include "Material.h"
#include "Math/BoundingBox.h"

namespace NekoEngine
{
    struct VertexBase
    {
        FVector3 position = FVector3(0.0f);
        FVector2 texCoords = FVector2(0.0f);

        bool operator==(const VertexBase &rhs) const
        {
            return position == rhs.position &&
                   texCoords == rhs.texCoords;
        }

    };

    struct Vertex : public VertexBase
    {
        Color color = Color(0.0f);
        FVector3 normal = FVector3(0.0f);
        FVector3 tangent = FVector3(0.0f);
        FVector3 biTangent = FVector3(0.0f);

        bool operator==(const Vertex& rhs) const
        {
            return position == rhs.position && texCoords == rhs.texCoords && color == rhs.color && normal == rhs.normal && tangent == rhs.tangent && biTangent == rhs.biTangent;
        }
    };

    //TODO add bounding box
    class Mesh
    {
    protected:
        String name;
        bool isActive = true;
        ArrayList<uint32_t> indices;
        ArrayList<Vertex> vertices;
        SharedPtr<VertexBuffer> vertexBuffer;
        SharedPtr<IndexBuffer> indexBuffer;
        SharedPtr<Material> material;
        SharedPtr<BoundingBox> boundingBox;
    public:
        Mesh();
        Mesh(const Mesh& mesh);
        Mesh(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices, float optimiseThreshold = 0.95f);

        virtual ~Mesh() = default;

        void SetName(const String& _name) { name = _name; }
        void SetMaterial(const SharedPtr<Material>& _material) { material = _material;}

        const SharedPtr<VertexBuffer>& GetVertexBuffer() const { return vertexBuffer; }
        const SharedPtr<IndexBuffer>& GetIndexBuffer() const { return indexBuffer; }
        const SharedPtr<Material>& GetMaterial() const { return material; }
        const SharedPtr<BoundingBox>& GetBoundingBox() const { return boundingBox; }

        bool IsActive() { return isActive; }
        const std::string& GetName() const { return name; }

        static glm::vec3 GenerateTangent(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec2& ta, const glm::vec2& tb, const glm::vec2& tc);
        static glm::vec3* GenerateNormals(uint32_t numVertices, glm::vec3* vertices, uint32_t* indices, uint32_t numIndices);
        static glm::vec3* GenerateTangents(uint32_t numVertices, glm::vec3* vertices, uint32_t* indices, uint32_t numIndices, glm::vec2* texCoords);
        static void GenerateNormals(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);
        static void GenerateTangents(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t numIndices);
        static void GenerateTangentsAndBitangents(Vertex* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);
    };

    enum class PrimitiveType : int
    {
        Plane    = 0,
        Quad     = 1,
        Cube     = 2,
        Pyramid  = 3,
        Sphere   = 4,
        Capsule  = 5,
        Cylinder = 6,
        Terrain  = 7,
        File     = 8,
        None     = 9
    };

} // NekoEngine