#include "tgaimage.h"
#include "global.h"
#include "model.h"
#include "shader.h"
#include "rasterization.h"

static TGAImage image(W, H, TGAImage::RGB);
static TGAImage z_image(W, H, TGAImage::RGB);
static double zbuffer[W*H+1];
static double shadow_map[W*H+1];

int main() {

    for(int i=W*H; i>=0; --i) zbuffer[i] = -z_far-1;
    for(int i=W*H; i>=0; --i) shadow_map[i] = -std::numeric_limits<double>::max();

    // std::vector<std::string> model_paths = {"../obj/floor.obj"};
    // std::vector<std::string> model_paths = {"../obj/african_head/african_head.obj",
    //                                         "../obj/african_head/african_head_eye_inner.obj",
    //                                         "../obj/floor.obj"};
    std::vector<std::string> model_paths = {"../obj/diablo3_pose/diablo3_pose.obj",
                                            "../obj/floor.obj"};
    std::vector<MSRender::Light> lights = {MSRender::Light(MSRender::pointd(0,1,3,1), 10)};
    MSRender::VertexShader* vertex_shader = new MSRender::VertexShader();
    MSRender::PixelShader* pixel_shader = new MSRender::PhongShader(lights);
    std::vector<MSRender::Model> models;
    std::vector<MSRender::ModelTransfParam> modelTPs(model_paths.size());

    modelTPs[0].thetas[1] = 120.;

    std::vector<MSRender::Triangle> triangles;
    std::vector<int> model_index;

    int model_cnt = 0;
    for(const std::string& path: model_paths) {
        MSRender::Model model(path);
        model.set_model_matrix(modelTPs[model_cnt]);
        models.push_back(model);
        for(size_t i = 0; i < model.faces_size(); i++) {
            MSRender::Triangle tri;
            for(int j = 0; j < 3; j++) {
                tri.vertex[j] = vertex_shader->shading(model, i, j);
                tri.vertex[j].light_space_pos = lights[0].get_light_space(tri.vertex[j].world_pos);
                tri.vertex[j].light_space_pos.x = (tri.vertex[j].light_space_pos.x+1)*W*0.5;
                tri.vertex[j].light_space_pos.y = (tri.vertex[j].light_space_pos.y+1)*W*0.5;
            }
            triangles.push_back(tri);
            model_index.push_back(model_cnt);
            MSRender::shadow(tri, shadow_map);
        }
        model_cnt++;
    }
    for(size_t i = 0; i < triangles.size(); i++) {
        MSRender::rasterize(triangles[i], image, models[model_index[i]], pixel_shader, zbuffer, lights[0], shadow_map);
    }
    MSRender::draw_zbuffer(shadow_map, z_image, TGAColor(255,255,255));
    z_image.write_tga_file("z_out.tga");
    image.write_tga_file("output.tga");
    delete vertex_shader;
    delete pixel_shader;
    return 0;
}