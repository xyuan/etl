//=======================================================================
// Copyright (c) 2014 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef ETL_FAST_DYN_VECTOR_HPP
#define ETL_FAST_DYN_VECTOR_HPP

#include<vector>

#include "fast.hpp"

namespace etl {

template<typename T, std::size_t Rows>
using fast_dyn_vector = fast_matrix_impl<T, std::vector<T>, Rows>;

} //end of namespace etl

#endif
