#ifndef __RASTERIZATION_H__
#define __RASTERIZATION_H__
#include "algebra.h"
#include "tgaimage.h"
#include "shader.h"

namespace MSRender{
    void rasterize(Triangle& tri, TGAImage& image, const Model& model, const PixelShader* shader, double* zbuffer, Light&, double* shadow_map=NULL);
    void draw_zbuffer(double*, TGAImage&, TGAColor);
    void shadow(Triangle& tri, double* shadow_map);
}
// void get_shadow_zbuffer(MSRender::Fragment*, TGAImage&, double*);

// // get mvp
// MSRender::mat4d model_transf(const double* scale, const double* thetas, const MSRender::vecd& translate);
// MSRender::mat4d view_transf(const MSRender::pointd& eye, const MSRender::vecd& eye_up_dir, const MSRender::pointd& center);
// MSRender::mat4d projection_transf(double eye_fov, double aspect_ratio, double z_near, double z_far);
// MSRender::mat4d ortho_proj_transf(double r, double t, double n);

#endif