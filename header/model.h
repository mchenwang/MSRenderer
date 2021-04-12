#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include "tgaimage.h"
#include "algebra.h"

namespace MSRender{
    
    struct ModelTransfParam {
        double scale[3] = {1., 1., 1.};
        double thetas[3] = {0., 0., 0.};
        MSRender::vecd translate;
    };
    
    class Model {
    private:
        std::vector<pointd> vertices;
        std::vector<uvd> uvs;
        std::vector<vecd> normals;
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
        Model() {}
        Model(const std::string filename);
        size_t vertexs_size() const;
        size_t faces_size() const;
        vecd get_normal(const size_t iface, const size_t nthvert) const;  // per triangle corner normal vertex
        pointd get_vertex(const size_t iface, const size_t nthvert) const;
        uvd get_uv(const size_t iface, const size_t nthvert) const;
        vecd get_diffuse(const uvd &uv) const;
        double get_specular(const uvd &uv) const;
        vecd get_diffuse(const double uv0, const double uv1) const;
        // void set_diffuse(const double uv0, const double uv1) ;
        double get_specular(const double uv0, const double uv1) const;
        vecd get_normal_with_map(const uvd &uv) const; // fetch the normal vector from the normal map texture
        vecd get_normal_with_map(const double uv0, const double uv1) const;

        const TGAImage& get_diffusemap() const;
        const TGAImage& get_normalmap() const;
        const TGAImage& get_specularmap() const;

        bool has_diffuse_map() const { return has_diffusemap; }
        bool has_normal_map() const { return has_normalmap; }
        bool has_specular_map() const { return has_specularmap; }

        mat4d model_matrix;
        // 模型变换的逆矩阵的转置
        mat4d normal_matrix;
        static bool nm_is_in_tangent;

        void set_model_matrix(const ModelTransfParam&);
        void set_normal_matrix();
        pointd model_transf(pointd&& p) const;
        vecd model_nm_transf(vecd&& nm) const;
    };
}

#endif