export module Monad;

import Functor;
import Util;
import std;

namespace Monad
{
    namespace Internal
    {
        template <template <typename...> typename M>
        M<Util::Dummy2> MonadFunction(Util::Dummy1 const&)
        {
            return {};
        }
    }

    // class (Functor m) => Monad m where
    //  pure :: a - > m a
    //  bind :: m a -> (a -> m b) -> m b
    export template <template <typename...> typename M>
    concept Monadable = requires (M<Util::Dummy1> t) {
        { t.MBind(&Internal::MonadFunction<M>) } -> std::same_as<M<Util::Dummy2>>;
    };

    // pure :: (Monad m) => a -> m a
    export template <template <typename> typename M, typename A>
    requires Monadable<M>
    M<A> Pure(A const& x)
    {
        return { x };
    }

    // bind :: (Monad m) => m a -> (a -> m b) -> m b
    export template <template <typename> typename M, typename A, std::invocable<A> Func>
    requires Monadable<M>
    std::invoke_result_t<Func, A> MBind(M<A> const& m, Func&& func)
    {
        return m.MBind(std::forward<Func>(func));
    }    
}

// (>>=) :: (Monad m) => m a -> (a -> m b) -> m b
// m a >>= f = f a
export template <template <typename> typename M, typename A, std::invocable<A> Func>
requires Monad::Monadable<M>
auto operator>>=(M<A> const& m, Func&& func)
{
    return m.MBind(std::forward<Func>(func));
}

// (>>) :: (Monad m) => m a -> m () -> m ()
// x >> y = x >>= \_ -> y
export template <template <typename> typename M, typename A, typename B>
requires Monad::Monadable<M>
M<B> operator>>(M<A> const& m, M<B> const& v)
{
    return m.MBind([=](auto&&) { return v; });
}
