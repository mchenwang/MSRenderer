#ifndef __DRAW_H__
#define __DRAW_H__
#include "vertex.h"
#include "tgaimage.h"
#include "shader.h"
#include <Eigen/Dense>

extern Eigen::Matrix4d light_space_matrix;
extern Eigen::Matrix4d mvp;

void triangle2d(MSRender::Point2i*, TGAImage&, TGAColor, bool has_dir = false);
void triangle(MSRender::Point3d*, double* zbuffer, TGAImage&, TGAColor);
void triangle_with_texture(MSRender::Point3d*, MSRender::Point2d*, double* zbuffer, TGAImage&, const TGAImage&, double);
void draw_zbuffer(double* zbuffer, TGAImage&, TGAColor);
void triangle_with_Phong(MSRender::Fragment*, MSRender::Shader*, TGAImage&, double*, const MSRender::Model&); // Blinn-Phong
void triangle_with_shadow(MSRender::Fragment*, MSRender::Shader*, TGAImage&, double*, const MSRender::Model&, double*, MSRender::Point3d&);
void get_shadow_zbuffer(MSRender::Fragment*, TGAImage&, double*);

// get mvp
Eigen::Matrix4d model_transf(const double* scale, const double* thetas, const MSRender::Vector3d& translate);
Eigen::Matrix4d view_transf(const MSRender::Point3d& eye, const MSRender::Vector3d& eye_up_dir, const MSRender::Point3d& center);
Eigen::Matrix4d projection_transf(double eye_fov, double aspect_ratio, double z_near, double z_far);
Eigen::Matrix4d ortho_proj_transf(double r, double t, double n);

#endif