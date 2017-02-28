//=======================================================================
// Copyright (c) 2014-2017 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#pragma once

// The idea of the kernel is largely inspired by the kernels in Blaze
// by Klaus Igleberg

namespace etl {

namespace impl {

namespace vec {

/*!
 * \brief Optimized version of small GEMV for row major version
 * \param aa The lhs matrix
 * \param bb The rhs vector
 * \param cc The result vector
 */
template <typename V, bool Padded, typename T>
void gemv_small_kernel(const T* aa, size_t m, size_t n, const T* bb, T* cc) {
    using vec_type = V;

    static constexpr size_t vec_size = vec_type::template traits<T>::size;

    static constexpr bool remainder = !advanced_padding || !Padded;
    const size_t last = remainder ? (n & size_t(-vec_size)) : n;

    size_t i = 0;

    // 8-Unrolled outer loop
    for (; i + 7 < m; i += 8) {
        auto r1 = vec_type::template zero<T>();
        auto r2 = vec_type::template zero<T>();
        auto r3 = vec_type::template zero<T>();
        auto r4 = vec_type::template zero<T>();
        auto r5 = vec_type::template zero<T>();
        auto r6 = vec_type::template zero<T>();
        auto r7 = vec_type::template zero<T>();
        auto r8 = vec_type::template zero<T>();

        size_t k = 0;

        // Vectorized inner loop
        for (; k < last; k += vec_size) {
            auto b1 = vec_type::load(bb + k);

            auto a1 = vec_type::loadu(aa + (i + 0) * n + k);
            auto a2 = vec_type::loadu(aa + (i + 1) * n + k);
            auto a3 = vec_type::loadu(aa + (i + 2) * n + k);
            auto a4 = vec_type::loadu(aa + (i + 3) * n + k);
            auto a5 = vec_type::loadu(aa + (i + 4) * n + k);
            auto a6 = vec_type::loadu(aa + (i + 5) * n + k);
            auto a7 = vec_type::loadu(aa + (i + 6) * n + k);
            auto a8 = vec_type::loadu(aa + (i + 7) * n + k);

            r1 = vec_type::fmadd(a1, b1, r1);
            r2 = vec_type::fmadd(a2, b1, r2);
            r3 = vec_type::fmadd(a3, b1, r3);
            r4 = vec_type::fmadd(a4, b1, r4);
            r5 = vec_type::fmadd(a5, b1, r5);
            r6 = vec_type::fmadd(a6, b1, r6);
            r7 = vec_type::fmadd(a7, b1, r7);
            r8 = vec_type::fmadd(a8, b1, r8);
        }

        cc[i + 0] = vec_type::hadd(r1);
        cc[i + 1] = vec_type::hadd(r2);
        cc[i + 2] = vec_type::hadd(r3);
        cc[i + 3] = vec_type::hadd(r4);
        cc[i + 4] = vec_type::hadd(r5);
        cc[i + 5] = vec_type::hadd(r6);
        cc[i + 6] = vec_type::hadd(r7);
        cc[i + 7] = vec_type::hadd(r8);

        // Remainder inner loop
        for (; remainder && k < n; ++k) {
            cc[i + 0] += aa[(i + 0) * n + k] * bb[k];
            cc[i + 1] += aa[(i + 1) * n + k] * bb[k];
            cc[i + 2] += aa[(i + 2) * n + k] * bb[k];
            cc[i + 3] += aa[(i + 3) * n + k] * bb[k];
            cc[i + 4] += aa[(i + 4) * n + k] * bb[k];
            cc[i + 5] += aa[(i + 5) * n + k] * bb[k];
            cc[i + 6] += aa[(i + 6) * n + k] * bb[k];
            cc[i + 7] += aa[(i + 7) * n + k] * bb[k];
        }
    }

    // 2-Unrolled outer loop
    for (; i + 1 < m; i += 2) {
        auto r1 = vec_type::template zero<T>();
        auto r2 = vec_type::template zero<T>();

        size_t k = 0;

        // Vectorized inner loop
        for (; k < last; k += vec_size) {
            auto b1 = vec_type::load(bb + k);

            auto a1 = vec_type::loadu(aa + (i + 0) * n + k);
            auto a2 = vec_type::loadu(aa + (i + 1) * n + k);

            r1 = vec_type::fmadd(a1, b1, r1);
            r2 = vec_type::fmadd(a2, b1, r2);
        }

        cc[i + 0] = vec_type::hadd(r1);
        cc[i + 1] = vec_type::hadd(r2);

        // Remainder inner loop
        for (; remainder && k < n; ++k) {
            cc[i + 0] += aa[(i + 0) * n + k] * bb[k];
            cc[i + 1] += aa[(i + 1) * n + k] * bb[k];
        }
    }

    // Remainder loop
    if (i < m) {
        auto r1 = vec_type::template zero<T>();

        size_t k = 0;

        // Vectorized inner loop
        for (; k < last; k += vec_size) {
            auto b1 = vec_type::load(bb + k);
            auto a1 = vec_type::loadu(aa + (i + 0) * n + k);
            r1 = vec_type::fmadd(a1, b1, r1);
        }

        auto result = vec_type::hadd(r1);

        // Remainder inner loop
        for (; remainder && k < n; ++k) {
            result += aa[i * n + k] * bb[k];
        }

        cc[i] = result;
    }
}

/*!
 * \brief Optimized version of large GEMV for row major version
 * \param aa The lhs matrix
 * \param bb The rhs vector
 * \param cc The result vector
 */
template <typename V, bool Padded, typename T>
void gemv_large_kernel(const T* aa, size_t m, size_t n, const T* bb, T* cc) {
    using vec_type = V;

    static constexpr size_t vec_size = vec_type::template traits<T>::size;

    static constexpr bool remainder = !advanced_padding || !Padded;
    const size_t last = remainder ? (n & size_t(-vec_size)) : n;

    size_t i = 0;

    // 8-Unrolled outer loop
    for (; i + 7 < m; i += 8) {
        auto r1 = vec_type::template zero<T>();
        auto r2 = vec_type::template zero<T>();
        auto r3 = vec_type::template zero<T>();
        auto r4 = vec_type::template zero<T>();
        auto r5 = vec_type::template zero<T>();
        auto r6 = vec_type::template zero<T>();
        auto r7 = vec_type::template zero<T>();
        auto r8 = vec_type::template zero<T>();

        size_t k = 0;

        // 4-Unrolled Vectorized inner loop
        for (; k + (vec_size * 3) < last; k += vec_size * 4) {
            const auto k1 = k + 0 * vec_size;
            const auto k2 = k + 1 * vec_size;
            const auto k3 = k + 2 * vec_size;
            const auto k4 = k + 3 * vec_size;

            auto b1 = vec_type::load(bb + k1);
            auto b2 = vec_type::load(bb + k2);
            auto b3 = vec_type::load(bb + k3);
            auto b4 = vec_type::load(bb + k4);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k1), b1, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k1), b1, r2);
            r3 = vec_type::fmadd(vec_type::loadu(aa + (i + 2) * n + k1), b1, r3);
            r4 = vec_type::fmadd(vec_type::loadu(aa + (i + 3) * n + k1), b1, r4);
            r5 = vec_type::fmadd(vec_type::loadu(aa + (i + 4) * n + k1), b1, r5);
            r6 = vec_type::fmadd(vec_type::loadu(aa + (i + 5) * n + k1), b1, r6);
            r7 = vec_type::fmadd(vec_type::loadu(aa + (i + 6) * n + k1), b1, r7);
            r8 = vec_type::fmadd(vec_type::loadu(aa + (i + 7) * n + k1), b1, r8);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k2), b2, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k2), b2, r2);
            r3 = vec_type::fmadd(vec_type::loadu(aa + (i + 2) * n + k2), b2, r3);
            r4 = vec_type::fmadd(vec_type::loadu(aa + (i + 3) * n + k2), b2, r4);
            r5 = vec_type::fmadd(vec_type::loadu(aa + (i + 4) * n + k2), b2, r5);
            r6 = vec_type::fmadd(vec_type::loadu(aa + (i + 5) * n + k2), b2, r6);
            r7 = vec_type::fmadd(vec_type::loadu(aa + (i + 6) * n + k2), b2, r7);
            r8 = vec_type::fmadd(vec_type::loadu(aa + (i + 7) * n + k2), b2, r8);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k3), b3, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k3), b3, r2);
            r3 = vec_type::fmadd(vec_type::loadu(aa + (i + 2) * n + k3), b3, r3);
            r4 = vec_type::fmadd(vec_type::loadu(aa + (i + 3) * n + k3), b3, r4);
            r5 = vec_type::fmadd(vec_type::loadu(aa + (i + 4) * n + k3), b3, r5);
            r6 = vec_type::fmadd(vec_type::loadu(aa + (i + 5) * n + k3), b3, r6);
            r7 = vec_type::fmadd(vec_type::loadu(aa + (i + 6) * n + k3), b3, r7);
            r8 = vec_type::fmadd(vec_type::loadu(aa + (i + 7) * n + k3), b3, r8);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k4), b4, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k4), b4, r2);
            r3 = vec_type::fmadd(vec_type::loadu(aa + (i + 2) * n + k4), b4, r3);
            r4 = vec_type::fmadd(vec_type::loadu(aa + (i + 3) * n + k4), b4, r4);
            r5 = vec_type::fmadd(vec_type::loadu(aa + (i + 4) * n + k4), b4, r5);
            r6 = vec_type::fmadd(vec_type::loadu(aa + (i + 5) * n + k4), b4, r6);
            r7 = vec_type::fmadd(vec_type::loadu(aa + (i + 6) * n + k4), b4, r7);
            r8 = vec_type::fmadd(vec_type::loadu(aa + (i + 7) * n + k4), b4, r8);
        }

        // 2-Unrolled Vectorized inner loop
        for (; k + (vec_size) < last; k += vec_size * 2) {
            const auto k1 = k + 0 * vec_size;
            const auto k2 = k + 1 * vec_size;

            auto b1 = vec_type::load(bb + k1);
            auto b2 = vec_type::load(bb + k2);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k1), b1, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k1), b1, r2);
            r3 = vec_type::fmadd(vec_type::loadu(aa + (i + 2) * n + k1), b1, r3);
            r4 = vec_type::fmadd(vec_type::loadu(aa + (i + 3) * n + k1), b1, r4);
            r5 = vec_type::fmadd(vec_type::loadu(aa + (i + 4) * n + k1), b1, r5);
            r6 = vec_type::fmadd(vec_type::loadu(aa + (i + 5) * n + k1), b1, r6);
            r7 = vec_type::fmadd(vec_type::loadu(aa + (i + 6) * n + k1), b1, r7);
            r8 = vec_type::fmadd(vec_type::loadu(aa + (i + 7) * n + k1), b1, r8);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k2), b2, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k2), b2, r2);
            r3 = vec_type::fmadd(vec_type::loadu(aa + (i + 2) * n + k2), b2, r3);
            r4 = vec_type::fmadd(vec_type::loadu(aa + (i + 3) * n + k2), b2, r4);
            r5 = vec_type::fmadd(vec_type::loadu(aa + (i + 4) * n + k2), b2, r5);
            r6 = vec_type::fmadd(vec_type::loadu(aa + (i + 5) * n + k2), b2, r6);
            r7 = vec_type::fmadd(vec_type::loadu(aa + (i + 6) * n + k2), b2, r7);
            r8 = vec_type::fmadd(vec_type::loadu(aa + (i + 7) * n + k2), b2, r8);
        }

        // Vectorized inner loop
        if (k < last) {
            auto b1 = vec_type::load(bb + k);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k), b1, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k), b1, r2);
            r3 = vec_type::fmadd(vec_type::loadu(aa + (i + 2) * n + k), b1, r3);
            r4 = vec_type::fmadd(vec_type::loadu(aa + (i + 3) * n + k), b1, r4);
            r5 = vec_type::fmadd(vec_type::loadu(aa + (i + 4) * n + k), b1, r5);
            r6 = vec_type::fmadd(vec_type::loadu(aa + (i + 5) * n + k), b1, r6);
            r7 = vec_type::fmadd(vec_type::loadu(aa + (i + 6) * n + k), b1, r7);
            r8 = vec_type::fmadd(vec_type::loadu(aa + (i + 7) * n + k), b1, r8);

            k += vec_size;
        }

        cc[i + 0] = vec_type::hadd(r1);
        cc[i + 1] = vec_type::hadd(r2);
        cc[i + 2] = vec_type::hadd(r3);
        cc[i + 3] = vec_type::hadd(r4);
        cc[i + 4] = vec_type::hadd(r5);
        cc[i + 5] = vec_type::hadd(r6);
        cc[i + 6] = vec_type::hadd(r7);
        cc[i + 7] = vec_type::hadd(r8);

        // Remainder inner loop
        for (; remainder && k < n; ++k) {
            cc[i + 0] += aa[(i + 0) * n + k] * bb[k];
            cc[i + 1] += aa[(i + 1) * n + k] * bb[k];
            cc[i + 2] += aa[(i + 2) * n + k] * bb[k];
            cc[i + 3] += aa[(i + 3) * n + k] * bb[k];
            cc[i + 4] += aa[(i + 4) * n + k] * bb[k];
            cc[i + 5] += aa[(i + 5) * n + k] * bb[k];
            cc[i + 6] += aa[(i + 6) * n + k] * bb[k];
            cc[i + 7] += aa[(i + 7) * n + k] * bb[k];
        }
    }

    // 2-Unrolled outer loop
    for (; i + 1 < m; i += 2) {
        auto r1 = vec_type::template zero<T>();
        auto r2 = vec_type::template zero<T>();

        size_t k = 0;

        // 4-Unrolled Vectorized inner loop
        for (; k + (vec_size * 3) < last; k += vec_size * 4) {
            const auto k1 = k + 0 * vec_size;
            const auto k2 = k + 1 * vec_size;
            const auto k3 = k + 2 * vec_size;
            const auto k4 = k + 3 * vec_size;

            auto b1 = vec_type::load(bb + k1);
            auto b2 = vec_type::load(bb + k2);
            auto b3 = vec_type::load(bb + k3);
            auto b4 = vec_type::load(bb + k4);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k1), b1, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k1), b1, r2);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k2), b2, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k2), b2, r2);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k3), b3, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k3), b3, r2);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k4), b4, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k4), b4, r2);
        }

        // 2-Unrolled Vectorized inner loop
        for (; k + vec_size < last; k += vec_size * 2) {
            const auto k1 = k + 0 * vec_size;
            const auto k2 = k + 1 * vec_size;

            auto b1 = vec_type::load(bb + k1);
            auto b2 = vec_type::load(bb + k2);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k1), b1, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k1), b1, r2);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k2), b2, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k2), b2, r2);
        }

        // Vectorized inner loop
        if (k < last) {
            auto b1 = vec_type::load(bb + k);

            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k), b1, r1);
            r2 = vec_type::fmadd(vec_type::loadu(aa + (i + 1) * n + k), b1, r2);

            k += vec_size;
        }

        cc[i + 0] = vec_type::hadd(r1);
        cc[i + 1] = vec_type::hadd(r2);

        // Remainder inner loop
        for (; remainder && k < n; ++k) {
            cc[i + 0] += aa[i + 0 * n + k] * bb[k];
            cc[i + 1] += aa[i + 1 * n + k] * bb[k];
        }
    }

    // Remainder loop
    if (i < m) {
        auto r1 = vec_type::template zero<T>();

        size_t k = 0;

        // Vectorized inner loop
        for (; k < last; k += vec_size) {
            auto b1 = vec_type::load(bb + k);
            r1 = vec_type::fmadd(vec_type::loadu(aa + (i + 0) * n + k), b1, r1);
        }

        auto result = vec_type::hadd(r1);

        // Remainder inner loop
        for (; remainder && k < n; ++k) {
            result += aa[i * n + k] * bb[k];
        }

        cc[i] = result;
    }
}

