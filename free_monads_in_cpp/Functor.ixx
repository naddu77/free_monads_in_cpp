export module Functor;

import Util;
import std;

namespace Functor
{
    // class Functor f where
    // map :: (a -> b) -> f a -> f b
    export template <template <typename...> typename F>
    concept Functorable = requires (F<Util::Dummy1> t) {
        { t.Fmap([](Util::Dummy1 const&) -> Util::Dummy2 { return {}; }) };
    };

    export template <template <typename> typename F, typename A, std::invocable<A> Func>
    requires Functorable<F>
    F<std::invoke_result_t<Func, A>> Fmap(Func&& func, F<A> const& fa)
    {
        return fa.Fmap(std::forward<Func>(func));
    }

    namespace Test
    {
        template <typename A>
        struct NullFunctor
        {
            template <std::invocable<A> Func>
            NullFunctor<std::invoke_result_t<Func, A>> Fmap(Func&&)
            {
                return {};
            }
        };

        static_assert(Functorable<Test::NullFunctor>, "NullFunctor must be Functor.");
    }
}
