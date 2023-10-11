#pragma once

#include "Functor.h"
#include <type_traits>

namespace Monad
{
    // class (Functor m) => Monad m where
    //  pure :: a - > m a
    //  bind :: m a -> (a -> m b) -> m b
    template <template <typename> typename M, typename Enable = void>
    struct Monad;

    namespace Internal
    {
        template <template <typename> typename M>
        struct IsMonadType
            : std::false_type
        {
            // Note: Nothing
        };

        template <template <typename> typename M>
            requires requires
        {
            { Monad<M>{} };
            { Functor::IsFunctor<M> };
        }
        struct IsMonadType<M>
            : std::true_type
        {
            // Note: Nothing
        };
    }

    template <template <typename> typename M>
    constexpr bool IsMonad = Internal::IsMonadType<M>::value;

    static_assert(!IsMonad<Functor::Test::NullFunctor>, "NullFunctor is a Functor.");

    // pure :: (Monad m) => a -> m a
    template <template <typename> typename M, typename A>
    requires IsMonad<M>
    M<A> Pure(A const& x)
    {
        return Monad<M>::Pure(x);
    }

    // bind :: (Monad m) => m a -> (a -> m b) -> m b
    template <template <typename> typename M, typename A, std::invocable<A> F>
    requires IsMonad<M>
    std::invoke_result_t<F, A> Bind(M<A> const& m, F&& f)
    {
        return Monad<M>::Bind(m, std::forward<F>(f));
    }
}

// (>>=) :: (Monad m) => m a -> (a -> m b) -> m b
// m a >>= f = f a
template <template <typename> typename M, typename A, std::invocable<A> F>
requires Monad::IsMonad<M>
auto operator>>=(M<A> const& m, F&& f)
{
    return Monad::Bind(m, std::forward<F>(f));
}

// (>>) :: (Monad m) => m a -> m () -> m ()
// x >> y = x >>= \_ -> y
template <template <typename> typename M, typename A, typename B>
requires Monad::IsMonad<M>
M<B> operator>>(M<A> const& m, M<B> const& v)
{
    return Monad::Bind(m, [=](auto&&) { return v; });
}

using Monad::Pure;
using Monad::Bind;