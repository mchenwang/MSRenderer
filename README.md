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

渲染之前需要载入模型，那么模型存储是首要任务，下面简单设计几个必备的数据结构：点、向量、矩阵。

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
    TGAColor color = TGAColor((uint8_t)255, (uint8_t)0, (uint8_t)0);
	// 之后可能需要增加更多的顶点信息，如纹理、法向等
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