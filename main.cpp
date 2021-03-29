#include "tgaimage.h"
#include "vertex.h"
#include <iostream>
#include <limits>
#include "model.h"
#include "draw.h"

constexpr int W = 1200;
constexpr int H = 1200;
double zbuffer[W*H+1];

using namespace MSRender;

int main(int argc, char** argv)
{
    Model model;
    if (argc == 2) {
        model = Model(argv[1]);
    } else {
        model = Model("../obj/african_head/african_head.obj");
        // model = Model("../obj/diablo3_pose/diablo3_pose.obj");
    }
    TGAImage image(W, H, TGAImage::RGB);
    for(int i = W*H; i >= 0; i--) zbuffer[i] = -1.1;
    // #pragma omp parallel for
    // Point2i points[3] = {Point2i({0, 0}), Point2i({300, 150}), Point2i({0,300})};
    // triangle2d(points, image, TGAColor(255, 255, 255));
    Vector3d light_dir({0., 0., -1.});
    const TGAColor color(255, 255, 255);
    for(int i = 0; i < model.faces_size(); i++) {
        Point3d screen_coords[3];
        Point3d world_coords[3];
        for(int j = 0; j < 3; j++) {
            Point3d p = model.get_point(i, j);
            screen_coords[j] = Vector3d({(p[0]+1)*W*0.5, (p[1]+1)*H*0.5, p[2]});
            world_coords[j] = p;
        }
        Vector3d n = cross(world_coords[1] - world_coords[0], world_coords[2] - world_coords[0]);
        n.normalize();
        double intensity = - (n*light_dir);
        if(intensity > 0){
            // std::cout<<intensity<<std::endl;
            triangle(screen_coords, zbuffer, image, color*intensity);
        }
    }
    draw_zbuffer(zbuffer, image, color);
    // image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}