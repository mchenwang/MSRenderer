#ifndef __ALGEBRA_H__
#define __ALGEBRA_H__
#include <iostream>
#include <cmath>

namespace MSRender {
    template<size_t n, typename T>
    struct vec { };

    template<typename T>
    struct vec<2, T> {
        T u, v;
        vec() = default;
        ~vec() = default;
        template<typename U> vec(const vec<2, U>& v) noexcept { u = (T)v.u, v = (T)v.v; }
        vec(T _u, T _v) noexcept : u(_u), v(_v) {}
        template<typename U> vec<2, T>& operator=(const vec<2, U>& v) {
            u = (T)v.u, v = (T)v.v;
            return *this;
        }

        T operator[](size_t i) const {
            if(i > 1 || i < 0) std::cerr << i << " uv access error\n";
            return i == 0 ? u : v;
        }
        T& operator[](size_t i) {
            if(i > 1 || i < 0) std::cerr << i << " uv access error\n";
            return i == 0 ? u : v;
        }

        vec<2, T> operator+(const vec<2, T>& b) const {
            return vec<2, T>(u+b.u, v+b.v);
        }
        vec<2, T>& operator+=(const vec<2, T>& b) {
            for(size_t i = 0; i < 2; i++) (*this)[i] += b[i];
            return *this;
        }
        vec<2, T> operator*(double c) const {
            return vec<2, T>(u*c, v*c);
        }
        vec<2, T>& operator*=(double c) {
            *this = (*this) * c;
            return *this;
        }
        vec<2, T> operator/(double c) const {
            return vec<2, T>(u/c, v/c);
        }
        vec<2, T>& operator/=(double c) {
            for(size_t i = 0; i < 2; i++) (*this)[i] /= c;
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& out, const vec<2, T>& v) {
            for(int i = 0; i < 2; i++) {
                out << v[i] << " ";
            }
            return out;
        }
    };

    using uvd = vec<2, double>;
    using uvf = vec<2, float>;
    using uvi = vec<2, int>;

    template<typename T>
    struct vec<4, T> {
        T x, y, z, w;
        ~vec() = default;
        vec(T _x=0, T _y=0, T _z=0, T _w=0) noexcept // w = 0 is vec, w = 1 is point
        : x(_x), y(_y), z(_z), w(_w) {}
        template<typename U> vec(const vec<4, U>& v) noexcept 
        { x = (T)v.x, y = (T)v.y, z = (T)v.z, w = (T)v.w; }
        template<typename U> vec<4, T>& operator=(const vec<4, U>& v) {
            x = (T)v.x, y = (T)v.y, z = (T)v.z, w = (T)v.w;
            return *this;
        }

        T operator[](const size_t i) const {
            if(i > 3 || i < 0) std::cerr << i << " vec access error\n";
            if(i == 0) return x;
            if(i == 1) return y;
            if(i == 2) return z;
            return w;
        }
        T& operator[](const size_t i) {
            if(i > 3 || i < 0) std::cerr << i << " vec access error\n";
            if(i == 0) return x;
            if(i == 1) return y;
            if(i == 2) return z;
            return w;
        }

        vec<4, T> operator*(double c) const {
            return vec<4, T>(x*c, y*c, z*c, w*c);
        }
        vec<4, T>& operator*=(double c) {
            x *= c, y *= c, z *= c, w *= c;
            return *this;
        }
        double operator*(const vec<4, T>& a) const {
            return a.x*x + a.y*y + a.z*z + a.w*w;
        }
        vec<4, T> operator+(const vec<4, T>& b) const {
            return vec<4, T>(x+b.x, y+b.y, z+b.z, w+b.w);
        }
        vec<4, T>& operator+=(const vec<4, T>& b) {
            for(size_t i = 0; i < 4; i++) (*this)[i] += b[i];
            return *this;
        }
        vec<4, T> operator/(double c) const {
            return vec<4, T>(x/c, y/c, z/c, w/c);
        }
        vec<4, T>& operator/=(double c) {
            x /= c, y /= c, z /= c, w /= c;
            return *this;
        }

