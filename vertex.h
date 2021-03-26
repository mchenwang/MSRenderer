#ifndef __Vertex_H__
#define __Vertex_H__

#include <cassert>
#include <array>
#include "tgaimage.h"

template<typename T, size_t n> struct Point {
    std::array<T, n> data = {};
    Point() = default;
    Point(const std::array<T, n>& data_) noexcept { data = data_; }
    Point(const Point<T, n>& p) noexcept { data = p.data; }
    Point<T, n>& operator=(const Point<T, n>& p) noexcept { data = p.data; return *this; }

    T operator[](const size_t i) const { return data.at(i); }
    T& operator[](const size_t i) { return data.at(i); }
    
    Point<T, n> operator+(const Point<T, n>& b) const {
        Point<T, n> ret = {};
        for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] + b[i];
        return ret;
    }
    Point<T, n> operator-(const Point<T, n>& b) const {
        Point<T, n> ret = {};
        for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] - b[i];
        return ret;
    }
};

template<size_t n>
using Pointi = Point<int, n>;
template<size_t n>
using Pointf = Point<float, n>;
template<size_t n>
using Pointd = Point<double, n>;

using Point2i = Pointi<2>;
using Point3i = Pointi<3>;
using Point2f = Pointf<2>;
using Point3f = Pointf<3>;
using Point2d = Pointi<2>;
using Point3d = Pointd<3>;

template<typename T, size_t n> struct Vertex {
    Point<T, n> p;
    TGAColor color = TGAColor((uint8_t)255, (uint8_t)0, (uint8_t)0);

    Vertex() = default;
    Vertex(const Vertex<T, n>& v) noexcept { p = v.p; }
    Vertex<T, n>& operator=(const Vertex<T, n>& v) noexcept {
        p = v.p;
        color = v.color;
        return *this;
    }

    T operator[](const size_t i) const { return p[i]; }
    T& operator[](const size_t i) { return p[i]; }
};

template<size_t n>
using Vertexi = Vertex<int, n>;
template<size_t n>
using Vertexf = Vertex<float, n>;
template<size_t n>
using Vertexd = Vertex<double, n>;

using Vertex2i = Vertexi<2>;
using Vertex3i = Vertexi<3>;
using Vertex2f = Vertexf<2>;
using Vertex3f = Vertexf<3>;
using Vertex2d = Vertexd<2>;
using Vertex3d = Vertexd<3>;

#endif