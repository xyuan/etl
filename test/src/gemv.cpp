//=======================================================================
// Copyright (c) 2014-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "test.hpp"
#include "etl/stop.hpp"

#include "mmul_test.hpp"

// Matrix-Vector Multiplication

GEMV_TEST_CASE("gemv/0", "[gemv]") {
    etl::fast_matrix<T, 2, 3> a = {1, 2, 3, 4, 5, 6};
    etl::fast_vector<T, 3> b    = {7, 8, 9};
    etl::fast_matrix<T, 2> c;

    Impl::apply(a, b, c);

    REQUIRE_EQUALS(c(0), 50);
    REQUIRE_EQUALS(c(1), 122);
}

GEMV_TEST_CASE("gemv/1", "[gemv]") {
    etl::fast_matrix<T, 2, 5> a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    etl::fast_vector<T, 5> b    = {7, 8, 9, 10, 11};
    etl::fast_matrix<T, 2> c;

    Impl::apply(a, b, c);

    REQUIRE_EQUALS(c(0), 145);
    REQUIRE_EQUALS(c(1), 370);
}

GEMV_TEST_CASE("gemv/2", "[gemv]") {
    etl::dyn_matrix<T> a(2, 3, etl::values(1, 2, 3, 4, 5, 6));
    etl::dyn_vector<T> b(3, etl::values(7, 8, 9));
    etl::dyn_vector<T> c(2);

    Impl::apply(a, b, c);

    REQUIRE_EQUALS(c(0), 50);
    REQUIRE_EQUALS(c(1), 122);
}

GEMV_TEST_CASE("gemv/3", "[gemv]") {
    etl::dyn_matrix<T> a(2, 5, etl::values(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
    etl::dyn_vector<T> b(5, etl::values(7, 8, 9, 10, 11));
    etl::dyn_vector<T> c(2);

    Impl::apply(a, b, c);

    REQUIRE_EQUALS(c(0), 145);
    REQUIRE_EQUALS(c(1), 370);
}

GEMV_TEST_CASE("gemv/4", "[gemv]") {
    etl::dyn_matrix<T> a(512, 512);
    etl::dyn_vector<T> b(512);

    etl::dyn_vector<T> c(512);
    etl::dyn_vector<T> c_ref(512);

    a = 0.01 * etl::sequence_generator(1.0);
    b = -0.032 * etl::sequence_generator(1.0);

    Impl::apply(a, b, c);

    c_ref = 0;

    for (size_t i = 0; i < 512; i++) {
        for (size_t k = 0; k < 512; k++) {
            c_ref(i) += a(i, k) * b(k);
        }
    }

    for(size_t i = 0; i < etl::size(c); ++i){
        REQUIRE_EQUALS_APPROX(c[i], c_ref[i]);
    }
}

GEMV_TEST_CASE("gemv/5", "[gemv]") {
    etl::dyn_matrix<T> a(512, 368);
    etl::dyn_vector<T> b(368);

    etl::dyn_vector<T> c(512);
    etl::dyn_vector<T> c_ref(512);

    a = 0.01 * etl::sequence_generator(1.0);
    b = -0.032 * etl::sequence_generator(1.0);

    Impl::apply(a, b, c);

    c_ref = 0;

    for (size_t i = 0; i < 512; i++) {
        for (size_t k = 0; k < 368; k++) {
            c_ref(i) += a(i, k) * b(k);
        }
    }

    for(size_t i = 0; i < etl::size(c); ++i){
        REQUIRE_EQUALS_APPROX(c[i], c_ref[i]);
    }
}

GEMV_TEST_CASE("gemv/6", "[gemv]") {
    etl::dyn_matrix<T> a(368, 512);
    etl::dyn_vector<T> b(512);

    etl::dyn_vector<T> c(368);
    etl::dyn_vector<T> c_ref(368);

    a = 0.01 * etl::sequence_generator(1.0);
    b = -0.032 * etl::sequence_generator(1.0);

    Impl::apply(a, b, c);

    c_ref = 0;

    for (size_t i = 0; i < 368; i++) {
        for (size_t k = 0; k < 512; k++) {
            c_ref(i) += a(i, k) * b(k);
        }
    }

    for(size_t i = 0; i < etl::size(c); ++i){
        REQUIRE_EQUALS_APPROX(c[i], c_ref[i]);
    }
}
