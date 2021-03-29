# SoftwareRendering 说明
 模仿 [tinyrendering](https://github.com/ssloy/tinyrenderer/wiki/Lesson-0:-getting-started) 实现一个简单的软渲染器。

## 运行

- Win10 + MinGW

```sh
git https://github.com/miawua/SoftwareRendering.git
cd SoftwareRendering
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make && rendering.exe
```

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
```

```c++
template<typename T, size_t n> struct Vertex {
    Point<T, n> p;
    Point<T, n-1> uv;
    Vector<T, n> normal;
    Vertex() = default;
    Vertex(const Vertex<T, n>& v) noexcept ;
    Vertex<T, n>& operator=(const Vertex<T, n>& v) noexcept ;

    T operator[](const size_t i) const ;
    T& operator[](const size_t i) ;
};
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
    std::vector<Vertex3d> vertexs;
    std::vector<int> face;
    TGAImage diffusemap_; // diffuse color texture
    TGAImage normalmap_; // normal map texture
    TGAImage specularmap_; // specular map texture
};
```

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

<img src="/img/triangle.jpg" style="width:200px;" />

仔细观察，可以发现三角形的边界并不平滑，有走样现象：

<img src="/img/aliasing.png" style="width:200px;">

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

<img src="/img/triangleMSAA.jpg" style="width:200px;" /><img src="/img/anti-aliasingMSAA.png" style="width:200px;">

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

<img src="/img/face1.jpg" style="width:200px;" />

这里不难发现，模型嘴部等几个地方出现了异样，因为上述做法是对三角形顺序遍历，逐一绘制，而当三角形的顺序不是按照画家算法的顺序存储，或者三角形存在交叉的情况时，就会出现错误的覆盖现象。

解决这个问题需要用到 z-buffer，即不再以三角形为单位绘制，而是以像素为单位，逐个计算像素的深度值，深度大（z 大）的覆盖深度小的。三角形中间的像素深度值，通过三角形重心坐标进行差值计算。结果如下：

<img src="/img/facez-buffer.jpg" style="width:200px;" />

将 z-buffer 可视化绘制一下：

<img src="/img/z-buffer.jpg" style="width:200px;" />