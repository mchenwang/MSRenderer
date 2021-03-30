#include "draw.h"
#include <iostream>

using namespace MSRender;

static bool MSAA = false;
static int sampling_num = 16;

template<typename T, typename U, size_t n>
bool in_triangle(Point<U, n>* points, Point<T, n> p, bool has_dir = false) {
    Vector<T, n> AB(points[0], points[1]);
    Vector<T, n> BC(points[1], points[2]);
    Vector<T, n> CA(points[2], points[0]);
    bool abo = AB.get_cross_z(Vector<T, n>(points[0], p)) >= 0;
    bool bco = BC.get_cross_z(Vector<T, n>(points[1], p)) >= 0;
    bool cao = CA.get_cross_z(Vector<T, n>(points[2], p)) >= 0;
    
    // 三角形顶点逆时针为正面
    if(has_dir) return abo && bco && cao;
    // 不论三角形正反面
    return (abo && bco && cao) || !(abo || bco || cao);
}

Vector3d barycentric(Point3d A, Point3d B, Point3d C, Point3d P) {
    Vector3d x({B[0]-A[0], C[0]-A[0], A[0]-P[0]});
    Vector3d y({B[1]-A[1], C[1]-A[1], A[1]-P[1]});
    double u = (x[1]*y[2] - x[2]*y[1]) / (x[0]*y[1] - x[1]*y[0]);
    double v = (x[0]*y[2] - x[2]*y[0]) / (x[1]*y[0] - x[0]*y[1]);
    return Vector3d({1.-u-v, u, v});
}

void triangle(Point3d* points, double* zbuffer, TGAImage &image, TGAColor color) {
    Point2d bboxmin({(double)image.get_width()-1, (double)image.get_height()-1});
    Point2d bboxmax({0., 0.});
    Point2d clamp({(double)image.get_width()-1, (double)image.get_height()-1});
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0.,       std::min(bboxmin[j], points[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], points[i][j]));
        }
    }
    
    Point3i P;
    for (P[0]=bboxmin[0]; P[0]<=bboxmax[0]; P[0]++) {
        for (P[1]=bboxmin[1]; P[1]<=bboxmax[1]; P[1]++) {
            Vector3d bc_screen  = barycentric(points[0], points[1], points[2], P);
            if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
            // if (in_triangle(points, P))
            //     image.set(P[0], P[1], color);
            double z = 0.;
            for (int i=0; i<3; i++) z += points[i][2]*bc_screen[i];
            if (zbuffer[P[0]+P[1]*image.get_width()] < z) {
                zbuffer[P[0]+P[1]*image.get_width()] = z;
                image.set(P[0], P[1], color);
            }
        }
    }
}

void triangle2d(Point2i* points, TGAImage &image, TGAColor color, bool has_dir) {
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
                // if (in_triangle(points, P, has_dir)) image.set(P[0], P[1], color);
                // 测试重心坐标，染色
                if (in_triangle(points, P, has_dir)){
                    Point3d A = Point3i({points[0][0], points[0][1], 0});
                    Point3d B = Point3i({points[1][0], points[1][1], 0});
                    Point3d C = Point3i({points[2][0], points[2][1], 0});
                    Point3d D = Point3i({P[0], P[1], 0});
                    Vector3d bc_screen  = barycentric(A, B, C, D);
                    // std::cout<<bc_screen[0]<<" "<<bc_screen[1]<<" "<<bc_screen[2]<<std::endl;
                    if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
                    image.set(P[0], P[1], TGAColor(color[0]*bc_screen[0], color[1]*bc_screen[1], color[2]*bc_screen[2]));
                }
            }
        }
    }
}

void draw_zbuffer(double* zbuffer, TGAImage &image, TGAColor color) {
    for(int i = 0; i < image.get_width(); i++) {
        for(int j = 0; j < image.get_height(); j++) {
            double z=zbuffer[i+j*image.get_width()];
            if(z > -1){
                if(z > 0.7) image.set(i, j, color);
                else if(z < -0.5) image.set(i, j, color*0.1);
                else image.set(i, j, color*((z+0.5)*0.5));
            }
        }
    }
}

void triangle_with_texture(Point3d* points, Point2d* uvs, double* zbuffer, TGAImage& image, const TGAImage& texture, double& intensity) {
    Point2d bboxmin({(double)image.get_width()-1, (double)image.get_height()-1});
    Point2d bboxmax({0., 0.});
    Point2d clamp({(double)image.get_width()-1, (double)image.get_height()-1});
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0.,       std::min(bboxmin[j], points[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], points[i][j]));
        }
    }
    
    Point3i P;
    for (P[0]=bboxmin[0]; P[0]<=bboxmax[0]; P[0]++) {
        for (P[1]=bboxmin[1]; P[1]<=bboxmax[1]; P[1]++) {
            Vector3d bc_screen  = barycentric(points[0], points[1], points[2], P);
            if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
            // if (in_triangle(points, P))
            //     image.set(P[0], P[1], color);
            double z = 0.;
            Point2d uv({0., 0.});
            for (int i=0; i<3; i++){
                z += points[i][2]*bc_screen[i];
                uv[0] += uvs[i][0]*bc_screen[i];
                uv[1] += uvs[i][1]*bc_screen[i];
            }
            if (zbuffer[P[0]+P[1]*image.get_width()] < z) {
                zbuffer[P[0]+P[1]*image.get_width()] = z;
                image.set(P[0], P[1], texture.get(uv[0]*texture.get_width(), uv[1]*texture.get_height())*intensity);
            }
        }
    }
}