/*!
 * \brief Optimized version of GEMV for row major version
 * \param a The lhs matrix
 * \param b The rhs vector
 * \param c The result vector
 */
template <typename A, typename B, typename C, cpp_enable_if((all_row_major<A, B, C>::value))>
void gemv(A&& a, B&& b, C&& c) {
    cpp_assert(vec_enabled, "At least one vector mode must be enabled for impl::VEC");

    a.ensure_cpu_up_to_date();
    b.ensure_cpu_up_to_date();

    const auto m = rows(a);
    const auto n = columns(a);

    if (etl::size(a) < gemv_small_threshold) {
        gemv_small_kernel<default_vec, all_padded<A, B, C>::value>(a.memory_start(), m, n, b.memory_start(), c.memory_start());
    } else {
        gemv_large_kernel<default_vec, all_padded<A, B, C>::value>(a.memory_start(), m, n, b.memory_start(), c.memory_start());
    }

    c.invalidate_gpu();
}

/*!
 * \brief Unoptimized version of GEMV for column major version
 * \param a The lhs matrix
 * \param b The rhs vector
 * \param c The result vector
 */
template <typename A, typename B, typename C, cpp_disable_if((all_row_major<A, B, C>::value))>
void gemv(A&& a, B&& b, C&& c) {
    cpp_assert(vec_enabled, "At least one vector mode must be enabled for impl::VEC");

    const auto m = rows(a);
    const auto n = columns(a);

    c = 0;

    for (size_t i = 0; i < m; i++) {
        for (size_t k = 0; k < n; k++) {
            c(i) += a(i, k) * b(k);
        }
    }
}

