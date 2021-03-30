#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include "tgaimage.h"
#include "vertex.h"

namespace MSRender{
    class Model {
    private:
        std::vector<Point3d> vertices;
        std::vector<Point2d> uvs;
        std::vector<Vector3d> normals;
        std::vector<int> face_vertices;
        std::vector<int> face_uvs;
        std::vector<int> face_normal;
        TGAImage diffusemap_;         // diffuse color texture
        TGAImage normalmap_;          // normal map texture
        TGAImage specularmap_;        // specular map texture
        void load_texture(const std::string filename, const std::string suffix, TGAImage &img);
    public:
        Model() {}
        Model(const std::string filename);
        int vertexs_size() const;
        int faces_size() const;
        Vector3d get_normal(const int iface, const int nthvert) const;  // per triangle corner normal vertex
        Vector3d get_normal(const Point2d &uv) const;                      // fetch the normal vector from the normal map texture
        // Point3d get_vertex(const int i) const;
        Point3d get_vertex(const int iface, const int nthvert) const;
        Point2d get_uv(const int iface, const int nthvert) const;
        TGAColor get_diffuse(const Point2d &uv) const;
        double get_specular(const Point2d &uv) const;
        const TGAImage& get_diffusemap() const;
        const TGAImage& get_normalmap() const;
        const TGAImage& get_specularmap() const;
    };
}

#endif