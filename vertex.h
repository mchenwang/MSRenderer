#ifndef __Vertex_H__
#define __Vertex_H__

#include <cmath>
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
using Point2d = Pointd<2>;
using Point3d = Pointd<3>;

template<typename T, size_t n> struct Vector: Point<T, n> {
    T operator*(const Vector<T, n>& x) const {
        T ret = 0.f;
        for(size_t i = 0; i < n; i++) ret += (*this)[i] * x[i];
        return ret;
    }
    Vector<T, n> operator/(const T& x) {
        Vector<T, n> ret;
        for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] / x;
        return ret;
    }
    void operator/=(const T& x) {
        for(size_t i = 0; i < n; i++) (*this)[i] /= x;
    }
    T norm2() { return (*this)*(*this); }
    T norm() { return sqrt((*this)*(*this)); }
    Vector<T, n>& normalize() {
        (*this) /= norm();
        return *this;
    }
    Vector<T, n> normalized() {
        return (*this)/norm();
    }
    
    Vector<T, n> operator+(const Vector<T, n>& b) const {
        Vector<T, n> ret = {};
        for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] + b[i];
        return ret;
    }
    Vector<T, n> operator-(const Vector<T, n>& b) const {
        Vector<T, n> ret = {};
        for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] - b[i];
        return ret;
    }
};

template<size_t n>
using Vectorf = Vector<float, n>;
template<size_t n>
using Vectord = Vector<double, n>;

using Vector2f = Vectorf<2>;
using Vector3f = Vectorf<3>;
using Vector2d = Vectord<2>;
using Vector3d = Vectord<3>;

template<typename T, size_t n> struct Vertex {
    Point<T, n> p;
    Point<T, n-1> uv;
    Vector<T, n> normal;

    Vertex() = default;
    explicit Vertex(const std::array<T, n>&& data_) noexcept { p.data = data_; }
    Vertex(const Vertex<T, n>& v) noexcept { p = v.p; }

    Vertex<T, n>& operator=(const Vertex<T, n>& v) noexcept {
        p = v.p;
        uv = v.uv;
        normal = v.normal;
        return *this;
    }

    T operator[](const size_t i) const { return p[i]; }
    T& operator[](const size_t i) { return p[i]; }

    void set_uv(const Point<T, n-1>& uv_) { uv = uv_; }
    void set_normal(const Vector<T, n>& normal_) { normal = normal_; }
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