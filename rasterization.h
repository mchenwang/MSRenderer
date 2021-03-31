#ifndef __DRAW_H__
#define __DRAW_H__
#include "vertex.h"
#include "tgaimage.h"

void triangle2d(MSRender::Point2i*, TGAImage&, TGAColor, bool has_dir = false);
void triangle(MSRender::Point3d*, double* zbuffer, TGAImage&, TGAColor);
void triangle_with_texture(MSRender::Point3d*, MSRender::Point2d*, double* zbuffer, TGAImage&, const TGAImage&, double&);
void draw_zbuffer(double* zbuffer, TGAImage&, TGAColor);

#endif