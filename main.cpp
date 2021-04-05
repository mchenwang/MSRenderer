#include "tgaimage.h"
#include "vertex.h"
#include "model.h"
#include "rasterization.h"
#include "shader.h"
#include <iostream>
// #include <limits>

constexpr int W = 1200;
constexpr int H = 1200;

using namespace MSRender;

static double zbuffer[W*H+1];
static TGAImage image(W, H, TGAImage::RGB);
static Model model;

void draw_a_triangle() {
    Point2i points[3] = {Point2i({0, 0}), Point2i({300, 150}), Point2i({0,300})};
    triangle2d(points, image, TGAColor(255, 255, 255));
}

void draw_model() {
    model = Model("../obj/african_head/african_head.obj");
    Vector3d light_dir({0., 0., -1.});
    const TGAColor color(255, 255, 255);
    for(size_t i = 0; i < model.faces_size(); i++) {
        Point3d screen_coords[3];
        Point3d world_coords[3];
        for(int j = 0; j < 3; j++) {
            Point3d p = model.get_vertex(i, j);
            screen_coords[j] = Vector3d({(p[0]+1)*W*0.5, (p[1]+1)*H*0.5, p[2]});
            world_coords[j] = p;
        }
        Vector3d n = cross(world_coords[1] - world_coords[0], world_coords[2] - world_coords[0]);
        n.normalize();
        double intensity = - (n*light_dir);
        if(intensity > 0){
            triangle(screen_coords, zbuffer, image, color*intensity);
        }
    }
    // draw_zbuffer(zbuffer, image, color);
}

void draw_model_with_texture() {
    model = Model("../obj/african_head/african_head.obj");
    Vector3d light_dir({0., 0., -1.});
    const TGAColor color(255, 255, 255);
    for(size_t i = 0; i < model.faces_size(); i++) {
        Point3d screen_coords[3];
        Point3d world_coords[3];
        Point2d uvs[3];
        for(int j = 0; j < 3; j++) {
            Point3d p = model.get_vertex(i, j);
            screen_coords[j] = Vector3d({(p[0]+1)*W*0.5, (p[1]+1)*H*0.5, p[2]});
            world_coords[j] = p;
            uvs[j] = model.get_uv(i, j);
        }
        Vector3d n = cross(world_coords[1] - world_coords[0], world_coords[2] - world_coords[0]);
        n.normalize();
        double intensity = - (n*light_dir);
        if(intensity > 0){
            triangle_with_texture(screen_coords, uvs, zbuffer, image, model.get_diffusemap(), intensity);
        }
    }
}

void draw_mvp() {
    model = Model("../obj/african_head/african_head.obj");
    // const Point3d eye({0., 0.3, 2.});
    const Point3d eye({1., 0.5, 1.});
    const Vector3d eye_up_dir({0., 1., 0.});
    const Point3d center({0., 0., 0.});
    const double scale[] = {1, 1., 1.};
    const double thetas[] = {0., 0., 0.};
    const Vector3d translate({0., 0., 0.});

    Eigen::Matrix4d mvp = projection_transf(90., 1.0, -1., -3.) * view_transf(eye, eye_up_dir, center) * model_transf(scale, thetas, translate);
    Vector3d light_dir({0., 0., -1.});
    light_dir.normalize();
    const TGAColor color(255, 255, 255);
    for(size_t i = 0; i < model.faces_size(); i++) {
        Point3d screen_coords[3];
        Point3d world_coords[3];
        Point2d uvs[3];
        for(int j = 0; j < 3; j++) {
            Point3d p = model.get_vertex(i, j);
            Eigen::Vector4d temp = mvp * Eigen::Vector4d(p[0], p[1], p[2], 1.);
            // 视口变换
            screen_coords[j] = Vector3d({(temp[0]/temp[3]+1)*W*0.5, (temp[1]/temp[3]+1)*H*0.5, temp[2]/temp[3]});
            world_coords[j] = Point3d({temp[0]/temp[3], temp[1]/temp[3], temp[2]/temp[3]});
            // world_coords[j] = p;
            uvs[j] = model.get_uv(i, j);
        }
        Vector3d n = cross(world_coords[1] - world_coords[0], world_coords[2] - world_coords[0]);
        n.normalize();
        double intensity = - (n*light_dir);
        if(intensity > 0){
            triangle_with_texture(screen_coords, uvs, zbuffer, image, model.get_diffusemap(), intensity);
        }
    }
}

