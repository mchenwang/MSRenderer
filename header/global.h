#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include "algebra.h"

constexpr int H = 1200;
constexpr int W = 1200;

const MSRender::pointd eye_pos(1, 1, 3, 1);
const MSRender::pointd center(0, 0, 0, 1);
const MSRender::vecd eye_up_dir(0, 1, 0, 0);
constexpr double amb_light_intensity = 15.;

constexpr double eye_fov = 90.;
constexpr double aspect_ratio = 1.0;
constexpr double z_near = 0.1;
constexpr double z_far  = 50.;

constexpr double PI = 3.141592653;

// MSRender::mat4d light_space_matrix;

#endif