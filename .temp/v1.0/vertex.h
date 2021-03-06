#ifndef __Vertex_H__
#define __Vertex_H__

#include <cmath>
#include <array>
#include "tgaimage.h"

namespace MSRender {
    template<typename T, size_t n> struct Point {
        std::array<T, n> data = {};
        Point() = default;
        Point(const std::array<T, n>& data_) noexcept { data = data_; }
        Point(const Point<T, n>& p) noexcept { data = p.data; }
        template<typename U>
        Point(const std::array<U, n>& data_) {
            for(size_t i = 0; i < n; i++) this->data[i] = (T)data_[i];
        }
        template<typename U>
        Point(const Point<U, n>& p) {
            for(size_t i = 0; i < n; i++) this->data[i] = (T)p.data[i];
        }
        Point<T, n>& operator=(const Point<T, n>& p) noexcept { data = p.data; return *this; }
        Point<T, n> operator*(const double x) const {
            Point<T, n> ret;
            for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] * x;
            return ret;
        }
        Point<T, n> operator+(const Point<T, n>& x) const {
            Point<T, n> ret;
            for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] + x[i];
            return ret;
        }
        void operator+=(const Point<T, n>& x) {
            for(size_t i = 0; i < n; i++) (*this)[i] += x[i];
        }

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
        Vector() = default;
        Vector(const std::array<T, n>& data_):Point<T, n>(data_) {}
        
        template<typename U>
        Vector(const Vector<U, n>& v) {
            for(size_t i = 0; i < n; i++) this->data[i] = (T)v.data[i];
        }
        template<typename U>
        Vector<T, n>& operator=(const Vector<U, n>& v) {
            for(size_t i = 0; i < n; i++) this->data[i] = (T)v.data[i];
            return *this;
        }
        template<typename U>
        Vector<T, n>& operator=(const Point<U, n>& v) {
            for(size_t i = 0; i < n; i++) this->data[i] = (T)v.data[i];
            return *this;
        }
        template<typename U, typename V>
        Vector(const Point<U, n>& o, const Point<V, n>& e) {
            for(size_t i = 0; i < n; i++) (*this)[i] = (T)(e[i] - o[i]);
        }
        T operator*(const Vector<T, n>& x) const {
            T ret = 0.f;
            for(size_t i = 0; i < n; i++) ret += (*this)[i] * x[i];
            return ret;
        }
        Vector<T, n> operator/(const T& x) const {
            Vector<T, n> ret;
            for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] / x;
            return ret;
        }
        void operator/=(const T& x) {
            for(size_t i = 0; i < n; i++) (*this)[i] /= x;
        }
        Vector<T, n> operator*(const T& x) const {
            Vector<T, n> ret;
            for(size_t i = 0; i < n; i++) ret[i] = (*this)[i] * x;
            return ret;
        }
        void operator*=(const T& x) {
            for(size_t i = 0; i < n; i++) (*this)[i] *= x;
        }
        T norm2() const { return (*this)*(*this); }
        T norm() const { return sqrt((*this)*(*this)); }
        Vector<T, n>& normalize() {
            (*this) /= norm();
            return *this;
        }
        Vector<T, n> normalized() const {
            return (*this) / norm();
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

        T get_cross_z(const Vector<T, n>& b) const {
            return (*this)[0]*b[1] - (*this)[1]*b[0];
        }
    };

    template<size_t n>
    using Vectori = Vector<int, n>;
    template<size_t n>
    using Vectorf = Vector<float, n>;
    template<size_t n>
    using Vectord = Vector<double, n>;

    using Vector2i = Vectori<2>;
    using Vector3i = Vectori<3>;
    using Vector2f = Vectorf<2>;
    using Vector3f = Vectorf<3>;
    using Vector2d = Vectord<2>;
    using Vector3d = Vectord<3>;

    template<typename T, typename U>
    Vector3d cross(const Vector<T, 2>& a, const Vector<U, 2>& b) {
        return Vector3d({0., 0., a[0]*b[1] - a[1]*b[0]});
    }
    template<typename T, typename U>
    Vector3d cross(const Vector<T, 3>& a, const Vector<U, 3>& b) {
        return Vector3d({a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0]});
    }
    template<typename T, size_t n>
    Vector<T, n> operator-(const Point<T, n>& a, const Point<T, n>& b) {
        Vector<T, n> ret;
        for(size_t i = 0; i < n; i++) ret[i] = a[i] - b[i];
        return ret;
    }

    // template<typename T, size_t n> struct Vertex {
    //     Point<T, n> p;
    //     Point<T, n-1> uv;
    //     Vector<T, n> normal;

    //     Vertex() = default;
    //     explicit Vertex(const std::array<T, n>&& data_) noexcept { p.data = data_; }
    //     Vertex(const Vertex<T, n>& v) noexcept {
    //         p = v.p;
    //         uv = v.uv;
    //         normal = v.normal;
    //     }

    //     Vertex<T, n>& operator=(const Vertex<T, n>& v) noexcept {
    //         p = v.p;
    //         uv = v.uv;
    //         normal = v.normal;
    //         return *this;
    //     }

    //     T operator[](const size_t i) const { return p[i]; }
    //     T& operator[](const size_t i) { return p[i]; }

    //     void set_uv(const Point<T, n-1> uv_) { uv = uv_; }
    //     void set_normal(const Vector<T, n> normal_) { normal = normal_; }
    // };

    // template<size_t n>
    // using Vertexi = Vertex<int, n>;
    // template<size_t n>
    // using Vertexf = Vertex<float, n>;
    // template<size_t n>
    // using Vertexd = Vertex<double, n>;

    // using Vertex2i = Vertexi<2>;
    // using Vertex3i = Vertexi<3>;
    // using Vertex2f = Vertexf<2>;
    // using Vertex3f = Vertexf<3>;
    // using Vertex2d = Vertexd<2>;
    // using Vertex3d = Vertexd<3>;
}

#endif