void draw_with_shading(std::vector<std::string> models) {
    // const Point3d eye({0., 0.3, 2.});
    const Vector3d eye_up_dir({0., 1., 0.});
    const Point3d eye({0.7, 1, 1.9});
    // const Point3d eye({0., 2., 0.});
    // const Vector3d eye_up_dir({0., 0., -2.});
    const Point3d center({0., 0., 0.});
    // const double scale[] = {1.3, 1.3, 1.3};
    const double scale[] = {1., 1., 1.};
    const double thetas[] = {0., 0., 0.};
    const Vector3d translate({0., 0., 0.});

    const double eye_fov = 90.;
    const double aspect_ratio = 1.0;
    const double z_near = 0.1;
    const double z_far  = 5.;

    Eigen::Matrix4d mvp = projection_transf(eye_fov, aspect_ratio, -z_near, -z_far) * view_transf(eye, eye_up_dir, center) * model_transf(scale, thetas, translate);
    // Eigen::Matrix4d vp = view_transf(eye, eye_up_dir, center) * model_transf(scale, thetas, translate);
    // double light_intensity = 6000.;
    std::vector<Light> lights{{Point3d({-1., 2., 1.}), 0.05},
                              {Point3d({3., 0.5, 2.}), 0.05}};
    // std::vector<Light> lights{{Point3d({0., 2., 0.}), 0.01}};

    double amb_light_intensity = 15.;
    PhongShader* phong_shader = new PhongShader(eye, amb_light_intensity, lights);

    for(auto& model_path: models) {
        model = Model(model_path);
        for(size_t i = 0; i < model.faces_size(); i++) {
            Fragment fragments[3];
            for(size_t j = 0; j < 3; j++) {
                Point3d p = model.get_vertex(i, j);
                fragments[j].world_pos = p;
                Eigen::Vector4d temp = mvp * Eigen::Vector4d(p[0], p[1], p[2], 1.);
                fragments[j].w = temp[3];
                fragments[j].screen_pos = Vector3d({(temp[0]/temp[3]+1)*W*0.5, (temp[1]/temp[3]+1)*H*0.5, temp[2]/temp[3]});
                fragments[j].normal = model.get_normal(i, j).normalized();
                fragments[j].uv  = model.get_uv(i, j);
            }
            triangle_with_Phong(fragments, phong_shader, image, zbuffer, model);
        }
    }
    delete phong_shader;
    phong_shader = NULL;
}

int main()
{
    for(int i = W*H; i >= 0; i--) zbuffer[i] = -1.1;
    // #pragma omp parallel for
    // draw_a_triangle();
    // draw_model();
    // draw_model_with_texture();
    // draw_mvp();
    // std::vector<std::string> models = {"../obj/african_head/african_head.obj"};
    // std::vector<std::string> models = {"../obj/floor.obj","../obj/african_head/african_head.obj"};
    std::vector<std::string> models = {"../obj/african_head/african_head_eye_inner.obj",
    "../obj/african_head/african_head.obj",
    "../obj/floor.obj"};
    // std::vector<std::string> models = {"../obj/diablo3_pose/diablo3_pose.obj","../obj/floor.obj"};
    // std::vector<std::string> models = {"../obj/floor.obj"};
    draw_with_shading(models);
    // draw_zbuffer(zbuffer, image, TGAColor(255,255,255));
    image.write_tga_file("output.tga");
    return 0;
}