/*!
 * \brief Optimized version of small GEVM for row major version
 * \param a The lhs vector
 * \param b The rhs matrix
 * \param c The result vector
 */
template <typename V, typename T>
void gevm_small_kernel(const T* aa, size_t m, size_t n, const T* bb, T* cc) {
    using vec_type = V;

    static constexpr size_t vec_size = vec_type::template traits<T>::size;

    size_t j = 0;

    for (; j + vec_size * 8 - 1 < n; j += vec_size * 8) {
        auto r1 = vec_type::template zero<T>();
        auto r2 = vec_type::template zero<T>();
        auto r3 = vec_type::template zero<T>();
        auto r4 = vec_type::template zero<T>();
        auto r5 = vec_type::template zero<T>();
        auto r6 = vec_type::template zero<T>();
        auto r7 = vec_type::template zero<T>();
        auto r8 = vec_type::template zero<T>();

        for (size_t k = 0; k < m; k++) {
            auto a1 = vec_type::set(aa[k]);

            auto b1 = vec_type::loadu(bb + k * n + j + 0 * vec_size);
            auto b2 = vec_type::loadu(bb + k * n + j + 1 * vec_size);
            auto b3 = vec_type::loadu(bb + k * n + j + 2 * vec_size);
            auto b4 = vec_type::loadu(bb + k * n + j + 3 * vec_size);
            auto b5 = vec_type::loadu(bb + k * n + j + 4 * vec_size);
            auto b6 = vec_type::loadu(bb + k * n + j + 5 * vec_size);
            auto b7 = vec_type::loadu(bb + k * n + j + 6 * vec_size);
            auto b8 = vec_type::loadu(bb + k * n + j + 7 * vec_size);

            r1 = vec_type::fmadd(a1, b1, r1);
            r2 = vec_type::fmadd(a1, b2, r2);
            r3 = vec_type::fmadd(a1, b3, r3);
            r4 = vec_type::fmadd(a1, b4, r4);
            r5 = vec_type::fmadd(a1, b5, r5);
            r6 = vec_type::fmadd(a1, b6, r6);
            r7 = vec_type::fmadd(a1, b7, r7);
            r8 = vec_type::fmadd(a1, b8, r8);
        }

        vec_type::storeu(cc + j + 0 * vec_size, r1);
        vec_type::storeu(cc + j + 1 * vec_size, r2);
        vec_type::storeu(cc + j + 2 * vec_size, r3);
        vec_type::storeu(cc + j + 3 * vec_size, r4);
        vec_type::storeu(cc + j + 4 * vec_size, r5);
        vec_type::storeu(cc + j + 5 * vec_size, r6);
        vec_type::storeu(cc + j + 6 * vec_size, r7);
        vec_type::storeu(cc + j + 7 * vec_size, r8);
    }

    for (; j + vec_size * 4 - 1 < n; j += vec_size * 4) {
        auto r1 = vec_type::template zero<T>();
        auto r2 = vec_type::template zero<T>();
        auto r3 = vec_type::template zero<T>();
        auto r4 = vec_type::template zero<T>();

        for (size_t k = 0; k < m; k++) {
            auto a1 = vec_type::set(aa[k]);

            auto b1 = vec_type::loadu(bb + k * n + j + 0 * vec_size);
            auto b2 = vec_type::loadu(bb + k * n + j + 1 * vec_size);
            auto b3 = vec_type::loadu(bb + k * n + j + 2 * vec_size);
            auto b4 = vec_type::loadu(bb + k * n + j + 3 * vec_size);

            r1 = vec_type::fmadd(a1, b1, r1);
            r2 = vec_type::fmadd(a1, b2, r2);
            r3 = vec_type::fmadd(a1, b3, r3);
            r4 = vec_type::fmadd(a1, b4, r4);
        }

        vec_type::storeu(cc + j + 0 * vec_size, r1);
        vec_type::storeu(cc + j + 1 * vec_size, r2);
        vec_type::storeu(cc + j + 2 * vec_size, r3);
        vec_type::storeu(cc + j + 3 * vec_size, r4);
    }

    for (; j + vec_size - 1 < n; j += vec_size) {
        auto r1 = vec_type::template zero<T>();

        for (size_t k = 0; k < m; k++) {
            auto a1 = vec_type::set(aa[k]);

            auto b1 = vec_type::loadu(bb + k * n + j + 0 * vec_size);

            r1 = vec_type::fmadd(a1, b1, r1);
        }

        vec_type::storeu(cc + j + 0 * vec_size, r1);
    }

    for (; j < n; j++) {
        auto value = T();

        for (size_t k = 0; k < m; k++) {
            value += aa[k] * bb[k * n + j];
        }

        cc[j] = value;
    }
}

