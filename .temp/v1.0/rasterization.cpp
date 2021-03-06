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
            if(z >= 0){
                image.set(i, j, color*z);
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

void triangle_with_Phong(MSRender::Fragment* fragments, MSRender::Shader* shader, TGAImage& image, double* zbuffer, const Model& model) {
    double x_max = std::max(fragments[0].screen_pos[0], std::max(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double x_min = std::min(fragments[0].screen_pos[0], std::min(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double y_max = std::max(fragments[0].screen_pos[1], std::max(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    double y_min = std::min(fragments[0].screen_pos[1], std::min(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    x_max = std::ceil (x_max); x_max = std::min(x_max, (double)image.get_width()-1);
    x_min = std::floor(x_min); x_min = std::max(x_min, 0.);
    y_max = std::ceil (y_max); y_max = std::min(y_max, (double)image.get_height()-1);
    y_min = std::floor(y_min); y_min = std::max(y_min, 0.);

    // Vector3d E1 = fragments[1].world_pos - fragments[0].world_pos;
    // Vector3d E2 = fragments[2].world_pos - fragments[0].world_pos;
    // double  du1 = fragments[1].uv[0] - fragments[0].uv[0];
    // double  dv1 = fragments[1].uv[1] - fragments[0].uv[1];
    // double  du2 = fragments[2].uv[0] - fragments[0].uv[0];
    // double  dv2 = fragments[2].uv[1] - fragments[0].uv[1];
    // double temp = dv2 * du1 - dv1 * du2;
    // Vector3d T = (E1 * dv2 - E2 * dv1) / temp;
    // Vector3d B = (E2 * du1 - E1 * du2) / temp;
    // Vector3d N = cross(B, T);
    // Eigen::Vector4d tempT = model.model_transf*Eigen::Vector4d(T[0],T[1],T[2],1.);
    // Eigen::Vector4d tempB = model.model_transf*Eigen::Vector4d(B[0],B[1],B[2],1.);
    // Eigen::Vector4d tempN = model.model_transf*Eigen::Vector4d(N[0],N[1],N[2],1.);
    // T = Vector3d({tempT(0)/tempT(3), tempT(1)/tempT(3), tempT(2)/tempT(3)});
    // B = Vector3d({tempB(0)/tempB(3), tempB(1)/tempB(3), tempB(2)/tempB(3)});
    // N = Vector3d({tempN(0)/tempN(3), tempN(1)/tempN(3), tempN(2)/tempN(3)});
    // T.normalize();
    // B.normalize();
    // N.normalize();
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
            
            if(model.has_normal_map()){
                // if(Model::nm_is_in_tangent){
                //     // fragment.normal = fragments[0].normal*bc_screen[0] + fragments[1].normal*bc_screen[1] + fragments[2].normal*bc_screen[2];
                //     // fragment.normal.normalize();
                //     // Vector3d nm_tan = model.get_normal_with_map(fragment.uv[0],fragment.uv[1]).normalized();
                //     // fragment.normal = Vector3d({nm_tan[0]*T[0]+nm_tan[1]*B[0]+nm_tan[2]*N[0],
                //     //                             nm_tan[0]*T[1]+nm_tan[1]*B[1]+nm_tan[2]*N[1],
                //     //                             nm_tan[0]*T[2]+nm_tan[1]*B[2]+nm_tan[2]*N[2]});
                //     // fragment.normal.normalize();
                // }
                // else 
                fragment.normal = model.get_normal_with_map(fragment.uv[0],fragment.uv[1]).normalized();
            }
            else fragment.normal = (fragments[0].normal*bc_screen[0] + fragments[1].normal*bc_screen[1] + fragments[2].normal*bc_screen[2]).normalized();
            // std::cout<<fragment.normal[0]<<" "<<fragment.normal[1]<<" "<<fragment.normal[2]<<"\n";
            if(model.has_diffuse_map())
                fragment.texture_color = model.get_diffuse(fragment.uv[0],fragment.uv[1]);
            else fragment.texture_color = TGAColor(0, 0, 0);
            if(model.has_specular_map())
                fragment.specular = model.get_specular(fragment.uv[0], fragment.uv[1]);
            else fragment.specular = 0.;
            
            if (zbuffer[P[0]+P[1]*image.get_width()] < z) {
                zbuffer[P[0]+P[1]*image.get_width()] = z;
                image.set(P[0], P[1], shader->shading(fragment));
            }
        }
    }
}

void triangle_with_shadow(MSRender::Fragment* fragments, MSRender::Shader* shader, TGAImage& image, double* zbuffer, const Model& model, double* shadow_map, MSRender::Point3d& light_pos) {
    double x_max = std::max(fragments[0].screen_pos[0], std::max(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double x_min = std::min(fragments[0].screen_pos[0], std::min(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double y_max = std::max(fragments[0].screen_pos[1], std::max(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    double y_min = std::min(fragments[0].screen_pos[1], std::min(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    x_max = std::ceil (x_max); x_max = std::min(x_max, (double)image.get_width()-1);
    x_min = std::floor(x_min); x_min = std::max(x_min, 0.);
    y_max = std::ceil (y_max); y_max = std::min(y_max, (double)image.get_height()-1);
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
            
            if(model.has_normal_map()){
                fragment.normal = model.get_normal_with_map(fragment.uv[0],fragment.uv[1]).normalized();
            }
            else fragment.normal = (fragments[0].normal*bc_screen[0] + fragments[1].normal*bc_screen[1] + fragments[2].normal*bc_screen[2]).normalized();
            
            if(model.has_diffuse_map())
                fragment.texture_color = model.get_diffuse(fragment.uv[0],fragment.uv[1]);
            else fragment.texture_color = TGAColor(0, 0, 0);
            if(model.has_specular_map())
                fragment.specular = model.get_specular(fragment.uv[0], fragment.uv[1]);
            else fragment.specular = 0.;

            // fragment.world_pos[2] = z;

            if (zbuffer[P[0]+P[1]*image.get_width()] < z) {
                zbuffer[P[0]+P[1]*image.get_width()] = z;
                Eigen::Vector4d temp = light_space_matrix * Eigen::Vector4d(fragment.world_pos[0], fragment.world_pos[1], fragment.world_pos[2], 1);
                double sx = (temp[0]/temp[3]+1)*0.5*image.get_width();
                double sy = (temp[1]/temp[3]+1)*0.5*image.get_height();
                double sz = (temp[2]/temp[3]+1)*0.5;
                double bias = std::max(0.005, 0.05 * (1.0 - fragment.normal * ((light_pos - Point3d({0.,0.,0.})).normalized())));
                
                // if(size_t(sx+sy*image.get_width())>=1440000){
                //     std::cout<<bc_screen[0]<<" "<<bc_screen[1]<<" "<<bc_screen[2]<<"\n";
                //     // std::cout<<fragments[1].world_pos[0]<<" "<<fragments[1].world_pos[1]<<" "<<fragments[1].world_pos[2]<<"\n";
                //     // std::cout<<fragment.world_pos[0]<<" "<<fragment.world_pos[1]<<" "<<fragment.world_pos[2]<<"\n\n";
                // }

                // if(shadow_map[size_t(sx+sy*image.get_width())] - bias > sz)
                //     image.set(P[0], P[1], shader->shading(fragment, 1));
                // else 
                image.set(P[0], P[1], shader->shading(fragment, 1));
            }
        }
    }
}

void get_shadow_zbuffer(MSRender::Fragment* fragments, TGAImage& image, double* shadow_map) {
    double x_max = std::max(fragments[0].screen_pos[0], std::max(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double x_min = std::min(fragments[0].screen_pos[0], std::min(fragments[1].screen_pos[0], fragments[2].screen_pos[0]));
    double y_max = std::max(fragments[0].screen_pos[1], std::max(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    double y_min = std::min(fragments[0].screen_pos[1], std::min(fragments[1].screen_pos[1], fragments[2].screen_pos[1]));
    x_max = std::ceil (x_max); x_max = std::min(x_max, (double)image.get_width()-1);
    x_min = std::floor(x_min); x_min = std::max(x_min, 0.);
    y_max = std::ceil (y_max); y_max = std::min(y_max, (double)image.get_height()-1);
    y_min = std::floor(y_min); y_min = std::max(y_min, 0.);
    Point3i P;
    for(P[0] = x_min; P[0] <= x_max; P[0]++) {
        for(P[1] = y_min; P[1] <= y_max; P[1]++) {
            Vector3d bc_screen  = barycentric(fragments[0].screen_pos, fragments[1].screen_pos, fragments[2].screen_pos, Point3d({P[0]+0.5, P[1]+0.5, 0.}));
            if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
            Point3d fragment = fragments[0].world_pos*bc_screen[0] + fragments[1].world_pos*bc_screen[1] + fragments[2].world_pos*bc_screen[2];
            Eigen::Vector4d temp = light_space_matrix * Eigen::Vector4d(fragment[0], fragment[1], fragment[2], 1);
            // double sx = (temp[0]/temp[3]+1)*image.get_width()*0.5;
            // double sy = (temp[1]/temp[3]+1)*image.get_height()*0.5;
            // double z = (temp[2]/temp[3]+1)*0.5;
            double z = temp[2]/temp[3];
            // if(z>-1&&z<-0.5) std::cout<<z<<" ";
            z = (z+1)*0.5;
            if (shadow_map[P[0]+P[1]*image.get_width()] < z) {
                shadow_map[P[0]+P[1]*image.get_width()] = z;
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
    Eigen::Matrix4d RotateX;
    RotateX << 1., 0., 0., 0.,
               0., cosx, -sinx, 0.,
               0., sinx, cosx, 0.,
               0., 0., 0., 1.;
    Eigen::Matrix4d RotateY;
    RotateY << cosy, 0., siny, 0.,
               0., 1., 0., 0.,
               -siny, 0., cosy, 0.,
               0., 0., 0., 1.;
    Eigen::Matrix4d RotateZ;
    RotateZ << cosz, -sinz, 0., 0.,
               sinz, cosz, 0., 0.,
               0., 0., 1., 0.,
               0., 0., 0., 1.;
    Rotate = RotateX * RotateY * RotateZ;
    // Rotate << cosy*cosz, -cosy*sinz, siny, 0.,
    //           sinx*siny*cosz+cosx*sinz, -sinx*siny*sinz+cosx*cosz, -sinx*cosy, 0.,
    //           -cosx*siny*cosz+sinx*sinz, cosx*siny*sinz+sinx*cosz, cosx*cosy, 0.,
    //           0., 0., 0., 1.;
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
    double n = std::abs(z_near), f = z_near - z_far;
    double t = -n * std::tan(eye_fov * 0.5 * PI/ 180);
    double b = -t;
    double r = t * aspect_ratio;
    double l = -r;
    Eigen::Matrix4d projection;
    projection << 2*n/(r-l), 0, (l+r)/(l-r), 0,
                  0, 2*n/(t-b), (t+b)/(b-t), 0,
                  0, 0, (n)/(n-f), n*f/(f-n),
                  0, 0, 1, 0;
    return projection;
    // Eigen::Matrix4d scale;
    // scale << 2./(r-l), 0., 0., 0.,
    //          0., 2./(t-b), 0., 0.,
    //          0., 0., 2./(n-f), 0.,
    //          0., 0., 0., 1.;
    // Eigen::Matrix4d translate;
    // translate << 1., 0., 0., -(l+r)*0.5,
    //              0., 1., 0., -(t+b)*0.5,
    //              0., 0., 1., -(f+n)*0.5,
    //              0., 0., 0., 1.;
    // Eigen::Matrix4d persp_to_ortho;
    // persp_to_ortho << n, 0., 0., 0.,
    //                   0., n, 0., 0.,
    //                   0., 0., (n+f), -n*f,
    //                   0., 0., 1., 0.;
    // auto xs = scale * translate * persp_to_ortho;
    // for(int i=0;i<4;i++){
    //     for(int j=0;j<4;j++) std::cout<<xs(i,j)<<" ";
    //     std::cout<<"\n";
    // }
    // return scale * translate * persp_to_ortho;
}

Eigen::Matrix4d ortho_proj_transf(double r, double t, double n) {
    double b = -t;
    double l = -r;
    double f = -n;
    Eigen::Matrix4d translate;
    translate << 1., 0., 0., -(l+r)*0.5,
                 0., 1., 0., -(t+b)*0.5,
                 0., 0., 1., -f,
                 0., 0., 0., 1.;
    Eigen::Matrix4d scale;
    scale << 2./(r-l), 0., 0., 0.,
             0., 2./(t-b), 0., 0.,
             0., 0., 1./(n-f), 0.,
             0., 0., 0., 1.;
    return scale * translate;
}
