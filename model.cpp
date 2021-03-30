#include <iostream>
#include <fstream>
#include <sstream>
#include "model.h"

using namespace MSRender;

Model::Model(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if(in.fail()) {
        std::cerr << "cannot load the model\n";
        return ;
    }
    std::string line;
    std::string _; // 过滤字符串
    std::vector<Point2d> vts;
    std::vector<Vector3d> vns;
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
            vts.push_back(vt);

            uvs.push_back(vt);
        }
        else if(!line.compare(0, 3, "vn ")) {
            Vector3d vn;
            iss >> _ >> vn[0] >> vn[1] >> vn[2];
            vns.push_back(vn.normalize());
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
    load_texture(filename, "_diffuse.tga",    diffusemap_);
    // load_texture(filename, "_nm_tangent.tga", normalmap_);
    // load_texture(filename, "_spec.tga",       specularmap_);
}

int Model::vertexs_size() const {
    return vertices.size();
}

int Model::faces_size() const {
    return face_vertices.size() / 3;
}

Point3d Model::get_vertex(const int iface, const int nthvert) const {
    return vertices[face_vertices[iface*3+nthvert]];
}

void Model::load_texture(std::string filename, const std::string suffix, TGAImage &img) {
    size_t dot = filename.find_last_of(".");
    if (dot==std::string::npos) return;
    std::string texfile = filename.substr(0,dot) + suffix;
    std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
    img.flip_vertically();
}

TGAColor Model::get_diffuse(const Point2d &uvf) const {
    return diffusemap_.get(uvf[0]*diffusemap_.get_width(), uvf[1]*diffusemap_.get_height());
}

Vector3d Model::get_normal(const Point2d &uvf) const {
    TGAColor c = normalmap_.get(uvf[0]*normalmap_.get_width(), uvf[1]*normalmap_.get_height());
    Vector3d res;
    for (int i=0; i<3; i++)
        res[2-i] = c[i]/255.*2 - 1;
    return res;
}

double Model::get_specular(const Point2d &uvf) const {
    return specularmap_.get(uvf[0]*specularmap_.get_width(), uvf[1]*specularmap_.get_height())[0];
}

Point2d Model::get_uv(const int iface, const int nthvert) const {
    return uvs[face_uvs[iface*3+nthvert]];
}

Vector3d Model::get_normal(const int iface, const int nthvert) const {
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