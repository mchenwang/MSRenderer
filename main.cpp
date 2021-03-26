#include "tgaimage.h"
#include "vertex.h"
#include <iostream>
constexpr int W = 300;
constexpr int H = 300;
int main()
{
    TGAImage image(W, H, TGAImage::RGB);
// #pragma omp parallel for
//     for (int i = 0; i < W / 2; i++)
//     {
//         for (int j = 0; j < H / 2; j++)
//         {
//             image.set(i, j, TGAColor(0, 255, 0));
//             image.set(i + W / 2, j, TGAColor(255, 0, 0));
//             image.set(i, j + H / 2, TGAColor(0, 0, 0));
//             image.set(i + W / 2, j + H / 2, TGAColor(0, 0, 255));
//         }
//     }
    for(int i=0;i<W;i+=10){
        for(int j=0;j<H;j+=10){
            Point2i a({i, j});
            image.set(a[0],a[1], TGAColor(255, 255, 255));
            // for(int k=0;k<2;k++) std::cout<<a[k]<<std::endl;
        }
    }
    image.write_tga_file("output.tga");
    return 0;
}