/*!
 * \brief Optimized version of large GEVM for row major version
 * \param a The lhs vector
 * \param b The rhs matrix
 * \param c The result vector
 */
template <typename V, typename T>
void gevm_large_kernel(const T* aa, size_t m, size_t n, const T* bb, T* cc) {
    using vec_type = V;

    static constexpr size_t vec_size = vec_type::template traits<T>::size;

    const size_t n_block = (32 * 1024) / sizeof(T);
    const size_t m_block = n < n_block ? 8 : 4;

    for (size_t block_j = 0; block_j < n; block_j += n_block) {
        for (size_t block_k = 0; block_k < m; block_k += m_block) {
            const size_t m_end = std::min(block_k + m_block, m);
            const size_t n_end = std::min(block_j + n_block, n) & size_t(-vec_size);

            size_t j = block_j;

            // 8-Unrolled Vectorized loop
            for (; j + vec_size * 8 - 1 < n_end; j += vec_size * 8) {
                auto r1 = vec_type::template zero<T>();
                auto r2 = vec_type::template zero<T>();
                auto r3 = vec_type::template zero<T>();
                auto r4 = vec_type::template zero<T>();
                auto r5 = vec_type::template zero<T>();
                auto r6 = vec_type::template zero<T>();
                auto r7 = vec_type::template zero<T>();
                auto r8 = vec_type::template zero<T>();

                for (size_t k = block_k; k < m_end; ++k) {
                    auto a1 = vec_type::set(aa[k]);

                    auto b1 = vec_type::loadu(bb + k * n + j + 0 * vec_size);
                    auto b2 = vec_type::loadu(bb + k * n + j + 1 * vec_size);
                    auto b3 = vec_type::loadu(bb + k * n + j + 2 * vec_size);
                    auto b4 = vec_type::loadu(bb + k * n + j + 3 * vec_size);
                    auto b5 = vec_type::loadu(bb + k * n + j + 4 * vec_size);
                    auto b6 = vec_type::loadu(bb + k * n + j + 5 * vec_size);
                    auto b7 = vec_type::loadu(bb + k * n + j + 6 * vec_size);
                    auto b8 = vec_type::loadu(bb + k * n + j + 7 * vec_size);

                    r1 = vec_type::fmadd(a1, b1, r1);
                    r2 = vec_type::fmadd(a1, b2, r2);
                    r3 = vec_type::fmadd(a1, b3, r3);
                    r4 = vec_type::fmadd(a1, b4, r4);
                    r5 = vec_type::fmadd(a1, b5, r5);
                    r6 = vec_type::fmadd(a1, b6, r6);
                    r7 = vec_type::fmadd(a1, b7, r7);
                    r8 = vec_type::fmadd(a1, b8, r8);
                }

                vec_type::storeu(cc + j + 0 * vec_size, vec_type::add(r1, vec_type::loadu(cc + j + 0 * vec_size)));
                vec_type::storeu(cc + j + 1 * vec_size, vec_type::add(r2, vec_type::loadu(cc + j + 1 * vec_size)));
                vec_type::storeu(cc + j + 2 * vec_size, vec_type::add(r3, vec_type::loadu(cc + j + 2 * vec_size)));
                vec_type::storeu(cc + j + 3 * vec_size, vec_type::add(r4, vec_type::loadu(cc + j + 3 * vec_size)));
                vec_type::storeu(cc + j + 4 * vec_size, vec_type::add(r5, vec_type::loadu(cc + j + 4 * vec_size)));
                vec_type::storeu(cc + j + 5 * vec_size, vec_type::add(r6, vec_type::loadu(cc + j + 5 * vec_size)));
                vec_type::storeu(cc + j + 6 * vec_size, vec_type::add(r7, vec_type::loadu(cc + j + 6 * vec_size)));
                vec_type::storeu(cc + j + 7 * vec_size, vec_type::add(r8, vec_type::loadu(cc + j + 7 * vec_size)));
            }

            // 4-Unrolled vectorized loop
            for (; j + vec_size * 4 - 1 < n_end; j += vec_size * 4) {
                auto r1 = vec_type::template zero<T>();
                auto r2 = vec_type::template zero<T>();
                auto r3 = vec_type::template zero<T>();
                auto r4 = vec_type::template zero<T>();

                for (size_t k = block_k; k < m_end; ++k) {
                    auto a1 = vec_type::set(aa[k]);

                    auto b1 = vec_type::loadu(bb + k * n + j + 0 * vec_size);
                    auto b2 = vec_type::loadu(bb + k * n + j + 1 * vec_size);
                    auto b3 = vec_type::loadu(bb + k * n + j + 2 * vec_size);
                    auto b4 = vec_type::loadu(bb + k * n + j + 3 * vec_size);

                    r1 = vec_type::fmadd(a1, b1, r1);
                    r2 = vec_type::fmadd(a1, b2, r2);
                    r3 = vec_type::fmadd(a1, b3, r3);
                    r4 = vec_type::fmadd(a1, b4, r4);
                }

                vec_type::storeu(cc + j + 0 * vec_size, vec_type::add(r1, vec_type::loadu(cc + j + 0 * vec_size)));
                vec_type::storeu(cc + j + 1 * vec_size, vec_type::add(r2, vec_type::loadu(cc + j + 1 * vec_size)));
                vec_type::storeu(cc + j + 2 * vec_size, vec_type::add(r3, vec_type::loadu(cc + j + 2 * vec_size)));
                vec_type::storeu(cc + j + 3 * vec_size, vec_type::add(r4, vec_type::loadu(cc + j + 3 * vec_size)));
            }

            // Base vectorized loop
            for (; j + vec_size - 1 < n_end; j += vec_size) {
                auto r1 = vec_type::template zero<T>();

                for (size_t k = block_k; k < m_end; ++k) {
                    auto a1 = vec_type::set(aa[k]);
                    auto b1 = vec_type::loadu(bb + k * n + j + 0 * vec_size);
                    r1 = vec_type::fmadd(a1, b1, r1);
                }

                vec_type::storeu(cc + j + 0 * vec_size, vec_type::add(r1, vec_type::loadu(cc + j + 0 * vec_size)));
            }

            // Remainder non-vectorized loop
            for (; j < n_end; ++j) {
                auto r1 = T();

                for (size_t k = block_k; k < m_end; ++k) {
                    r1 += aa[k] * bb[k * n + j];
                }

                cc[j] += r1;
            }
        }
    }
}

