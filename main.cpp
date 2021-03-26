#include "tgaimage.h"
constexpr int W = 100;
constexpr int H = 100;
int main()
{
    TGAImage image(W, H, TGAImage::RGB);
#pragma omp parallel for
    for (int i = 0; i < W / 2; i++)
    {
        for (int j = 0; j < H / 2; j++)
        {
            image.set(i, j, TGAColor(0, 255, 0));
            image.set(i + W / 2, j, TGAColor(255, 0, 0));
            image.set(i, j + H / 2, TGAColor(0, 0, 0));
            image.set(i + W / 2, j + H / 2, TGAColor(0, 0, 255));
        }
    }
    image.write_tga_file("output.tga");
    return 0;
}