        double norm2() const { return x*x+y*y+z*z; }
        double norm() const { return std::sqrt(norm2()); }
        vec<4, T> normalized() const {
            return (*this) / norm();
        }
        vec<4, T>& normalize() {
            (*this) /= norm();
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& out, const vec<4, T>& v) {
            for(int i = 0; i < 4; i++) {
                out << v[i] << " ";
            }
            return out;
        }
    };

    template<typename T, typename U>
    vec<4, double> cross(const vec<4, T>& a, const vec<4, U>& b) {
        return vec<4, double>(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x, 0);
    }

    using vecd = vec<4, double>; using pointd = vec<4, double>;
    using vecf = vec<4, float>;  using pointf = vec<4, float>;
    using veci = vec<4, int>;    using pointi = vec<4, int>;

    template<typename T, typename U>
    vecd operator-(const vec<4, T>& a, const vec<4, U>& b) {
        return vecd(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w);
    }

    template<size_t row, size_t col, typename T>
    struct mat{ };

    template<typename T>
    struct mat<4, 4, T> {
        vec<4, T> data[4];
        mat(bool is_I = false) noexcept {
            for(size_t i = 0; i < 4; i++) data[i] = vec<4, T>(0, 0, 0, 0);
            if(is_I) for(size_t i = 0; i < 4; i++) data[i][i] = 1;
        }
        ~mat() = default;
        mat(const mat<4, 4, T>& m) {
            for(size_t i = 0; i < 4; i++) data[i] = m.data[i];
        }
        mat<4, 4, T>& operator=(const mat<4, 4, T>& m) {
            for(size_t i = 0; i < 4; i++) data[i] = m.data[i];
            return *this;
        }
        vec<4, T> operator[](const size_t i) const { return data[i]; }
        vec<4, T>& operator[](const size_t i) { return data[i]; }
        vec<4, T> column(size_t i) const {
            return vec<4, T>(data[0][i], data[1][i], data[2][i], data[3][i]);
        }

        mat<4, 4, T> operator*(const mat<4, 4, T>& m) const {
            mat<4, 4, T> ret;
            for(size_t i = 0; i < 4; i++)
                for(size_t j = 0; j < 4; j++)
                    ret[i][j] = (*this)[i] * m.column(j);
            return ret;
        }

        friend std::ostream& operator<<(std::ostream& out, const mat<4, 4, T>& m) {
            for(int i = 0; i < 4; i++) {
                out << m[i] << "\n";
            }
            return out;
        }

        mat<4, 4, T> Transpose() {
            mat<4, 4, T> ret;
            for(int i = 0; i < 4; i++) ret[i] = (*this).column(i);
            return ret;
        }

        mat<4, 4, double> inverse() {
            mat<4, 4, double> a = (*this);
            mat<4, 4, double> b(true);
            for(int i = 0; i < 4; i++) {
                int main_r = i;
                for(int j = 0; j < 4; j++) {
                    if(std::abs(a[j][i]) > std::abs(a[main_r][i]))
                        main_r = j;
                }
                if(a[main_r][i] == 0) {
                    std::cerr << "mat cannot inverse.\n";
                    return (*this);
                }
                if(main_r != i) std::swap(a[main_r], a[i]), std::swap(b[main_r], b[i]);
                for(int k = 0; k < 4; k++) if(k != i) {
                    double p = a[k][i] / a[i][i];
                    for(int j = 0; j < 4; j++) {
                        a[k][j] -= p*a[i][j];
                        b[k][j] -= p*b[i][j];
                    }
                }
            }
            for(int i = 0; i < 4; i++) b[i]/=a[i][i];
            return b;
        }
    };
    using mat4d = mat<4, 4, double>;
    using mat4f = mat<4, 4, float>;
    using mat4i = mat<4, 4, int>;

    template<typename T, typename U>
    vec<4, double> operator*(const mat<4, 4, U>& m, const vec<4, T>& v) {
        return vec<4, double>(m[0]*v, m[1]*v, m[2]*v, m[3]*v);
    }
}

#endif