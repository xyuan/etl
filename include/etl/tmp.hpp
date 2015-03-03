//=======================================================================
// Copyright (c) 2014-2015 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef ETL_TMP_HPP
#define ETL_TMP_HPP

namespace etl {

template<typename E>
using value_t = typename std::decay_t<E>::value_type;

template<std::size_t F, std::size_t... Dims>
struct mul_all final : std::integral_constant<std::size_t, F * mul_all<Dims...>::value> {};

template<std::size_t F>
struct mul_all<F> final : std::integral_constant<std::size_t, F> {};

template<std::size_t S, std::size_t I, std::size_t F, std::size_t... Dims>
struct nth_size final {
    template<std::size_t S2, std::size_t I2, typename Enable = void>
    struct nth_size_int : std::integral_constant<std::size_t, nth_size<S, I+1, Dims...>::value> {};

    template<std::size_t S2, std::size_t I2>
    struct nth_size_int<S2, I2, std::enable_if_t<S2 == I2>> : std::integral_constant<std::size_t, F> {};

    static constexpr const std::size_t value = nth_size_int<S, I>::value;
};


template<typename... Dims>
std::string concat_sizes(Dims... sizes){
    std::array<std::size_t, sizeof...(Dims)> tmp{{sizes...}};
    std::string result;
    std::string sep;
    for(auto& v : tmp){
        result += sep;
        result += std::to_string(v);
        sep = ",";
    }
    return result;
}

} //end of namespace etl

#endif
