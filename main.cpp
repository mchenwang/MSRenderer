#include "tgaimage.h"
#include "vertex.h"
#include <iostream>
#include <limits>
#include "model.h"
#include "draw.h"

constexpr int W = 1200;
constexpr int H = 1200;

using namespace MSRender;

static double zbuffer[W*H+1];
static TGAImage image(W, H, TGAImage::RGB);
static Model model;

void draw_a_triangle() {
    Point2i points[3] = {Point2i({0, 0}), Point2i({300, 150}), Point2i({0,300})};
    triangle2d(points, image, TGAColor(255, 255, 255));
}

void draw_model() {
    Vector3d light_dir({0., 0., -1.});
    const TGAColor color(255, 255, 255);
    for(int i = 0; i < model.faces_size(); i++) {
        Point3d screen_coords[3];
        Point3d world_coords[3];
        for(int j = 0; j < 3; j++) {
            Point3d p = model.get_vertex(i, j);
            screen_coords[j] = Vector3d({(p[0]+1)*W*0.5, (p[1]+1)*H*0.5, p[2]});
            world_coords[j] = p;
        }
        Vector3d n = cross(world_coords[1] - world_coords[0], world_coords[2] - world_coords[0]);
        n.normalize();
        double intensity = - (n*light_dir);
        if(intensity > 0){
            triangle(screen_coords, zbuffer, image, color*intensity);
        }
    }
    // draw_zbuffer(zbuffer, image, color);
}

void draw_model_with_texture() {
    Vector3d light_dir({0., 0., -1.});
    const TGAColor color(255, 255, 255);
    for(int i = 0; i < model.faces_size(); i++) {
        Point3d screen_coords[3];
        Point3d world_coords[3];
        Point2d uvs[3];
        for(int j = 0; j < 3; j++) {
            Point3d p = model.get_vertex(i, j);
            screen_coords[j] = Vector3d({(p[0]+1)*W*0.5, (p[1]+1)*H*0.5, p[2]});
            world_coords[j] = p;
            uvs[j] = model.get_uv(i, j);
        }
        Vector3d n = cross(world_coords[1] - world_coords[0], world_coords[2] - world_coords[0]);
        n.normalize();
        double intensity = - (n*light_dir);
        if(intensity > 0){
            triangle_with_texture(screen_coords, uvs, zbuffer, image, model.get_diffusemap(), intensity);
        }
    }
}

int main(int argc, char** argv)
{
    if (argc == 2) {
        model = Model(argv[1]);
    } else {
        // model = Model("../obj/floor.obj");
        model = Model("../obj/african_head/african_head.obj");
        // model = Model("../obj/diablo3_pose/diablo3_pose.obj");
    }
    for(int i = W*H; i >= 0; i--) zbuffer[i] = -1.1;
    // #pragma omp parallel for
    // draw_model();
    draw_model_with_texture();
    // image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}