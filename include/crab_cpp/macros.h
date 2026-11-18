#pragma once

#define CRAB_CPP_PP_CAT_IMPL(a, b) a##b
#define CRAB_CPP_PP_CAT(a, b) CRAB_CPP_PP_CAT_IMPL(a, b)

#define CRAB_MATCH(...) ::crab_cpp::internal::matcher{__VA_ARGS__} ->* ::crab_cpp::internal::overload

#define CRAB_CPP_TRY_IMPL(var_name, decl, ...) \
    auto&& var_name = (__VA_ARGS__); \
    if (::crab_cpp::internal::try_is_err(var_name)) \
    { \
        return ::crab_cpp::internal::try_propagate_err(static_cast<decltype(var_name)&&>(var_name)); \
    } \
    decl = ::crab_cpp::internal::try_unwrap_unchecked(static_cast<decltype(var_name)&&>(var_name))

#define CRAB_CPP_TRY_VOID_IMPL(var_name, ...) \
    do \
    { \
        auto&& var_name = (__VA_ARGS__); \
        if (::crab_cpp::internal::try_is_err(var_name)) \
        { \
            return ::crab_cpp::internal::try_propagate_err(static_cast<decltype(var_name)&&>(var_name)); \
        } \
    } \
    while (false)

#define CRAB_TRY(decl, ...) CRAB_CPP_TRY_IMPL(CRAB_CPP_PP_CAT(_crab_cpp_try_, __COUNTER__), decl, __VA_ARGS__)

#define CRAB_TRY_VOID(...) CRAB_CPP_TRY_VOID_IMPL(CRAB_CPP_PP_CAT(_crab_cpp_try_, __COUNTER__), __VA_ARGS__)

#ifndef CRAB_CPP_NO_SHORT_MACROS
#define match(...) CRAB_MATCH(__VA_ARGS__)
#endif
