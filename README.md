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

为查看渲染结果，首先需要将数字信息转换成像素输出成图片形式。简单起见，使用 tinyrendering 项目中的 `TGAImage` 类与 `TGAColor` 类（在 tgaimage.h/tgaimage.cpp 文件中）。

使用如下代码进行测试：

```c++
#include "tgaimage.h"
constexpr int W = 500; // 图片宽
constexpr int H = 500; // 图片高
int main() {
    TGAImage image(W, H, TGAImage::RGB); // 创建图片
    for(int i=0;i<250;i++){
        for(int j=0;j<250;j++){
            image.set(i, j, TGAColor(0, 255, 0)); // 绿
            image.set(i+250, j, TGAColor(255, 0, 0)); // 红
            image.set(i, j+250, TGAColor(0, 0, 0)); // 黑
            image.set(i+250, j+250, TGAColor(0, 0, 255)); //蓝
        }
    }
    image.write_tga_file("output.tga"); //输出图片，格式为 tga
    return 0;
}
```

上述代码构建了一个 500*500 的窗口，并且分为四分，涂上不同的颜色，结果如下：

<img src="/img/colortga.jpg" style="zoom:33%;" />

由结果可知，`TGAImage` 类构建出图片的坐标原点在左下角。