/*!
 * \brief Optimized version of GEVM for row major version
 * \param a The lhs vector
 * \param b The rhs matrix
 * \param c The result vector
 */
template <typename A, typename B, typename C, cpp_enable_if((all_row_major<A, B, C>::value))>
void gevm(A&& a, B&& b, C&& c) {
    cpp_assert(vec_enabled, "At least one vector mode must be enabled for impl::VEC");

    a.ensure_cpu_up_to_date();
    b.ensure_cpu_up_to_date();

    const auto m = rows(b);
    const auto n = columns(b);

    if(etl::size(b) < gevm_small_threshold){
        gevm_small_kernel<default_vec>(a.memory_start(), m, n, b.memory_start(), c.memory_start());
    } else {
        c = 0;

        gevm_large_kernel<default_vec>(a.memory_start(), m, n, b.memory_start(), c.memory_start());
    }

    c.invalidate_gpu();
}

/*!
 * \brief Unoptimized version of GEVM for column major version
 * \param a The lhs vector
 * \param b The rhs matrix
 * \param c The result vector
 */
template <typename A, typename B, typename C, cpp_disable_if((all_row_major<A, B, C>::value))>
void gevm(A&& a, B&& b, C&& c) {
    cpp_assert(vec_enabled, "At least one vector mode must be enabled for impl::VEC");

    c = 0;

    for (std::size_t k = 0; k < etl::dim<0>(a); k++) {
        for (std::size_t j = 0; j < columns(b); j++) {
            c(j) += a(k) * b(k, j);
        }
    }
}

/*!
 * \brief Optimized version of small GEMM for row major version
 * \param a The lhs matrix
 * \param b The rhs matrix
 * \param c The result matrix
 */
