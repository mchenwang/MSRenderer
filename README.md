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