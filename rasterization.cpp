#include "rasterization.h"
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
            Vector3d bc_screen  = barycentric(points[0], points[1], points[2], Point3d({P[0]+0.5,P[1]+0.5,0.}));
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
                if (in_triangle(points, Point2d({P[0]+0.5,P[1]+0.5}), has_dir)){
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

void triangle_with_texture(Point3d* points, Point2d* uvs, double* zbuffer, TGAImage& image, const TGAImage& texture, double intensity) {
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
            Vector3d bc_screen  = barycentric(points[0], points[1], points[2], Point3d({P[0]+0.5,P[1]+0.5,0.}));
            if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
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

void triangle_with_Phong(MSRender::Fragment* fragments, MSRender::Shader* shader, TGAImage& image, double* zbuffer, const TGAImage& texture, const TGAImage& specular) {
    double x_max = std::max(fragments[0].screen_pos[0], std::max(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double x_min = std::min(fragments[0].screen_pos[0], std::min(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double y_max = std::max(fragments[0].screen_pos[1], std::max(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    double y_min = std::min(fragments[0].screen_pos[1], std::min(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    x_max = std::ceil (x_max); x_max = std::min(x_max, (double)image.get_width());
    x_min = std::floor(x_min); x_min = std::max(x_min, 0.);
    y_max = std::ceil (y_max); y_max = std::min(y_max, (double)image.get_height());
    y_min = std::floor(y_min); y_min = std::max(y_min, 0.);
    Point3i P;
    for(P[0] = x_min; P[0] <= x_max; P[0]++) {
        for(P[1] = y_min; P[1] <= y_max; P[1]++) {
            Vector3d bc_screen  = barycentric(fragments[0].screen_pos, fragments[1].screen_pos, fragments[2].screen_pos, Point3d({P[0]+0.5, P[1]+0.5, 0.}));
            if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
            
            double zt = bc_screen[0] / fragments[0].w + bc_screen[1] / fragments[1].w + bc_screen[2] / fragments[2].w;
            double z = -zt;
            // 透视修正
            bc_screen[0] = bc_screen[0] / (zt*fragments[0].w);
            bc_screen[1] = bc_screen[1] / (zt*fragments[1].w);
            bc_screen[2] = bc_screen[2] / (zt*fragments[2].w);
            
            Fragment fragment;
            fragment.world_pos = fragments[0].world_pos*bc_screen[0] + fragments[1].world_pos*bc_screen[1] + fragments[2].world_pos*bc_screen[2];
            fragment.uv = fragments[0].uv*bc_screen[0] + fragments[1].uv*bc_screen[1] + fragments[2].uv*bc_screen[2];
            
            fragment.normal = fragments[0].normal*bc_screen[0] + fragments[1].normal*bc_screen[1] + fragments[2].normal*bc_screen[2];
            fragment.normal.normalize();
            fragment.texture_color = texture.get(fragment.uv[0]*texture.get_width(), fragment.uv[1]*texture.get_height());
            fragment.specular = specular.get(fragment.uv[0]*specular.get_width(), fragment.uv[1]*specular.get_height())[0];
            
            // z = fragment.world_pos[2];
            if (zbuffer[P[0]+P[1]*image.get_width()] < z) {
                zbuffer[P[0]+P[1]*image.get_width()] = z;
                image.set(P[0], P[1], shader->shading(fragment));
            }
        }
    }
}

constexpr double PI = 3.141592653;
Eigen::Matrix4d model_transf(const double* scale, const double* thetas, const Vector3d& translate) {
    Eigen::Matrix4d Scale;
    Scale << scale[0], 0., 0., 0.,
             0., scale[1], 0., 0.,
             0., 0., scale[2], 0.,
             0., 0., 0., 1.;
    Eigen::Matrix4d Rotate;
    double cosx = std::cos(thetas[0]*PI/180.);
    double sinx = 1. - cosx * cosx;
    double cosy = std::cos(thetas[1]*PI/180.);
    double siny = 1. - cosy * cosy;
    double cosz = std::cos(thetas[2]*PI/180.);
    double sinz = 1. - cosz * cosz;
    // Eigen::Matrix4d RotateX;
    // RotateX << 1., 0., 0., 0.,
    //            0., cosx, -sinx, 0.,
    //            0., sinx, cosx, 0.,
    //            0., 0., 0., 1.;
    // Eigen::Matrix4d RotateY;
    // RotateY << cosy, 0., siny, 0.,
    //            0., 1., 0., 0.,
    //            -siny, 0., cosy, 0.,
    //            0., 0., 0., 1.;
    // Eigen::Matrix4d RotateZ;
    // RotateZ << cosz, -sinz, 0., 0.,
    //            sinz, cosz, 0., 0.,
    //            0., 0., 1., 0.,
    //            0., 0., 0., 1.;
    // Rotate = RotateX * RotateY * RotateZ;
    Rotate << cosy*cosz, -cosy*sinz, siny, 0.,
              sinx*siny*cosz+cosx*sinz, -sinx*siny*sinz+cosx*cosz, -sinx*cosy, 0.,
              -cosx*siny*cosz+sinx*sinz, cosx*siny*sinz+sinx*cosz, cosx*cosy, 0.,
              0., 0., 0., 1.;
    Eigen::Matrix4d Translate;
    Translate << 1., 0., 0., translate[0],
                 0., 1., 0., translate[1],
                 0., 0., 1., translate[2],
                 0., 0., 0., 1.;
    return Translate * Rotate * Scale;
}

Eigen::Matrix4d view_transf(const Point3d& eye, const Vector3d& eye_up_dir, const Point3d& center) {
    Vector3d z = (eye - center).normalize();
    Vector3d x = cross(eye_up_dir, z).normalize();
    Vector3d y = cross(z, x).normalize();

    Eigen::Matrix4d T;
    T << 1., 0., 0., -eye[0],
         0., 1., 0., -eye[1],
         0., 0., 1., -eye[2],
         0., 0., 0., 1.;
    Eigen::Matrix4d R;
    // g = -z
    R << x[0], x[1], x[2], 0.,
         y[0], y[1], y[2], 0.,
         z[0], z[1], z[2], 0.,
         0.  , 0.  , 0.  , 1.;
    // auto xs = R*T;
    // for(int i=0;i<4;i++){
    //     for(int j=0;j<4;j++) std::cout<<R(i,j)<<" ";
    //     std::cout<<"\n";
    // }
    return R*T;
}

Eigen::Matrix4d projection_transf(double eye_fov, double aspect_ratio, double z_near, double z_far) {
    double n = z_near, f = z_far;
    double t = -n * std::tan(eye_fov * 0.5 * PI/ 180);
    double b = -t;
    double r = t * aspect_ratio;
    double l = -r;
    Eigen::Matrix4d translate;
    translate << 1., 0., 0., -(l+r)*0.5,
                 0., 1., 0., -(t+b)*0.5,
                 0., 0., 1., -(f+n)*0.5,
                 0., 0., 0., 1.;
    Eigen::Matrix4d scale;
    scale << 2./(r-l), 0., 0., 0.,
             0., 2./(t-b), 0., 0.,
             0., 0., 2./(n-f), 0.,
             0., 0., 0., 1.;
    Eigen::Matrix4d persp_to_ortho;
    persp_to_ortho << n, 0., 0., 0.,
                      0., n, 0., 0.,
                      0., 0., (n+f), -n*f,
                      0., 0., 1., 0.;
    return translate * scale * persp_to_ortho;
}