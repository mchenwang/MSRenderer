# SoftwareRendering 说明
一个简单的软渲染器。

重要的公式推导过程记录在[这里](https://github.com/miawua/MSRenderer/wiki)。

参考：

-  [tinyrendering](https://github.com/ssloy/tinyrenderer/wiki/Lesson-0:-getting-started) 
- GAMES 101

>  2021/4/11 完成重构，原始版本保留在 .temp 文件夹中

## 运行

- Win10 + MinGW

```sh
git clone https://github.com/miawua/MSRenderer.git
cd MSRenderer
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make && renderer.exe && start output.tga
```

- Windows 系统下查看 tga 格式的图片可使用软件 [IrfanView](https://www.irfanview.com/)

## 过程记录

### Step 1 准备工作

#### 图片输出

为查看渲染结果，首先需要将数字信息转换成像素输出成图片形式。简单起见，使用 tinyrendering 项目中的 `TGAImage` 类与 `TGAColor` 类（在 tgaimage.h/tgaimage.cpp 文件中）。

使用如下代码进行测试：

```c++
#include "tgaimage.h"
constexpr int W = 100; // 图片宽
constexpr int H = 100; // 图片高
int main()
{
    TGAImage image(W, H, TGAImage::RGB); // 创建图片
    for (int i = 0; i < W / 2; i++) {
        for (int j = 0; j < H / 2; j++) {
            image.set(i, j, TGAColor(0, 255, 0)); // 绿
            image.set(i + W / 2, j, TGAColor(255, 0, 0)); // 红
            image.set(i, j + H / 2, TGAColor(0, 0, 0)); // 黑
            image.set(i + W / 2, j + H / 2, TGAColor(0, 0, 255)); //蓝
        }
    }
    image.write_tga_file("output.tga");
    return 0;
}
```

上述代码构建了一个 100*100 的窗口，并且分为四分，涂上不同的颜色，结果如下：

<img src="/img/colortga.jpg" style="width:100px;" />

由结果可知，`TGAImage` 类构建出图片的坐标原点在左下角。为使图片原点坐标符合常见的左上角规定，可以在写入文件之前使用 `image.flip_vertically()` 将图片进行翻转，从而得到原点坐标在左上角的图片。

> 需要注意的是，TGAColor 的实现是以 BGR 的顺序存储颜色值的，在后期工作中需要注意不同颜色通道的顺序。

到此，可以认为输出不再是问题，接下来需要解决怎样输入的问题。

#### 基本数据结构

渲染之前需要载入模型，那么模型存储是首要任务，下面简单设计几个必备的数据结构：顶点、向量、矩阵、模型。

```c++
template<typename T, size_t n> struct Point {
    std::array<T, n> data = {};
    Point() = default;
    Point(const std::array<T, n>& data_) noexcept ;
    Point(const Point<T, n>& p) noexcept ;
    Point<T, n>& operator=(const Point<T, n>& p) noexcept ;

    T operator[](const size_t i) const;
    T& operator[](const size_t i);
    
    Point<T, n> operator+(const Point<T, n>& b) const ;
    Point<T, n> operator-(const Point<T, n>& b) const ;
};
template<typename T, size_t n> struct Vector: Point<T, n> {};
```

简单测试一下：

```c++
for (int i = 0; i < W; i += 10)
    for (int j = 0; j < H; j += 10) {
        Point2i a({i, j});
        image.set(a[0], a[1], TGAColor(255, 255, 255));
    }
```

得到如下结果：

<img src="/img/points.jpg" style="width:200px;" />

矩阵直接使用 `Eigen` 库，将 `Eigen` 添加到 include 目录下，在 CMakeLists.txt 中添加 `INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/include)` 即可使用。

模型对象是用于导入 3D obj 文件信息，如：顶点、三角形、法向、纹理坐标等信息，这里继续使用 tinyrendering 中的代码，稍作修改：

```c++
class Model {
private:
    std::vector<Point3d> vertices;
    std::vector<Point2d> uvs;
    std::vector<Vector3d> normals;
    std::vector<int> face_vertices;
    std::vector<int> face_uvs;
    std::vector<int> face_normal;
    TGAImage diffusemap_;         // diffuse color texture
    TGAImage normalmap_;          // normal map texture
    TGAImage specularmap_;        // specular map texture
};
```

> 值得一提的是，模型中的一个顶点可能属于多个三角形，而在不同三角形中的 uv 坐标和法向是不同，因此需要分开存储；而我在设计之初，是将顶点坐标、uv 坐标和法向封装成了一个 Vertex 类，导致后续加入贴图后，出现了问题，可见下图：
>
> <img src="/img/texturebug.png"  height="240px" width="240px" />

模型从 obj 文件以及相应的 tga 文件中导入，具体实现可见源码。

到这里，可以说是解决了软渲染器的输入和输出，接下来开始一步一步完成渲染的各个步骤。

### Step 2 画三角形

先从最简单的画三角形开始。

采用扫描法，逐像素判断是否在三角形内：

```c++
void triangle(Point2i* points, TGAImage &image, TGAColor color) {
    Point2i bboxmin({image.get_width()-1,  image.get_height()-1});
    Point2i bboxmax({0, 0});
    Point2i clamp({image.get_width()-1, image.get_height()-1});
    // 先求出三角形的包围盒
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0,        std::min(bboxmin[j], points[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], points[i][j]));
        }
    }
    Point2i P;
    // 对包围盒内的像素判断是否在三角形内
    for (P[0]=bboxmin[0]; P[0]<=bboxmax[0]; P[0]++) {
        for (P[1]=bboxmin[1]; P[1]<=bboxmax[1]; P[1]++) {
            if (!in_triangle(points, P)) continue;
            image.set(P[0], P[1], color);
        }
    }
}
```

其中，判断点是否在三角形内可以使用向量叉积。由叉积定义可知，两个向量叉积的结果向量垂直于这两个向量组成的平面，而在右手系中，用右手定则可以判断结果向量的方向。那么给定三角形 $\triangle ABC$ 和点 $O$，如果向量 $\vec{AB}\times \vec{AO}$、 $\vec{BC}\times \vec{BO}$、 $\vec{CA}\times \vec{CO}$ 的方向相同，则点在三角形内。

简单测试一下：

```c++
Point2i points[3] = {Point2i({280,30}), Point2i({10, 10}), Point2i({150, 280})};
triangle(points, image, TGAColor(255, 255, 255));
image.flip_vertically();
image.write_tga_file("output.tga");
```

得到：

<img src="/img/triangle.jpg"  height="240px" width="240px" />

仔细观察，可以发现三角形的边界并不平滑，有走样现象：

<img src="/img/aliasing.png"  height="240px" width="240px">

可以采用 MSAA 的方法来减轻走样现象：

```c++
for (P[0]=bboxmin[0]; P[0]<=bboxmax[0]; P[0]++) {
    for (P[1]=bboxmin[1]; P[1]<=bboxmax[1]; P[1]++) {
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
    }
}
```

得到结果：

<img src="/img/triangleMSAA.jpg"  height="240px" width="240px" /><img src="/img/anti-aliasingMSAA.png"  height="240px" width="240px">

除了使用叉积判断像素在不在三角形内外，还可以使用重心坐标的方法，如果一个点的重心坐标 $(1-u-v,u, v)$ 都不小于 0，则该点在三角形内。
实际上，根据重心坐标定义，可以列出如下线性方程组，从而解出重心坐标：

$$
\begin{array}{c}
        u\vec{AB}_x+v\vec{AC}_x+\vec{PA}_x=0 \\ 
        u\vec{AB}_y+v\vec{AC}_y+\vec{PA}_y=0 
    \end{array}
$$

```c++
Vector3d barycentric(Point3d A, Point3d B, Point3d C, Point3d P) {
    Vector3d x({B[0]-A[0], C[0]-A[0], A[0]-P[0]});
    Vector3d y({B[1]-A[1], C[1]-A[1], A[1]-P[1]});
    double u = (x[1]*y[2] - x[2]*y[1]) / (x[0]*y[1] - x[1]*y[0]);
    double v = (x[0]*y[2] - x[2]*y[0]) / (x[1]*y[0] - x[0]*y[1]);
    return Vector3d({1.-u-v, u, v});
}
```

那么绘制三角形的代码即为：

```c++
Vector3d bc_screen  = barycentric(points[0], points[1], points[2], P);
if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
image.set(P[0], P[1], color);
```

> 实际上，算一个像素点是否在三角形内，往往取像素点的中心，即在计算时坐标 (x,y) 应该为 (P[0]+0.5,P[1]+0.5)

### Step 3 绘制模型

obj 文件中模型由很多三角形组成，因此，只要分别画出每个三角形，即可绘制出模型。当然其中涉及到坐标从三维空间到二维屏幕的映射。

这里先明确几个概念：绘制单个模型时，模型坐标即为世界坐标；世界坐标轴为右手系，从垂直屏幕朝外的方向为 z 轴正方向，x 轴正方向朝右；三角形点逆时针排列为正方向。

此时，如果观察模型的正面，将 z 轴坐标去掉，就可以得到二维空间的点的坐标，而 obj 文件中坐标的值的范围在 $[-1,1]$，因此，需要将坐标映射到屏幕上: $[0,W-1][0,H-1]$。

```c++
Point3d p = model.get_point(i, j);
screen_coords[j] = Vector3d({(p[0]+1)*W*0.5, (p[1]+1)*H*0.5, p[2]});
world_coords[j] = p;
```

为了体现立体感，需要给不同朝向的三角面片画上不同深浅的颜色，规定一束朝 z 轴负向的光；

```c++
Vector3d light_dir({0., 0., -1.});
```

三角形的朝向可以用法线方向来判断，根据三角形两条边叉乘可以求出法线，如果法线的 z 值为正，则表示可以看到该三角形，否则，不能看到；并且归一化后 z 值的大小决定与光线的夹角，越大，则越正对光照，应该越亮，反之越暗：

```c++
Vector3d n = cross(world_coords[1] - world_coords[0], world_coords[2] - world_coords[0]);
n.normalize(); // 计算法线需要根据世界坐标计算，屏幕坐标的x\y被拉伸了
double intensity = - (n*light_dir);
if(intensity > 0) triangle(screen_coords, image, color*intensity);
```

结果如下：

<img src="/img/face1.jpg"  height="240px" width="240px" />

这里不难发现，模型嘴部等几个地方出现了异样，因为上述做法是对三角形顺序遍历，逐一绘制，而当三角形的顺序不是按照画家算法的顺序存储，或者三角形存在交叉的情况时，就会出现错误的覆盖现象。

解决这个问题需要用到 z-buffer，即不再以三角形为单位绘制，而是以像素为单位，逐个计算像素的深度值，深度大（z 大）的覆盖深度小的。三角形中间的像素深度值，通过三角形重心坐标进行差值计算。

```c++
Vector3d bc_screen  = barycentric(points[0], points[1], points[2], P);
if (bc_screen[0]<0 || bc_screen[1]<0 || bc_screen[2]<0) continue;
double z = 0.;
for (int i=0; i<3; i++) z += points[i][2]*bc_screen[i];
if (zbuffer[P[0]+P[1]*image.get_width()] < z) {
    zbuffer[P[0]+P[1]*image.get_width()] = z;
    image.set(P[0], P[1], color);
}
```

结果如下：

<img src="/img/facez-buffer.jpg"  height="240px" width="240px" />

将 z-buffer 可视化绘制一下：

<img src="/img/z-buffer.jpg"  height="240px" width="240px" />

当然，重心坐标插值还可以做更多的事情，比如加入纹理，利用 uv 坐标插值，找到像素点在纹理贴图中的颜色，只要对之前的代码稍加修改，向函数中传入纹理贴图即可，uv 坐标的插值与 z 的插值相同：

```c++
Vector3d bc_screen  = barycentric(points[0], points[1], points[2], P);
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
```

结果如下：

<img src="/img/texture.jpg"  height="240px" width="240px" />

### Step 4 变换观察视角

在这里开始，进一步明确几个概念：世界坐标，右手系，左下角为原点，屏幕朝外是 z 轴正方向，朝右是 x 轴正方向；模型坐标，即 obj 文件中记录的顶点坐标构成的坐标系；相机坐标\相机视角，即相机在世界坐标的什么位置从什么方向观察世界，决定了最后图片呈现的内容以及方向；模型变换，将模型在模型坐标下进行平移旋转和缩放；视图变换，将相机摆放到某个位置，为了简便计算，约定将相机的位置变换为世界坐标的原点；投影变换，将 3D 模型投影到平面上，上述直接去掉 z 轴的方式为正交投影变换，虽然是固定的，接下来将要实现的是透视投影变换，根据视锥将看到的内容投影到平面上。

先摆物体，包括平移、旋转、缩放：

```c++
Eigen::Matrix4d Scale; // 缩放
Scale << scale[0], 0., 0., 0.,
         0., scale[1], 0., 0.,
         0., 0., scale[2], 0.,
         0., 0., 0., 1.;
Eigen::Matrix4d Rotate;
double cosx = std::cos(thetas[0]*PI/180.);
double sinx = std::sin(thetas[0]*PI/180.);
double cosy = std::cos(thetas[1]*PI/180.);
double siny = std::sin(thetas[1]*PI/180.);
double cosz = std::cos(thetas[2]*PI/180.);
double sinz = std::sin(thetas[2]*PI/180.);
Eigen::Matrix4d RotateX; // 绕 x 轴旋转
RotateX << 1., 0., 0., 0.,
           0., cosx, -sinx, 0.,
           0., sinx, cosx, 0.,
           0., 0., 0., 1.;
Eigen::Matrix4d RotateY; // 绕 y 轴旋转
RotateY << cosy, 0., siny, 0.,
           0., 1., 0., 0.,
           -siny, 0., cosy, 0.,
           0., 0., 0., 1.;
Eigen::Matrix4d RotateZ; // 绕 z 轴旋转
RotateZ << cosz, -sinz, 0., 0.,
           sinz, cosz, 0., 0.,
           0., 0., 1., 0.,
           0., 0., 0., 1.;
Rotate = RotateX * RotateY * RotateZ;
Eigen::Matrix4d Translate; // 平移
Translate << 1., 0., 0., translate[0],
             0., 1., 0., translate[1],
             0., 0., 1., translate[2],
             0., 0., 0., 1.;
```

这里模型的旋转是相对自身的，因此需要先做旋转变换，`Translate*Scale*Rotate` 即为模型变换矩阵。再摆相机，此时相机位置随意设置，包括相机看向的方向以及朝上的方向：

```c++
// 求出相机坐标系
Vector3d z = (eye - center).normalize();
Vector3d x = cross(eye_up_dir, z).normalize();
Vector3d y = cross(z, x).normalize();
Eigen::Matrix4d T; // 平移到原点
T << 1., 0., 0., -eye[0],
     0., 1., 0., -eye[1],
     0., 0., 1., -eye[2],
     0., 0., 0., 1.;
Eigen::Matrix4d R; // 旋转到相机坐标系与世界坐标系重合
// g = -z 相机看向的方向是 z 轴负向
R << x[0], x[1], x[2], 0.,
     y[0], y[1], y[2], 0.,
     z[0], z[1], z[2], 0.,
     0.  , 0.  , 0.  , 1.;
```

先做平移变换再旋转，因此 `R*T` 即为视图变换的矩阵。最后将相机能够拍到的地方投影到平面上：

```c++
// 需要视锥的参数，这里是相机变换到原点后的视锥，z_near/z_far 分别是相机能看到最近和最远的平面 z 轴值；
// eye_fov 是视锥切面的顶点角度
// aspect_ratio 即视锥长宽比
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
return scale * translate * persp_to_ortho;
```

下面初始化一些参数

```c++
const Point3d eye({1., 0.5, 1.}); // 相机初始位置
const Vector3d eye_up_dir({0., 1., 0.}); // 相机朝上的位置，不必垂直，后面可根据叉乘算出相机坐标系
const Point3d center({0., 0., 0.}); // 相机看向的点
const double scale[] = {1., 1., 1.};
const double thetas[] = {0., 0., 0.};
const Vector3d translate({0., 0., 0.});
Eigen::Matrix4d mvp = projection_transf(90., 1.0, -1., -3.) * 
                      view_transf(eye, eye_up_dir, center) *
                      model_transf(scale, thetas, translate);
```

当然，在光栅化时传入的点的坐标需要改为变换后的：

```c++
Eigen::Vector4d temp = mvp * Eigen::Vector4d(p[0], p[1], p[2], 1.);
// 视口变换
screen_coords[j] = Vector3d({(temp[0]/temp[3]+1)*W*0.5, (temp[1]/temp[3]+1)*H*0.5, temp[2]/temp[3]});
world_coords[j] = Point3d({temp[0]/temp[3], temp[1]/temp[3], temp[2]/temp[3]});
```

这里包括之前都以及用到了视口变换，即模型的坐标是在 $[-1,1]^3$ 空间下的，透视投影后的坐标是在 $[-1,1]^2$ 平面内的，现在需要把平面扩展到平面空间，即为视口变换。

下面看看结果：

<img src="/img/mvp.jpg"  height="240px" width="240px" />

试着修改一些参数，比如绕 z 轴旋转 90°：

<img src="/img/mvp2.jpg"  height="240px" width="240px" />

### Step 5 光照模型

实际上，之前的步骤中也使用了光照，不过是平行光。

接下来的光照模型（Blinn-Phong 模型）将会使用点光源和全局光。

Blinn-Phong 模型以及 Phong 模型根据经验，认为照在物体上的光由三部分组成：全局光照、漫反射光、镜面反射光。

全局光照是为了保证模型在无光处不至于全黑，只需要设定一个全局光照强度和一个全局光照系数即可：

```c++
la = k_a * amb_light_intensity;
```

镜面反射光决定了物体的高光，而根据经验来看，当光线可以通过反射，到达人眼时，即可看到高光，而反射光与视线的夹角越小，高光强度越大，光强随光源距离的平方衰减：

```c++
ls = ks * light.intensity / r2 * cos(theta);
```

其中 `theta` 的值即为反射光线与视线的夹角，也等于该点的法线与半程向量（视线+光线）的夹角，同时为了让高光保持在一个小区域，可以给 cos 加个指数，然后公式即为：

```c++
ls = ks*light.intensity / r2 * (fragment.normal * (light_dir + eye_dir).normalized())^p;
```

漫反射系数在有纹理的情况下，即为纹理的颜色，公式如下：

```c++
ld = kd * light.intensity / r2 * (fragment.normal * light_dir.normalized());
```

最后结果为三个部分的累加，颜色即为结果再乘 255 将数据映射到颜色范围中。可看到结果如下：

<img src="/img/BPhong.jpg"  height="240px" width="240px"><img src="/img/BPhong2.jpg"  height="240px" width="240px">

另外，我们增加一个地板试试看：

<img src="/img/perspectiveerror.jpg"  height="240px" width="240px">

这里似乎出现了问题，地板上的纹理并不合理，这里是因为在设置纹理时，我们使用屏幕空间的坐标对点及其属性进行插值，而根据第四步中得到的 MVP 变换后的点坐标在 x 和 y 轴坐标上是有线性关系的，而在 z 轴上是对 1/z 有线性关系，因此进行线性插值时，会出现错误，因此需要矫正：透视矫正。根据公式：

```c++
double zt = bc_screen[0] / fragments[0].w + bc_screen[1] / fragments[1].w + bc_screen[2] / fragments[2].w;
alpha = bc_screen[0] / (zt*fragments[0].w);
beta = bc_screen[1] / (zt*fragments[1].w);
gamma = bc_screen[2] / (zt*fragments[2].w);
```

结果如下：

<img src="/img/perspectivecorrect.jpg"  height="240px" width="240px">

实际上到这里，关于 z 值的插值仍然有问题，设点 $P(x,y,z,1)$ 经过 MVP 变换后为 $P'(x',y',z',1)$；则：其中 $z'=\frac{(n+f)}{n-f}-\frac{2nf}{n-f}\cdot\frac{1}{z}$，因此 $z'$ 的值关于 $-\frac{1}{z}$ 成正比，所以可以使用上述代码中的 -zt 作为新的 z 值更新 zbuffer。

另外，表现一个物体的材质只用纹理是完全不够的，高光的程度是另一个表现手段，如金属的表面往往更像镜面，同一方向的反射光往往更多，而人皮肤的表面往往更粗糙，镜面反射的光应该更少。

这时应该通过高光贴图来表现这些特点。高光贴图中，颜色越接近白色，镜面反射的光应该更多，越是黑色，镜面反射的光应该少。因此，将之前镜面反射公式中的镜面反射系数 ks 用高光贴图中的颜色值代替。

给地板一个全白的高光贴图，结果如下，p = 32，p = 128：

<img src="/img/spec.png"  height="240px" width="240px"><img src="/img/spec2.png" height="240px" width="240px">

### Step 6 法线贴图

物体的表面往往不会很光滑，会有凹凸不平的地方，表现这种表面的方法可以是增加三角形的数量，让表达更精确，但这也增加了数据的存储和计算量，而另一种方法是利用法线，法线可以表达包含该点的微小平面的方向，使用法线贴图，在渲染时改变该点的法线方向。

而法线贴图有两种，第一种是看起来比较花的图片，即每个像素点的 RGB 值代表着对应点的法线向量，可以直接替换原本的法线，得到结果如下：

<img src="/img/nm.jpg" height="240px" width="240px">

另一种贴图看起来偏蓝紫色，每个像素点记录了切线空间的法线扰动，需要计算从切线空间转换到世界空间下的转换矩阵，即 TBN：

```c++
vecd E1    = tri[1].world_pos - tri[0].world_pos;
vecd E2    = tri[2].world_pos - tri[0].world_pos;
double du1 = tri[1].uv.u - tri[0].uv.u;
double dv1 = tri[1].uv.v - tri[0].uv.v;
double du2 = tri[2].uv.u - tri[0].uv.u;
double dv2 = tri[2].uv.v - tri[0].uv.v;
double temp = dv2 * du1 - dv1 * du2;
vecd T = ((E1 * dv2 - E2 * dv1) / temp).normalized();
vecd B = ((E2 * du1 - E1 * du2) / temp).normalized();
```

而 N 轴即为原本法线方向。

### Step 7 硬阴影

实现硬阴影的原理很简单：

1. 从光的角度记录光能照到的位置（将光当作相机，光栅化记录 z-buffer）
2. 从相机的角度再次光栅化，将像素点转换到光空间下，与第一步记录的 z 值比较，如果该像素点 z 较小，则在阴影中。

得出 shadow map 的 z-buffer 可视化：

<img src="/img/shadowzbuffer.jpg"  height="240px" width="240px">

结果：

<img src="/img/shadowe.jpg"  height="240px" width="240px">

这里有很多波纹状的东西，是自遮挡造成的，需要在判断时增加一个容忍度：

```c++
double bias = std::max(0.005, 0.05 * (1.0 - f.normal * (light.pos - f.world_pos).normalized()));
```

然后比较 `shadow_map[sx + sy * W] - bias` 与 `f.light_space_pos.z` 的大小即可：

<img src="/img/shadow.jpg"  height="240px" width="240px">

到此，基本任务全部完成。
