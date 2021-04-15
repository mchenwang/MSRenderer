#ifndef __SHADER_H__
#define __SHADER_H__
#include "model.h"
#include "algebra.h"
#include "tgaimage.h"
#include "global.h"
#include <iostream>

namespace MSRender {

    struct Vertex {
        pointd screen_pos;
        pointd light_space_pos;
        double w;
        pointd world_pos;
        vecd normal;
        uvd uv;
        pointd texture;
        pointd glow;
        double specular; // 黑白只有一个值
    };
    using Fragment = Vertex;

    struct Triangle {
        Vertex vertex[3];
        Vertex operator[](const size_t i) const {return vertex[i];}
        Vertex& operator[](const size_t i) {return vertex[i];}
    };

    struct Light {
        pointd pos;
        double intensity;
        TGAColor color;
        Light(pointd pos_, double intensity_)
        : pos(pos_), intensity(intensity_), color(TGAColor(255, 255, 255)) {
            set_light_space_matrix(center, eye_up_dir, shadow_map_size);
        }
        Light(pointd pos_, double intensity_, TGAColor c)
        : pos(pos_), intensity(intensity_), color(c) {
            set_light_space_matrix(center, eye_up_dir, shadow_map_size);
        }

        mat4d light_space_matrix;
        void set_light_space_matrix(vecd center, vecd up, double r);
        pointd get_light_space(pointd p);
    };

    class PixelShader {
    protected:
        std::vector<Light> lights;
    public:
        virtual ~PixelShader() = default;
        PixelShader(std::vector<Light> ls)
        : lights(ls) {}
        virtual TGAColor shading(const Fragment&, double shadow=1.) const = 0;
    };

    class PhongShader: public PixelShader {
        const int p;
        double ka; // 全局光照系数
        // double ks; // 镜面反射系数
        // TGAColor kd; // 漫反射系数，texture
    public:
        ~PhongShader() = default;
        PhongShader(std::vector<Light> ls, int _p=512, double ka_=0.5/*, double ks_=0.7*/)
        : PixelShader(ls), p(_p), ka(ka_)/*, ks(ks_)*/ {}
        TGAColor shading(const Fragment& fragment, double shadow=1.) const override ;
    };

    class VertexShader {
        mat4d projection_matrix;
        mat4d view_matrix;
        mat4d vp;
        void set_view_matrix(const pointd& eye, const vecd& eye_up_dir, const pointd& center);
        void set_projection_matrix(double eye_fov, double aspect_ratio, double z_near, double z_far);
    public:
        VertexShader();
        ~VertexShader() = default;

        // mvp 变换 + 视口变换
        Vertex shading(const Model&, const size_t, const size_t);
    };
}

#endif