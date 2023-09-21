#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "vector"

namespace NekoEngine
{

    struct HullVertex
    {
        int idx = 0;
        glm::vec3 pos;
        std::vector<int> enclosing_edges;
        std::vector<int> enclosing_faces;
    };

    struct HullEdge
    {
        int idx = 0;
        int vStart = 0, vEnd = 0;
        std::vector<int> adjoining_edge_ids;
        std::vector<int> enclosing_faces;
    };

    struct HullFace
    {
        int idx = 0;
        glm::vec3 normal;
        std::vector<int> vert_ids;
        std::vector<int> edge_ids;
        std::vector<int> adjoining_face_ids;
    };

    class Hull
    {
    protected:
        std::vector<HullVertex> m_Vertices;
        std::vector<HullEdge> m_Edges;
        std::vector<HullFace> m_Faces;

        int ConstructNewEdge(int parent_face_idx, int vert_start, int vert_end); // Called by AddFace
    public:
        Hull();

        ~Hull();

        void AddVertex(const glm::vec3 &v);

        void AddFace(const glm::vec3 &normal, int nVerts, const int* verts);

        void AddFace(const glm::vec3 &normal, const std::vector<int> &vert_ids)
        { AddFace(normal, static_cast<int>(vert_ids.size()), &vert_ids[0]); }

        int FindEdge(int v0_idx, int v1_idx);

        const HullVertex &GetVertex(int idx)
        { return m_Vertices[idx]; }

        const HullEdge &GetEdge(int idx)
        { return m_Edges[idx]; }

        const HullFace &GetFace(int idx)
        { return m_Faces[idx]; }

        size_t GetNumVertices() const
        { return m_Vertices.size(); }

        size_t GetNumEdges() const
        { return m_Edges.size(); }

        size_t GetNumFaces() const
        { return m_Faces.size(); }

        void GetMinMaxVerticesInAxis(const glm::vec3 &local_axis, int* out_min_vert, int* out_max_vert);

        void DebugDraw(const glm::mat4 &transform);
    };

} // NekoEngine


