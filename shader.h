#ifndef __SHADER_H__
#define __SHADER_H__
#include "model.h"
#include "vertex.h"
#include "tgaimage.h"
#include <iostream>

namespace MSRender {

    struct Fragment {
        Point3d pos;
        Vector3d normal;
        Point2d uv;
        TGAColor texture_color;
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
        Shader(Point3d ep, double ali, std::vector<Light> ls)
        : eye_pos(ep), amb_light_intensity(ali), lights(ls) {}
        virtual TGAColor shading(const Fragment&) = 0;
    };

    class PhongShader: public Shader {
        const int p;
        double ka; // 全局光照系数
        double ks; // 镜面反射系数
        // TGAColor kd; // 漫反射系数，texture
    public:
        PhongShader(Point3d ep, double ali, std::vector<Light> ls, int _p=512, double ka_=0.005, double ks_=0.8)
        : Shader(ep, ali, ls), p(_p), ka(ka_), ks(ks_) {}
        TGAColor shading(const Fragment& fragment) override {
            Point3d result({0.,0.,0.});
            
            Point3d la({ka*amb_light_intensity, ka*amb_light_intensity, ka*amb_light_intensity});
            result += la;

            for(auto& light: lights) {
                Vector3d light_dir = light.pos - fragment.pos;
                Vector3d eye_dir = eye_pos - fragment.pos;
                double r2 = light_dir * light_dir;

                TGAColor kd = fragment.texture_color;
                // TGAColor kd = TGAColor(0, 0, 255);
                Vector3d ld({kd[0]*light.intensity/r2, kd[1]*light.intensity/r2, kd[2]*light.intensity/r2});
                ld *= std::max(0., fragment.normal * light_dir.normalized());
                Vector3d ls({ks*light.intensity/r2, ks*light.intensity/r2, ks*light.intensity/r2});
                // Vector3d ls({(1.-kd[0])*light.light_intensity/r2, (1.-kd[1])*light.light_intensity/r2, (1.-kd[2])*light.light_intensity/r2});
                ls *= std::pow(std::max(0., fragment.normal * (light_dir + eye_dir).normalized()), p);

                result += ld + ls;
            }
            
            // std::cout<<ld[0]<<" "<<ld[1]<<" "<<ld[2]<<"\n";
            TGAColor result_color(
                (std::uint8_t) std::min(255., result[0]*255.),
                (std::uint8_t) std::min(255., result[1]*255.),
                (std::uint8_t) std::min(255., result[2]*255.));
            // TGAColor result_color(
            //     (std::uint8_t) std::min(255., result[2]*255.),
            //     (std::uint8_t) std::min(255., result[1]*255.),
            //     (std::uint8_t) std::min(255., result[0]*255.));
            
            // std::cout<<(int)result_color[0]<<" "<<(int)result_color[1]<<" "<<(int)result_color[2]<<"\n";
            return result_color;
        }
    };
}

#endif