template <typename V, typename T>
void gemm_small_kernel(const T* a, const T* b, T* c, size_t M, size_t N, size_t K) {
    using vec_type = V;

    static constexpr size_t vec_size = vec_type::template traits<T>::size;

    size_t j = 0;

    for (; j + vec_size * 8 - 1 < N; j += vec_size * 8) {
        for (size_t i = 0; i < M; ++i) {
            auto r1 = vec_type::template zero<T>();
            auto r2 = vec_type::template zero<T>();
            auto r3 = vec_type::template zero<T>();
            auto r4 = vec_type::template zero<T>();
            auto r5 = vec_type::template zero<T>();
            auto r6 = vec_type::template zero<T>();
            auto r7 = vec_type::template zero<T>();
            auto r8 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto a1 = vec_type::set(a[i * K + k]);

                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);
                auto b2 = vec_type::loadu(b + k * N + j + vec_size * 1);
                auto b3 = vec_type::loadu(b + k * N + j + vec_size * 2);
                auto b4 = vec_type::loadu(b + k * N + j + vec_size * 3);
                auto b5 = vec_type::loadu(b + k * N + j + vec_size * 4);
                auto b6 = vec_type::loadu(b + k * N + j + vec_size * 5);
                auto b7 = vec_type::loadu(b + k * N + j + vec_size * 6);
                auto b8 = vec_type::loadu(b + k * N + j + vec_size * 7);

                r1 = vec_type::fmadd(a1, b1, r1);
                r2 = vec_type::fmadd(a1, b2, r2);
                r3 = vec_type::fmadd(a1, b3, r3);
                r4 = vec_type::fmadd(a1, b4, r4);
                r5 = vec_type::fmadd(a1, b5, r5);
                r6 = vec_type::fmadd(a1, b6, r6);
                r7 = vec_type::fmadd(a1, b7, r7);
                r8 = vec_type::fmadd(a1, b8, r8);
            }

            vec_type::storeu(c + i * N + j + 0 * vec_size, r1);
            vec_type::storeu(c + i * N + j + 1 * vec_size, r2);
            vec_type::storeu(c + i * N + j + 2 * vec_size, r3);
            vec_type::storeu(c + i * N + j + 3 * vec_size, r4);
            vec_type::storeu(c + i * N + j + 4 * vec_size, r5);
            vec_type::storeu(c + i * N + j + 5 * vec_size, r6);
            vec_type::storeu(c + i * N + j + 6 * vec_size, r7);
            vec_type::storeu(c + i * N + j + 7 * vec_size, r8);
        }
    }

    for (; j + (4 * vec_size) - 1 < N; j += 4 * vec_size) {
        size_t i = 0;

        for (; i + 1 < M; i += 2){
            auto r11 = vec_type::template zero<T>();
            auto r12 = vec_type::template zero<T>();

            auto r21 = vec_type::template zero<T>();
            auto r22 = vec_type::template zero<T>();

            auto r31 = vec_type::template zero<T>();
            auto r32 = vec_type::template zero<T>();

            auto r41 = vec_type::template zero<T>();
            auto r42 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);
                auto b2 = vec_type::loadu(b + k * N + j + vec_size * 1);
                auto b3 = vec_type::loadu(b + k * N + j + vec_size * 2);
                auto b4 = vec_type::loadu(b + k * N + j + vec_size * 3);

                auto a1 = vec_type::set(a[(i + 0) * K + k]);
                auto a2 = vec_type::set(a[(i + 1) * K + k]);

                r11 = vec_type::fmadd(a1, b1, r11);
                r12 = vec_type::fmadd(a2, b1, r12);

                r21 = vec_type::fmadd(a1, b2, r21);
                r22 = vec_type::fmadd(a2, b2, r22);

                r31 = vec_type::fmadd(a1, b3, r31);
                r32 = vec_type::fmadd(a2, b3, r32);

                r41 = vec_type::fmadd(a1, b4, r41);
                r42 = vec_type::fmadd(a2, b4, r42);
            }

            vec_type::storeu(c + (i+0) * N + j + 0 * vec_size, r11);
            vec_type::storeu(c + (i+1) * N + j + 0 * vec_size, r12);

            vec_type::storeu(c + (i+0) * N + j + 1 * vec_size, r21);
            vec_type::storeu(c + (i+1) * N + j + 1 * vec_size, r22);

            vec_type::storeu(c + (i+0) * N + j + 2 * vec_size, r31);
            vec_type::storeu(c + (i+1) * N + j + 2 * vec_size, r32);

            vec_type::storeu(c + (i+0) * N + j + 3 * vec_size, r41);
            vec_type::storeu(c + (i+1) * N + j + 3 * vec_size, r42);
        }

        if (i < M) {
            auto r11 = vec_type::template zero<T>();
            auto r21 = vec_type::template zero<T>();
            auto r31 = vec_type::template zero<T>();
            auto r41 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);
                auto b2 = vec_type::loadu(b + k * N + j + vec_size * 1);
                auto b3 = vec_type::loadu(b + k * N + j + vec_size * 2);
                auto b4 = vec_type::loadu(b + k * N + j + vec_size * 3);

                auto a1 = vec_type::set(a[(i + 0) * K + k]);

                r11 = vec_type::fmadd(a1, b1, r11);
                r21 = vec_type::fmadd(a1, b2, r21);
                r31 = vec_type::fmadd(a1, b3, r31);
                r41 = vec_type::fmadd(a1, b4, r41);
            }

            vec_type::storeu(c + i * N + j + 0 * vec_size, r11);
            vec_type::storeu(c + i * N + j + 1 * vec_size, r21);
            vec_type::storeu(c + i * N + j + 2 * vec_size, r31);
            vec_type::storeu(c + i * N + j + 3 * vec_size, r41);
        }
    }

    for (; j + (2 * vec_size) - 1 < N; j += 2 * vec_size) {
        size_t i = 0;

        for (; i + 3 < M; i += 4) {
            auto r11 = vec_type::template zero<T>();
            auto r12 = vec_type::template zero<T>();
            auto r13 = vec_type::template zero<T>();
            auto r14 = vec_type::template zero<T>();

            auto r21 = vec_type::template zero<T>();
            auto r22 = vec_type::template zero<T>();
            auto r23 = vec_type::template zero<T>();
            auto r24 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);
                auto b2 = vec_type::loadu(b + k * N + j + vec_size * 1);

                auto a1 = vec_type::set(a[(i + 0) * K + k]);
                auto a2 = vec_type::set(a[(i + 1) * K + k]);
                auto a3 = vec_type::set(a[(i + 2) * K + k]);
                auto a4 = vec_type::set(a[(i + 3) * K + k]);

                r11 = vec_type::fmadd(a1, b1, r11);
                r12 = vec_type::fmadd(a2, b1, r12);
                r13 = vec_type::fmadd(a3, b1, r13);
                r14 = vec_type::fmadd(a4, b1, r14);

                r21 = vec_type::fmadd(a1, b2, r21);
                r22 = vec_type::fmadd(a2, b2, r22);
                r23 = vec_type::fmadd(a3, b2, r23);
                r24 = vec_type::fmadd(a4, b2, r24);
            }

            vec_type::storeu(c + (i+0) * N + j + 0 * vec_size, r11);
            vec_type::storeu(c + (i+1) * N + j + 0 * vec_size, r12);
            vec_type::storeu(c + (i+2) * N + j + 0 * vec_size, r13);
            vec_type::storeu(c + (i+3) * N + j + 0 * vec_size, r14);

            vec_type::storeu(c + (i+0) * N + j + 1 * vec_size, r21);
            vec_type::storeu(c + (i+1) * N + j + 1 * vec_size, r22);
            vec_type::storeu(c + (i+2) * N + j + 1 * vec_size, r23);
            vec_type::storeu(c + (i+3) * N + j + 1 * vec_size, r24);
        }

        for (; i + 1 < M; i += 2) {
            auto r11 = vec_type::template zero<T>();
            auto r12 = vec_type::template zero<T>();

            auto r21 = vec_type::template zero<T>();
            auto r22 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);
                auto b2 = vec_type::loadu(b + k * N + j + vec_size * 1);

                auto a1 = vec_type::set(a[(i + 0) * K + k]);
                auto a2 = vec_type::set(a[(i + 1) * K + k]);

                r11 = vec_type::fmadd(a1, b1, r11);
                r12 = vec_type::fmadd(a2, b1, r12);

                r21 = vec_type::fmadd(a1, b2, r21);
                r22 = vec_type::fmadd(a2, b2, r22);
            }

            vec_type::storeu(c + (i+0) * N + j + 0 * vec_size, r11);
            vec_type::storeu(c + (i+1) * N + j + 0 * vec_size, r12);

            vec_type::storeu(c + (i+0) * N + j + 1 * vec_size, r21);
            vec_type::storeu(c + (i+1) * N + j + 1 * vec_size, r22);
        }

        if (i < M) {
            auto r11 = vec_type::template zero<T>();
            auto r21 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);
                auto b2 = vec_type::loadu(b + k * N + j + vec_size * 1);

                auto a1 = vec_type::set(a[(i + 0) * K + k]);

                r11 = vec_type::fmadd(a1, b1, r11);
                r21 = vec_type::fmadd(a1, b2, r21);
            }

            vec_type::storeu(c + (i+0) * N + j + 0 * vec_size, r11);
            vec_type::storeu(c + (i+0) * N + j + 1 * vec_size, r21);
        }
    }

    for (; j + vec_size - 1 < N; j += vec_size) {
        size_t i = 0;

        for (; i + 1 < M; i += 2) {
            auto r1 = vec_type::template zero<T>();
            auto r2 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);

                auto a1 = vec_type::set(a[(i + 0) * K + k]);
                auto a2 = vec_type::set(a[(i + 1) * K + k]);

                r1 = vec_type::fmadd(a1, b1, r1);
                r2 = vec_type::fmadd(a2, b1, r2);
            }

            vec_type::storeu(c + (i+0) * N + j, r1);
            vec_type::storeu(c + (i+1) * N + j, r2);
        }

        if (i < M) {
            auto r1 = vec_type::template zero<T>();

            for (size_t k = 0; k < K; ++k) {
                auto b1 = vec_type::loadu(b + k * N + j + vec_size * 0);

                auto a1 = vec_type::set(a[(i + 0) * K + k]);

                r1 = vec_type::fmadd(a1, b1, r1);
            }

            vec_type::storeu(c + (i+0) * N + j, r1);
        }
    }

    for (; j + 1 < N; j += 2) {
        const size_t j1 = j + 0;
        const size_t j2 = j + 1;

        size_t i = 0;

        for (; i + 1 < M; i += 2) {
            auto r11 = T();
            auto r12 = T();
            auto r21 = T();
            auto r22 = T();

            for (size_t k = 0; k < K; ++k) {
                r11 += a[(i + 0) * K + k] * b[k * N + j1];
                r21 += a[(i + 0) * K + k] * b[k * N + j2];
                r12 += a[(i + 1) * K + k] * b[k * N + j1];
                r22 += a[(i + 1) * K + k] * b[k * N + j2];
            }

            c[(i + 0) * N + j1] = r11;
            c[(i + 0) * N + j2] = r21;
            c[(i + 1) * N + j1] = r12;
            c[(i + 1) * N + j2] = r22;
        }

        if (i < M) {
            auto r1 = T();
            auto r2 = T();

            for (size_t k = 0; k < K; ++k) {
                r1 += a[i * K + k] * b[k * N + j1];
                r2 += a[i * K + k] * b[k * N + j2];
            }

            c[i * N + j1] = r1;
            c[i * N + j2] = r2;
        }
    }

    if (j < N) {
        size_t i = 0;

        for (; i + 1 < M; i += 2) {
            auto r1 = T();
            auto r2 = T();

            for (size_t k = 0; k < K; ++k) {
                r1 += a[(i + 0) * K + k] * b[k * N + j];
                r2 += a[(i + 1) * K + k] * b[k * N + j];
            }

            c[(i + 0) * N + j] = r1;
            c[(i + 1) * N + j] = r2;
        }

        if (i < M) {
            auto r1 = T();

            for (size_t k = 0; k < K; ++k) {
                r1 += a[i * K + k] * b[k * N + j];
            }

            c[i * N + j] = r1;
        }
    }
}

