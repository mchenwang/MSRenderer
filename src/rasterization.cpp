#include "rasterization.h"
#include "global.h"

using namespace MSRender;
template<typename T, typename U>
static inline double max(T a, U b) { return a>b?a:b; }
template<typename T, typename U>
static inline double min(T a, U b) { return a<b?a:b; }

struct bbox { int max_x, min_x, max_y, min_y; };
static inline bbox get_bbox(pointd A, pointd B, pointd C) {
    double max_x = min(W-1, max(A.x, max(B.x, C.x)));
    double min_x = max(0,   min(A.x, min(B.x, C.x)));
    double max_y = min(H-1, max(A.y, max(B.y, C.y)));
    double min_y = max(0,   min(A.y, min(B.y, C.y)));
    return {(int)std::ceil(max_x), (int)std::floor(min_x), (int)std::ceil(max_y), (int)std::floor(min_y)};
}

static vecd barycentric(pointd A, pointd B, pointd C, pointd P) {
    vecd x(B.x-A.x, C.x-A.x, A.x-P.x);
    vecd y(B.y-A.y, C.y-A.y, A.y-P.y);
    double u = (x.y*y.z - x.z*y.y) / (x.x*y.y - x.y*y.x);
    double v = (x.x*y.z - x.z*y.x) / (x.y*y.x - x.x*y.y);
    return vecd(1.-u-v, u, v);
}

template<typename T>
inline T interpolation(T a, T b, T c, vecd& bc) {
    return a * bc[0] + b * bc[1] + c * bc[2];
}

inline std::pair<vecd, vecd> getTB(Triangle& tri, bool flag) {
    if(!flag) return {vecd(), vecd()};
    vecd E1    = tri[1].world_pos - tri[0].world_pos;
    vecd E2    = tri[2].world_pos - tri[0].world_pos;
    double du1 = tri[1].uv.u - tri[0].uv.u;
    double dv1 = tri[1].uv.v - tri[0].uv.v;
    double du2 = tri[2].uv.u - tri[0].uv.u;
    double dv2 = tri[2].uv.v - tri[0].uv.v;
    double temp = dv2 * du1 - dv1 * du2;
    vecd T = ((E1 * dv2 - E2 * dv1) / temp).normalized();
    vecd B = ((E2 * du1 - E1 * du2) / temp).normalized();
    return {T, B};
}

void MSRender::rasterize(Triangle& tri, TGAImage& image, const Model& model, const PixelShader* shader, double* zbuffer, Light& light, double* shadow_map) {
    auto [max_x, min_x, max_y, min_y] = get_bbox(tri[0].screen_pos, tri[1].screen_pos, tri[2].screen_pos);

    auto [T, B] = getTB(tri, model.has_normal_map() && Model::nm_is_in_tangent);

    for(int x = min_x; x <= max_x; x++) {
        for(int y = min_y; y <= max_y; y++){
            vecd bc_screen = barycentric(tri[0].screen_pos, tri[1].screen_pos, tri[2].screen_pos, pointd(x+0.5, y+0.5));
            if(bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0) continue;
            
            // 透视修正
            double zt = bc_screen[0] / tri[0].w + bc_screen[1] / tri[1].w + bc_screen[2] / tri[2].w;
            double z = interpolation(tri[0].screen_pos.z, tri[1].screen_pos.z, tri[2].screen_pos.z, bc_screen);
            z = z / zt;
            
            bc_screen[0] /= (zt*tri[0].w);
            bc_screen[1] /= (zt*tri[1].w);
            bc_screen[2] /= (zt*tri[2].w);
            
            if(zbuffer[x + y * W] < z) {
                zbuffer[x + y * W] = z;
                
                Fragment f;
                f.world_pos = interpolation(tri[0].world_pos, tri[1].world_pos, tri[2].world_pos, bc_screen);
                f.uv        = interpolation(tri[0].uv, tri[1].uv, tri[2].uv, bc_screen);
                f.normal    = interpolation(tri[0].normal, tri[1].normal, tri[2].normal, bc_screen).normalized();
                
                if(model.has_diffuse_map())
                    f.texture = model.get_diffuse(f.uv);
                else f.texture = interpolation(tri[0].texture, tri[1].texture, tri[2].texture, bc_screen);
                if(model.has_specular_map())
                    f.specular = model.get_specular(f.uv);
                else f.specular = interpolation(tri[0].specular, tri[1].specular, tri[2].specular, bc_screen);
                
                if(model.has_normal_map()) {
                    if(Model::nm_is_in_tangent){
                        vecd N = f.normal;
                        // vecd T = (U - N*(U*N)).normalized();
                        // vecd B = cross(N, T).normalized();
                        vecd nm_tan = model.get_normal_with_map(f.uv);
                        f.normal = vecd(nm_tan[0] * T[0] + nm_tan[1] * B[0] + nm_tan[2] * N[0],
                                        nm_tan[0] * T[1] + nm_tan[1] * B[1] + nm_tan[2] * N[1],
                                        nm_tan[0] * T[2] + nm_tan[1] * B[2] + nm_tan[2] * N[2], 
                                        0).normalized();
                    }
                    else f.normal = model.get_normal_with_map(f.uv);
                }

                f.light_space_pos = light.get_light_space(f.world_pos);
                int sx = (f.light_space_pos.x + 1)*W*0.5;
                int sy = (f.light_space_pos.y + 1)*H*0.5;
                double bias = std::max(0.005, 0.05 * (1.0 - f.normal * (light.pos - f.world_pos).normalized()));
                
                if(shadow_map[sx + sy * W] - bias > f.light_space_pos.z)
                    image.set(x, y, shader->shading(f, 0.3));
                else image.set(x, y, shader->shading(f, 1));
            }
        }
    }
}

void MSRender::draw_zbuffer(double* zbuffer, TGAImage &image, TGAColor color) {
    double z_min = -1, z_max = -1;
    bool flag = true;
    for(int i = 0; i < H*W; i++){
        if(zbuffer[i] > -z_far-1) {
            if(flag) z_min = z_max = zbuffer[i], flag = false;
            else z_min = std::min(z_min, zbuffer[i]), z_max = std::max(z_max, zbuffer[i]);
        }
    }
    for(int i = 0; i < H*W; i++) zbuffer[i] = (zbuffer[i] - z_min) / (z_max - z_min);
    for(int i = 0; i < image.get_width(); i++) {
        for(int j = 0; j < image.get_height(); j++) {
            double z=zbuffer[i+j*image.get_width()];
            if(z >= 0){
                image.set(i, j, color*z);
            }
        }
    }
}

void MSRender::shadow(Triangle& tri, double* shadow_map) {
    auto [max_x, min_x, max_y, min_y] = get_bbox(tri[0].light_space_pos, tri[1].light_space_pos, tri[2].light_space_pos);

    for(int x = min_x; x <= max_x; x++) {
        for(int y = min_y; y <= max_y; y++){
            vecd bc_screen = barycentric(tri[0].light_space_pos, tri[1].light_space_pos, tri[2].light_space_pos, pointd(x+0.5, y+0.5));
            if(bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0) continue;
            
            double z = interpolation(tri[0].light_space_pos.z, tri[1].light_space_pos.z, tri[2].light_space_pos.z, bc_screen);
            
            if(shadow_map[x + y * W] < z) {
                shadow_map[x + y * W] = z;
            }
        }
    }
}