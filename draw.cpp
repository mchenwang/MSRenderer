#include "draw.h"
#include <iostream>

static bool MSAA = true;
static int sampling_num = 16;

template<typename T, typename U>
bool in_triangle(Point<U, 2>* points, Point<T, 2> p) {
    Vector<T, 2> AB(points[0], points[1]);
    Vector<T, 2> BC(points[1], points[2]);
    Vector<T, 2> CA(points[2], points[0]);
    bool abo = AB.get_cross_z(Vector<T, 2>(points[0], p)) >= 0;
    bool bco = BC.get_cross_z(Vector<T, 2>(points[1], p)) >= 0;
    bool cao = CA.get_cross_z(Vector<T, 2>(points[2], p)) >= 0;
    return (abo && bco && cao) || !(abo || bco || cao);
}

void triangle(Point2i* points, TGAImage &image, TGAColor color) {
    Point2i bboxmin({image.get_width()-1,  image.get_height()-1});
    Point2i bboxmax({0, 0});
    Point2i clamp({image.get_width()-1, image.get_height()-1});
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0,        std::min(bboxmin[j], points[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], points[i][j]));
        }
    }
    Point2i P;
    for (P[0]=bboxmin[0]; P[0]<=bboxmax[0]; P[0]++) {
        for (P[1]=bboxmin[1]; P[1]<=bboxmax[1]; P[1]++) {
            if(MSAA) {
                int n = std::sqrt(sampling_num);
                double step = 1.0 / (n<<1);
                Point2d sp({P[0]+step, P[1]+step});
                int cnt = 0;
                for(double tx = step; tx <= 1; tx += step*2) {
                    for(double ty = step; ty <= 1; ty += step*2) {
                        if (in_triangle(points, Point2d({P[0]+tx, P[1]+ty}))) ++cnt;
                    }
                }
                if(cnt > 0) image.set(P[0], P[1], color*((double)cnt/sampling_num));
            } else {
                if (in_triangle(points, P)) image.set(P[0], P[1], color);
            }
        }
    }
}