/*!
 * \brief Optimized version of large GEMM for row major version
 * \param a The lhs matrix
 * \param b The rhs matrix
 * \param c The result matrix
 */
template <typename V, typename T>
void gemm_large_kernel(const T* a, const T* b, T* c, size_t M, size_t N, size_t K) {
    using vec_type = V;

    static constexpr size_t vec_size = vec_type::template traits<T>::size;

    const size_t n_block_size = 128;
    const size_t m_block_size = 64;
    const size_t k_block_size = 128;

    for (size_t block_j = 0; block_j < N; block_j += n_block_size) {
        const size_t j_end = std::min(block_j + n_block_size, N);

        for (size_t block_i = 0; block_i < M; block_i += m_block_size) {
            const size_t i_end = std::min(block_i + m_block_size, M);

            for (size_t i = block_i; i < i_end; ++i) {
                for (size_t j = block_j; j < j_end; ++j) {
                    c[i * N + j] = 0;
                }
            }

            for (size_t block_k = 0; block_k < K; block_k += k_block_size) {
                const size_t k_end = std::min(block_k + k_block_size, K);

                size_t j = block_j;

                for (; j + vec_size * 4 - 1 < j_end; j += vec_size * 4) {
                    const size_t j1 = j + vec_size * 1;
                    const size_t j2 = j + vec_size * 2;
                    const size_t j3 = j + vec_size * 3;

                    size_t i = block_i;

                    for (; i + 1 < i_end; i += 2) {
                        auto r11 = vec_type::loadu(c + (i + 0) * N + j);
                        auto r12 = vec_type::loadu(c + (i + 0) * N + j1);
                        auto r13 = vec_type::loadu(c + (i + 0) * N + j2);
                        auto r14 = vec_type::loadu(c + (i + 0) * N + j3);

                        auto r21 = vec_type::loadu(c + (i + 1) * N + j);
                        auto r22 = vec_type::loadu(c + (i + 1) * N + j1);
                        auto r23 = vec_type::loadu(c + (i + 1) * N + j2);
                        auto r24 = vec_type::loadu(c + (i + 1) * N + j3);

                        for (size_t k = block_k; k < k_end; ++k) {
                            auto a1 = vec_type::set(a[(i + 0) * K + k]);
                            auto a2 = vec_type::set(a[(i + 1) * K + k]);

                            auto b1 = vec_type::loadu(b + k * N + j);
                            auto b2 = vec_type::loadu(b + k * N + j1);
                            auto b3 = vec_type::loadu(b + k * N + j2);
                            auto b4 = vec_type::loadu(b + k * N + j3);

                            r11 = vec_type::fmadd(a1, b1, r11);
                            r12 = vec_type::fmadd(a1, b2, r12);
                            r13 = vec_type::fmadd(a1, b3, r13);
                            r14 = vec_type::fmadd(a1, b4, r14);

                            r21 = vec_type::fmadd(a2, b1, r21);
                            r22 = vec_type::fmadd(a2, b2, r22);
                            r23 = vec_type::fmadd(a2, b3, r23);
                            r24 = vec_type::fmadd(a2, b4, r24);
                        }

                        vec_type::storeu(c + (i + 0) * N + j,  r11);
                        vec_type::storeu(c + (i + 0) * N + j1, r12);
                        vec_type::storeu(c + (i + 0) * N + j2, r13);
                        vec_type::storeu(c + (i + 0) * N + j3, r14);
                        vec_type::storeu(c + (i + 1) * N + j,  r21);
                        vec_type::storeu(c + (i + 1) * N + j1, r22);
                        vec_type::storeu(c + (i + 1) * N + j2, r23);
                        vec_type::storeu(c + (i + 1) * N + j3, r24);
                    }

                    if (i < i_end) {
                        auto r1 = vec_type::loadu(c + (i + 0) * N + j);
                        auto r2 = vec_type::loadu(c + (i + 0) * N + j1);
                        auto r3 = vec_type::loadu(c + (i + 0) * N + j2);
                        auto r4 = vec_type::loadu(c + (i + 0) * N + j3);

                        for (size_t k = block_k; k < k_end; ++k) {
                            auto a1 = vec_type::set(a[(i + 0) * K + k]);

                            auto b1 = vec_type::loadu(b + k * N + j);
                            auto b2 = vec_type::loadu(b + k * N + j1);
                            auto b3 = vec_type::loadu(b + k * N + j2);
                            auto b4 = vec_type::loadu(b + k * N + j3);

                            r1 = vec_type::fmadd(a1, b1, r1);
                            r2 = vec_type::fmadd(a1, b2, r2);
                            r3 = vec_type::fmadd(a1, b3, r3);
                            r4 = vec_type::fmadd(a1, b4, r4);
                        }

                        vec_type::storeu(c + (i + 0) * N + j,  r1);
                        vec_type::storeu(c + (i + 0) * N + j1, r2);
                        vec_type::storeu(c + (i + 0) * N + j2, r3);
                        vec_type::storeu(c + (i + 0) * N + j3, r4);
                    }
                }

                for (; j + vec_size * 2 - 1 < j_end; j += vec_size * 2) {
                    const size_t j1(j + vec_size);

                    size_t i = block_i;

                    for (; i + 3 < i_end; i += 4) {
                        auto r11 = vec_type::loadu(c + (i + 0) * N + j);
                        auto r12 = vec_type::loadu(c + (i + 0) * N + j1);

                        auto r21 = vec_type::loadu(c + (i + 1) * N + j);
                        auto r22 = vec_type::loadu(c + (i + 1) * N + j1);

                        auto r31 = vec_type::loadu(c + (i + 2) * N + j);
                        auto r32 = vec_type::loadu(c + (i + 2) * N + j1);

                        auto r41 = vec_type::loadu(c + (i + 3) * N + j);
                        auto r42 = vec_type::loadu(c + (i + 3) * N + j1);

                        for (size_t k = block_k; k < k_end; ++k) {
                            auto a1 = vec_type::set(a[(i + 0) * K + k]);
                            auto a2 = vec_type::set(a[(i + 1) * K + k]);
                            auto a3 = vec_type::set(a[(i + 2) * K + k]);
                            auto a4 = vec_type::set(a[(i + 3) * K + k]);

                            auto b1 = vec_type::loadu(b + k * N + j);
                            auto b2 = vec_type::loadu(b + k * N + j1);

                            r11 = vec_type::fmadd(a1, b1, r11);
                            r12 = vec_type::fmadd(a1, b2, r12);

                            r21 = vec_type::fmadd(a2, b1, r21);
                            r22 = vec_type::fmadd(a2, b2, r22);

                            r31 = vec_type::fmadd(a3, b1, r31);
                            r32 = vec_type::fmadd(a3, b2, r32);

                            r41 = vec_type::fmadd(a4, b1, r41);
                            r42 = vec_type::fmadd(a4, b2, r42);
                        }

                        vec_type::storeu(c + (i + 0) * N + j,  r11);
                        vec_type::storeu(c + (i + 0) * N + j1, r12);
                        vec_type::storeu(c + (i + 1) * N + j,  r21);
                        vec_type::storeu(c + (i + 1) * N + j1, r22);
                        vec_type::storeu(c + (i + 2) * N + j,  r31);
                        vec_type::storeu(c + (i + 2) * N + j1, r32);
                        vec_type::storeu(c + (i + 3) * N + j,  r41);
                        vec_type::storeu(c + (i + 3) * N + j1, r42);
                    }

                    for (; i + 2 - 1 < i_end; i += 2) {
                        auto r11 = vec_type::loadu(c + (i + 0) * N + j);
                        auto r12 = vec_type::loadu(c + (i + 0) * N + j1);

                        auto r21 = vec_type::loadu(c + (i + 1) * N + j);
                        auto r22 = vec_type::loadu(c + (i + 1) * N + j1);

                        for (size_t k = block_k; k < k_end; ++k) {
                            auto a1 = vec_type::set(a[(i + 0) * K + k]);
                            auto a2 = vec_type::set(a[(i + 1) * K + k]);

                            auto b1 = vec_type::loadu(b + k * N + j);
                            auto b2 = vec_type::loadu(b + k * N + j1);

                            r11 = vec_type::fmadd(a1, b1, r11);
                            r12 = vec_type::fmadd(a1, b2, r12);

                            r21 = vec_type::fmadd(a2, b1, r21);
                            r22 = vec_type::fmadd(a2, b2, r22);
                        }

                        vec_type::storeu(c + (i + 0) * N + j,  r11);
                        vec_type::storeu(c + (i + 0) * N + j1, r12);
                        vec_type::storeu(c + (i + 1) * N + j,  r21);
                        vec_type::storeu(c + (i + 1) * N + j1, r22);
                    }

                    if (i < i_end) {
                        auto r1 = vec_type::loadu(c + (i + 0) * N + j);
                        auto r2 = vec_type::loadu(c + (i + 0) * N + j1);

                        for (size_t k = block_k; k < k_end; ++k) {
                            auto a1 = vec_type::set(a[(i + 0) * K + k]);

                            auto b1 = vec_type::loadu(b + k * N + j);
                            auto b2 = vec_type::loadu(b + k * N + j1);

                            r1 = vec_type::fmadd(a1, b1, r1);
                            r2 = vec_type::fmadd(a1, b2, r2);
                        }

                        vec_type::storeu(c + (i + 0) * N + j,  r1);
                        vec_type::storeu(c + (i + 0) * N + j1, r2);
                    }
                }

                for (; j + vec_size - 1 < j_end; j += vec_size) {
                    for (size_t i = block_i; i < i_end; ++i) {
                        auto r1 = vec_type::loadu(c + (i + 0) * N + j);

                        for (size_t k = block_k; k < k_end; ++k) {
                            auto a1 = vec_type::set(a[(i + 0) * K + k]);
                            auto b1 = vec_type::loadu(b + k * N + j);
                            r1 = vec_type::fmadd(a1, b1, r1);
                        }

                        vec_type::storeu(c + (i + 0) * N + j,  r1);
                    }
                }

                for (; j < j_end; ++j) {
                    for (size_t i = block_i; i < i_end; ++i) {
                        auto value = c[i * N + j];

                        for (size_t k = block_k; k < k_end; ++k) {
                            value += a[i * K + k] * b[k * N + j];
                        }

                        c[i * N + j] = value;
                    }
                }
            }
        }
    }
}

