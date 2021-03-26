#include "tgaimage.h"
constexpr int W = 500;
constexpr int H = 500;
int main() {
    TGAImage image(W, H, TGAImage::RGB);
#pragma omp parallel for
    for(int i=0;i<250;i++){
        for(int j=0;j<250;j++){
            image.set(i, j, TGAColor(0, 255, 0));
            image.set(i+250, j, TGAColor(255, 0, 0));
            image.set(i, j+250, TGAColor(0, 0, 0));
            image.set(i+250, j+250, TGAColor(0, 0, 255));
        }
    }
    image.write_tga_file("output.tga");
    return 0;
}