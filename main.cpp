#include "tgaimage.h"
#include "vertex.h"
#include <iostream>
#include "model.h"
#include "draw.h"

constexpr int W = 300;
constexpr int H = 300;

int main(/*int argc, char** argv*/)
{
    // Model model;
    // if (argc == 2) {
    //     model = Model(argv[1]);
    // } else {
    //     model = Model("obj/african_head.obj");
    // }
    TGAImage image(W, H, TGAImage::RGB);
    // #pragma omp parallel for
    // for (int i = 0; i < W; i += 10)
    // {
    //     for (int j = 0; j < H; j += 10)
    //     {
    //         Vertex2d a({(double)i, (double)j});
    //         image.set(a[0], a[1], TGAColor(255, 255, 255));
    //     }
    // }
    Point2i points[3] = {Point2i({280,30}), Point2i({10, 10}), Point2i({150, 280})};
    triangle(points, image, TGAColor(255, 255, 255));
    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}