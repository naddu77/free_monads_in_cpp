#pragma once

#include <type_traits>
#include <concepts>
#include <iostream>

namespace Functor
{
    // class Functor f where
    //  map :: (a -> b) -> f a -> f b
    template <template <typename> typename F, typename Enable = void>
    struct Functor;

    namespace Internal
    {
        template <template <typename> typename F>
        struct IsFunctorType
            : std::false_type
        {
            // Note: Nothing
        };

        struct Dummy1 {};
        struct Dummy2 {};

        template <template <typename> typename F>
            requires requires
        {
            { Functor<F>::Fmap(std::declval<Dummy2(Dummy1)>(), std::declval<F<Dummy1>>()) } -> std::same_as<F<Dummy2>>;
        }
        struct IsFunctorType<F>
            : std::true_type
        {
            // Note: Nothing
        };
    }

    template <template <typename> typename F>
    constexpr bool IsFunctor = Internal::IsFunctorType<F>::value;

    template <template <typename> typename F, typename A, std::invocable<A> Fun>
    requires IsFunctor<F>
    F<std::invoke_result_t<Fun, A>> Fmap(Fun&& fun, F<A> const& fa)
    {
        return Functor<F>::Fmap(std::forward<Fun>(fun), fa);
    }

    namespace Test
    {
        template <typename A>
        struct NullFunctor
        {
            // Note: Nothing
        };

    }

    template <>
    struct Functor<Test::NullFunctor>
    {
        template <typename A, std::invocable<A> Fun>
        static Test::NullFunctor<std::invoke_result_t<Fun, A>> Fmap(Fun, Test::NullFunctor<A>)
        {
            return {};
        }
    };

    static_assert(IsFunctor<Test::NullFunctor>, "NullFunctor must be a Functor.");
}

using Functor::Fmap;
