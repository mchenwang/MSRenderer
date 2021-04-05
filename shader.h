#ifndef __SHADER_H__
#define __SHADER_H__
#include "model.h"
#include "vertex.h"
#include "tgaimage.h"
#include <iostream>

namespace MSRender {

    struct Fragment {
        Point3d screen_pos;
        Point3d p;
        double w;
        Point3d world_pos;
        Vector3d normal;
        Point2d uv;
        TGAColor texture_color;
        double specular; // 黑白只有一个值
    };
    struct Light {
        Point3d pos;
        double intensity;
        Light(Point3d pos_, double intensity_): pos(pos_), intensity(intensity_) {}
    };

    class Shader {
    protected:
        Point3d eye_pos;
        double amb_light_intensity;
        std::vector<Light> lights;
    public:
        virtual ~Shader() = default;
        Shader(Point3d ep, double ali, std::vector<Light> ls)
        : eye_pos(ep), amb_light_intensity(ali), lights(ls) {}
        virtual TGAColor shading(const Fragment&) = 0;
    };

    class PhongShader: public Shader {
        const int p;
        double ka; // 全局光照系数
        // double ks; // 镜面反射系数
        // TGAColor kd; // 漫反射系数，texture
    public:
        ~PhongShader() = default;
        PhongShader(Point3d ep, double ali, std::vector<Light> ls, int _p=128, double ka_=0.005/*, double ks_=0.7*/)
        : Shader(ep, ali, ls), p(_p), ka(ka_)/*, ks(ks_)*/ {}
        TGAColor shading(const Fragment& fragment) override {
            Point3d result({0.,0.,0.});
            
            Point3d la({ka*amb_light_intensity, ka*amb_light_intensity, ka*amb_light_intensity});
            result += la;

            TGAColor kd = fragment.texture_color;
            double ks = fragment.specular;

            for(auto& light: lights) {
                Vector3d light_dir = light.pos - fragment.world_pos;
                Vector3d eye_dir = eye_pos - fragment.world_pos;
                double r2 = light_dir * light_dir;

                Vector3d ld({kd[0]*light.intensity/r2, kd[1]*light.intensity/r2, kd[2]*light.intensity/r2});
                ld *= std::max(0., fragment.normal * light_dir.normalized());
                Vector3d ls({ks*light.intensity/r2, ks*light.intensity/r2, ks*light.intensity/r2});
                ls *= std::pow(std::max(0., fragment.normal * (light_dir + eye_dir).normalized()), p);
                result += ld + ls;
            }
            
            TGAColor result_color(
                (std::uint8_t) std::min(255., result[0]*255.),
                (std::uint8_t) std::min(255., result[1]*255.),
                (std::uint8_t) std::min(255., result[2]*255.));
            
            return result_color;
        }
    };
}

#endif