/*!
 * \brief Optimized version of GEMM for row major version
 * \param a The lhs matrix
 * \param b The rhs matrix
 * \param c The result matrix
 */
template <typename A, typename B, typename C, cpp_enable_if((all_row_major<A, B, C>::value))>
void gemm(A&& a, B&& b, C&& c) {
    cpp_assert(vec_enabled, "At least one vector mode must be enabled for impl::VEC");

    a.ensure_cpu_up_to_date();
    b.ensure_cpu_up_to_date();

    const size_t M = etl::rows(a);
    const size_t N = etl::columns(b);
    const size_t K = etl::columns(a);

    if(etl::size(b) <= gemm_small_threshold){
        gemm_small_kernel<default_vec>(a.memory_start(), b.memory_start(), c.memory_start(), M, N, K);
    } else {
        gemm_large_kernel<default_vec>(a.memory_start(), b.memory_start(), c.memory_start(), M, N, K);
    }

    c.invalidate_gpu();
}

/*!
 * \brief Unoptimized version of GEMM for column major version
 * \param a The lhs matrix
 * \param b The rhs matrix
 * \param c The result matrix
 */
template <typename A, typename B, typename C, cpp_disable_if((all_row_major<A, B, C>::value))>
void gemm(A&& a, B&& b, C&& c) {
    cpp_assert(vec_enabled, "At least one vector mode must be enabled for impl::VEC");

    c = 0;

    for (std::size_t i = 0; i < rows(a); i++) {
        for (std::size_t k = 0; k < columns(a); k++) {
            for (std::size_t j = 0; j < columns(b); j++) {
                c(i, j) += a(i, k) * b(k, j);
            }
        }
    }
}

} //end of namespace vec

} //end of namespace impl

} //end of namespace etl
