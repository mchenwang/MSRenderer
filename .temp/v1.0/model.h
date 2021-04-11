#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include "tgaimage.h"
#include "vertex.h"
#include <Eigen/Dense>

namespace MSRender{
    class Model {
    private:
        std::vector<Point3d> vertices;
        std::vector<Point2d> uvs;
        std::vector<Vector3d> normals;
        std::vector<int> face_vertices;
        std::vector<int> face_uvs;
        std::vector<int> face_normal;
        bool has_diffusemap;
        TGAImage diffusemap_;         // diffuse color texture
        bool has_normalmap;
        TGAImage normalmap_;          // normal map texture
        bool has_specularmap;
        TGAImage specularmap_;        // specular map texture
        bool load_texture(const std::string filename, const std::string suffix, TGAImage &img);
    public:
        static bool nm_is_in_tangent;
        Model() {}
        Model(const std::string filename);
        size_t vertexs_size() const;
        size_t faces_size() const;
        Vector3d get_normal(const size_t iface, const size_t nthvert) const;  // per triangle corner normal vertex
        Point3d get_vertex(const size_t iface, const size_t nthvert) const;
        Point2d get_uv(const size_t iface, const size_t nthvert) const;
        TGAColor get_diffuse(const Point2d &uv) const;
        double get_specular(const Point2d &uv) const;
        TGAColor get_diffuse(const double uv0, const double uv1) const;
        void set_diffuse(const double uv0, const double uv1) ;
        double get_specular(const double uv0, const double uv1) const;
        Vector3d get_normal_with_map(const Point2d &uv) const; // fetch the normal vector from the normal map texture
        Vector3d get_normal_with_map(const double uv0, const double uv1) const;

        const TGAImage& get_diffusemap() const;
        const TGAImage& get_normalmap() const;
        const TGAImage& get_specularmap() const;

        bool has_diffuse_map() const { return has_diffusemap; }
        bool has_normal_map() const { return has_normalmap; }
        bool has_specular_map() const { return has_specularmap; }
    };
}

#endif