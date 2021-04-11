#include <iostream>
#include <fstream>
#include <sstream>
#include "model.h"

using namespace MSRender;

bool Model::nm_is_in_tangent = false;

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
            Point3d v;
            iss >> _ >> v[0] >> v[1] >> v[2];
            vertices.push_back(v);
        }
        else if(!line.compare(0, 3, "vt ")) {
            Point2d vt;
            iss >> _ >> vt[0] >> vt[1];
            uvs.push_back(vt);
        }
        else if(!line.compare(0, 3, "vn ")) {
            Vector3d vn;
            iss >> _ >> vn[0] >> vn[1] >> vn[2];
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

Point3d Model::get_vertex(const size_t iface, const size_t nthvert) const {
    if(iface*3+nthvert >= face_normal.size()) return Point3d({0., 0., 0.});
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

Vector3d Model::get_normal_with_map(const Point2d &uvf) const {
    TGAColor c = normalmap_.get(uvf[0]*normalmap_.get_width(), uvf[1]*normalmap_.get_height());
    Vector3d res;
    for (int i=0; i<3; i++)
        res[i] = (double)c[i]/255.*2 - 1;
    return res;
}
TGAColor Model::get_diffuse(const Point2d &uvf) const {
    return diffusemap_.get(uvf[0]*diffusemap_.get_width(), uvf[1]*diffusemap_.get_height());
}
double Model::get_specular(const Point2d &uvf) const {
    return specularmap_.get(uvf[0]*specularmap_.get_width(), uvf[1]*specularmap_.get_height())[0];
}

Vector3d Model::get_normal_with_map(const double uv0, const double uv1) const {
    TGAColor c = normalmap_.get(uv0*normalmap_.get_width(), uv1*normalmap_.get_height());
    Vector3d res;
    for (int i=0; i<3; i++)
        res[i] = (double)c[i]/255.*2 - 1;
    return res;
}
TGAColor Model::get_diffuse(const double uv0, const double uv1) const {
    return diffusemap_.get(uv0*diffusemap_.get_width(), uv1*diffusemap_.get_height());
}
void Model::set_diffuse(const double uv0, const double uv1) {
    return diffusemap_.set(uv0*diffusemap_.get_width(), uv1*diffusemap_.get_height(), get_diffuse(uv0, uv1)*0.3);
}
double Model::get_specular(const double uv0, const double uv1) const {
    return specularmap_.get(uv0*specularmap_.get_width(), uv1*specularmap_.get_height())[0];
}

Point2d Model::get_uv(const size_t iface, const size_t nthvert) const {
    if(iface*3+nthvert >= face_normal.size()) return Point2d({0., 0.});
    return uvs[face_uvs[iface*3+nthvert]];
}

Vector3d Model::get_normal(const size_t iface, const size_t nthvert) const {
    if(iface*3+nthvert >= face_normal.size()) return Vector3d({0., 0., 0.});
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