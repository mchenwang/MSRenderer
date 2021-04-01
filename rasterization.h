#ifndef __DRAW_H__
#define __DRAW_H__
#include "vertex.h"
#include "tgaimage.h"
#include "shader.h"
#include <Eigen/Dense>

void triangle2d(MSRender::Point2i*, TGAImage&, TGAColor, bool has_dir = false);
void triangle(MSRender::Point3d*, double* zbuffer, TGAImage&, TGAColor);
void triangle_with_texture(MSRender::Point3d*, MSRender::Point2d*, double* zbuffer, TGAImage&, const TGAImage&, double);
void draw_zbuffer(double* zbuffer, TGAImage&, TGAColor);
void triangle_with_Phong(MSRender::Fragment*, MSRender::Shader*, TGAImage&, double*, const TGAImage&); // Blinn-Phong

// get mvp
Eigen::Matrix4d model_transf(const double* scale, const double* thetas, const MSRender::Vector3d& translate);
Eigen::Matrix4d view_transf(const MSRender::Point3d& eye, const MSRender::Vector3d& eye_up_dir, const MSRender::Point3d& center);
Eigen::Matrix4d projection_transf(double eye_fov, double aspect_ratio, double z_near, double z_far);

#endif