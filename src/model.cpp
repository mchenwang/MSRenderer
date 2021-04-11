#include <iostream>
#include <fstream>
#include <sstream>
#include "model.h"
#include "global.h"

using namespace MSRender;

bool Model::nm_is_in_tangent = true;

Model::Model(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if(in.fail()) {
        std::cerr << "cannot load the model\n";
        return ;
    }
    std::string line;
    std::string _; // 过滤字符串
    while(!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line);
        if(!line.compare(0, 2, "v ")) {
            pointd v;
            iss >> _ >> v.x >> v.y >> v.z;
            v.w = 1;
            vertices.push_back(v);
        }
        else if(!line.compare(0, 3, "vt ")) {
            uvd vt;
            iss >> _ >> vt.u >> vt.v;
            uvs.push_back(vt);
        }
        else if(!line.compare(0, 3, "vn ")) {
            vecd vn;
            iss >> _ >> vn.x >> vn.y >> vn.z;
            vn.w = 0;
            normals.push_back(vn);
        }
        else if(!line.compare(0, 2, "f ")) {
            int v, t, n, cnt = 0;
            char c;
            iss >> c;
            while(iss >> v >> c >> t >> c >> n) {
                face_vertices.push_back(v-1);
                face_uvs.push_back(t-1);
                face_normal.push_back(n-1);
                cnt++;
            }
            if (cnt != 3) {
                std::cerr << "Error: the obj file is supposed to be triangulated\n";
                in.close();
                return;
            }
        }
    }
    in.close();
    has_diffusemap = load_texture(filename, "_diffuse.tga",    diffusemap_);
    has_normalmap = load_texture(filename, (Model::nm_is_in_tangent? "_nm_tangent.tga":"_nm.tga"), normalmap_);
    has_specularmap = load_texture(filename, "_spec.tga",       specularmap_);
}

size_t Model::vertexs_size() const {
    return vertices.size();
}

size_t Model::faces_size() const {
    return face_vertices.size() / 3;
}

pointd Model::get_vertex(const size_t iface, const size_t nthvert) const {
    if(iface*3+nthvert >= face_normal.size()) return pointd(0., 0., 0., 1.);
    return vertices[face_vertices[iface*3+nthvert]];
}

bool Model::load_texture(std::string filename, const std::string suffix, TGAImage &img) {
    size_t dot = filename.find_last_of(".");
    if (dot==std::string::npos) return false;
    std::string texfile = filename.substr(0,dot) + suffix;
    bool flag = img.read_tga_file(texfile.c_str());
    std::cerr << "texture file " << texfile << " loading " << (flag ? "ok" : "failed") << std::endl;
    img.flip_vertically();
    return flag;
}

vecd Model::get_normal_with_map(const uvd &uv) const {
    TGAColor c = normalmap_.get(uv.u*normalmap_.get_width(), uv.v*normalmap_.get_height());
    vecd res;
    for (int i=0; i<3; i++)
        res[2-i] = (c[i] * 2. / 255.) - 1;
    return res;
}
vecd Model::get_diffuse(const uvd &uv) const {
    TGAColor c = diffusemap_.get(uv.u*diffusemap_.get_width(), uv.v*diffusemap_.get_height());
    return vecd(c[2], c[1], c[0]);
}
double Model::get_specular(const uvd &uv) const {
    return specularmap_.get(uv.u*specularmap_.get_width(), uv.v*specularmap_.get_height())[0];
}

vecd Model::get_normal_with_map(const double uv0, const double uv1) const {
    TGAColor c = normalmap_.get(uv0*normalmap_.get_width(), uv1*normalmap_.get_height());
    vecd res;
    for (int i=0; i<3; i++)
        res[2-i] = (c[i] * 2. / 255.) - 1;
    return res;
}
vecd Model::get_diffuse(const double uv0, const double uv1) const {
    TGAColor c = diffusemap_.get(uv0*diffusemap_.get_width(), uv1*diffusemap_.get_height());
    return vecd(c[2], c[1], c[0]);
}
// void Model::set_diffuse(const double uv0, const double uv1) {
//     return diffusemap_.set(uv0*diffusemap_.get_width(), uv1*diffusemap_.get_height(), get_diffuse(uv0, uv1)*0.3);
// }
double Model::get_specular(const double uv0, const double uv1) const {
    return specularmap_.get(uv0*specularmap_.get_width(), uv1*specularmap_.get_height())[0];
}

uvd Model::get_uv(const size_t iface, const size_t nthvert) const {
    if(iface*3+nthvert >= face_normal.size()) return uvd(0., 0.);
    return uvs[face_uvs[iface*3+nthvert]];
}

vecd Model::get_normal(const size_t iface, const size_t nthvert) const {
    if(iface*3+nthvert >= face_normal.size()) return vecd(0., 0., 0.);
    return normals[face_normal[iface*3+nthvert]];
}

const TGAImage& Model::get_diffusemap() const {
    return diffusemap_;
}
const TGAImage& Model::get_normalmap() const {
    return normalmap_;
}
const TGAImage& Model::get_specularmap() const {
    return specularmap_;
}

void Model::set_model_matrix(const ModelTransfParam& param) {
    mat4d Scale;
    Scale[0][0] = param.scale[0];
    Scale[1][1] = param.scale[1];
    Scale[2][2] = param.scale[2];
    Scale[3][3] = 1;
    double cosx = std::cos(param.thetas[0]*PI/180.);
    double sinx = 1. - cosx * cosx;
    double cosy = std::cos(param.thetas[1]*PI/180.);
    double siny = 1. - cosy * cosy;
    double cosz = std::cos(param.thetas[2]*PI/180.);
    double sinz = 1. - cosz * cosz;
    mat4d Rotate;
    Rotate[0] = vecd(cosy*cosz, -cosy*sinz, siny, 0);
    Rotate[1] = vecd(sinx*siny*cosz+cosx*sinz, -sinx*siny*sinz+cosx*cosz, -sinx*cosy, 0);
    Rotate[2] = vecd(-cosx*siny*cosz+sinx*sinz, cosx*siny*sinz+sinx*cosz, cosx*cosy, 0);
    Rotate[3][3] = 1;
    mat4d Translate(true);
    Translate[0][3] = param.translate[0];
    Translate[1][3] = param.translate[1];
    Translate[2][3] = param.translate[2];
    model_matrix = Translate * Rotate * Scale;
}

pointd Model::model_transf(pointd&& p) const {
    return model_matrix * p;
}