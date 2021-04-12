#include "shader.h"

using namespace MSRender;

TGAColor PhongShader::shading(const Fragment& fragment, double shadow) const {
    pointd result(0.,0.,0.);
    
    pointd la(ka*amb_light_intensity, ka*amb_light_intensity, ka*amb_light_intensity);
    result += la;

    vecd kd(fragment.texture[0]/255., fragment.texture[1]/255., fragment.texture[2]/255.);
    double ks = fragment.specular/255.;

    for(auto& light: lights) {
        vecd light_dir = light.pos - fragment.world_pos;
        vecd eye_dir = eye_pos - fragment.world_pos;
        double r2 = light_dir.norm2();

        vecd ld(kd[0]*light.intensity/r2, kd[1]*light.intensity/r2, kd[2]*light.intensity/r2);
        ld *= std::max(0., fragment.normal * light_dir.normalized());

        vecd ls(ks*light.intensity/r2, ks*light.intensity/r2, ks*light.intensity/r2);
        ls *= std::pow(std::max(0., fragment.normal * (light_dir + eye_dir).normalized()), p);

        result += (ld + ls)*255.;
    }
    
    return TGAColor((std::uint8_t) std::min(255., result[0]*shadow),
                    (std::uint8_t) std::min(255., result[1]*shadow),
                    (std::uint8_t) std::min(255., result[2]*shadow));
}

VertexShader::VertexShader() {
    set_view_matrix(eye_pos, eye_up_dir, center);
    set_projection_matrix(eye_fov, aspect_ratio, z_near, z_far);
    vp = projection_matrix * view_matrix;
}

void VertexShader::set_view_matrix(const pointd& eye_pos, const vecd& eye_up_dir, const pointd& center) {
    vecd z = (eye_pos - center).normalized();
    vecd x = cross(eye_up_dir, z).normalized();
    vecd y = cross(z, x).normalized();
    
    mat4d T(true);
    T[0][3] = -eye_pos.x;
    T[1][3] = -eye_pos.y;
    T[2][3] = -eye_pos.z;
    mat4d R;
    R[0][0] = x.x, R[0][1] = x.y, R[0][2] = x.z;
    R[1][0] = y.x, R[1][1] = y.y, R[1][2] = y.z;
    R[2][0] = z.x, R[2][1] = z.y, R[2][2] = z.z;
    R[3][3] = 1.; 
    view_matrix = R*T;
}

void VertexShader::set_projection_matrix(double eye_fov, double aspect_ratio, double z_near, double z_far) {
    double n = -z_near, f = -z_far;
    double t = std::abs(n) * std::tan(eye_fov * 0.5 * PI / 360);
    double b = -t;
    double r = aspect_ratio * t;
    double l = -r;
    
    mat4d persp_to_ortho;
    persp_to_ortho[0][0] = n;
    persp_to_ortho[1][1] = n;
    persp_to_ortho[2][2] = n+f;
    persp_to_ortho[2][3] = -n*f;
    persp_to_ortho[3][2] = 1;
    mat4d ortho;
    ortho[0][0] = 2 / (r-l); ortho[0][3] = (r+l) / (l-r);
    ortho[1][1] = 2 / (t-b); ortho[1][3] = (t+b) / (b-t);
    ortho[2][2] = 1 / (n-f); ortho[2][3] = f / (f-n);
    ortho[3][3] = 1;

    projection_matrix = ortho * persp_to_ortho;
}

Vertex VertexShader::shading(const Model& model, const size_t iface, const size_t nthvert) {
    Vertex ret;
    ret.world_pos = model.model_transf(model.get_vertex(iface, nthvert));
    auto temp = vp * ret.world_pos;
    ret.screen_pos = pointd((temp.x/temp.w+1)*W*0.5, (temp.y/temp.w+1)*H*0.5, temp.z/temp.w, 1);
    // 透视纠正使用距离的关系，w 需要表示距离，为正数
    ret.w = std::abs(temp.w);
    ret.uv = model.get_uv(iface, nthvert);
    ret.normal =model.model_nm_transf(model.get_normal(iface, nthvert));
    if(!model.has_diffuse_map()) ret.texture = pointd(255, 255, 255) * 0.5;
    if(!model.has_specular_map()) ret.specular = 0;
    return ret;
}

void Light::set_light_space_matrix(vecd center, vecd up, double r) {
    vecd z = (pos - center).normalized();
    vecd x = cross(up, z).normalized();
    vecd y = cross(z, x).normalized();
    
    mat4d T(true);
    T[0][3] = -pos.x;
    T[1][3] = -pos.y;
    T[2][3] = -pos.z;
    mat4d R;
    R[0][0] = x.x, R[0][1] = x.y, R[0][2] = x.z;
    R[1][0] = y.x, R[1][1] = y.y, R[1][2] = y.z;
    R[2][0] = z.x, R[2][1] = z.y, R[2][2] = z.z;
    R[3][3] = 1.; 
    // view_matrix = R*T;

    double n = 0, f = -2*r, l = -r, t = r, b = - t;
    mat4d ortho;
    ortho[0][0] = 2 / (r-l); ortho[0][3] = (r+l) / (l-r);
    ortho[1][1] = 2 / (t-b); ortho[1][3] = (t+b) / (b-t);
    ortho[2][2] = 1 / (n-f); ortho[2][3] = f / (f-n);
    ortho[3][3] = 1;

    light_space_matrix = ortho * R * T;
}

pointd Light::get_light_space(pointd p) {
    auto temp = light_space_matrix * p;
    return pointd(temp.x/temp.w, temp.y/temp.w, temp.z/temp.w, 1);
}