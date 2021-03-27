#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include "tgaimage.h"
#include "vertex.h"

class Model {
private:
    std::vector<Vertex3d> vertexs;
    std::vector<int> face;
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
    Vector3d get_normal(const Vector2d &uv) const;                      // fetch the normal vector from the normal map texture
    Vertex3d get_vertex(const int i) const;
    Vertex3d get_vertex(const int iface, const int nthvert) const;
    Point3d get_point(const int i) const;
    Point3d get_point(const int iface, const int nthvert) const;
    Point2d uv(const int iface, const int nthvert) const;
    TGAColor diffuse(const Vector2d &uv) const;
    double specular(const Vector2d &uv) const;
};

#endif