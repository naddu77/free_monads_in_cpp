#include "Free.h"
#include "List.h"
#include <iostream>
#include <functional>
#include <string>
#include <cassert>
#include <print>

using Unit = std::tuple<>;

std::ostream& operator<<(std::ostream& os, Unit)
{
    return os << "Unit{}";
}

// id :: a -> a
// id x = x
template <typename A>
A Id(A a)
{
    
    return a;
}

// compose :: (b ->c) -> (a -> b) -> a - > c
// compose f g = \w -> f (g x)
template <typename F, typename G>
auto Compose(F&& f, G&& g)
{
    return [=]<typename X>(X&& x) { return f(g(std::forward<X>(x))); };
}

// data Prog a =
//      Read (Int -> a)
//  |   Write Int (() -> a)
template <typename Next>
struct Read
{
    std::function<Next(int)> next;
};

template <typename Next>
struct Write
{
    int x;
    std::function<Next(Unit)> next;
};

template <typename Next>
struct Prog
{
    std::variant<Read<Next>, Write<Next>> v;
};

template <typename F>
Prog<std::invoke_result_t<F, int>> MakeRead(F next)
{
    return { Read<std::invoke_result_t<F, int>>{ next } };
}

template <typename F>
Prog<std::invoke_result_t<F, Unit>> MakeWrite(int x, F next)
{
    return { Write<std::invoke_result_t<F, Unit>>{ x, next } };
}

namespace Functor
{
    template <>
    struct Functor<Prog>
    {
        template <typename Fun, typename Next>
        static Prog<std::invoke_result_t<Fun, Next>> Fmap(Fun&& fun, Prog<Next> const& prog)
        {
            using Res = std::invoke_result_t<Fun, Next>;

            struct Visitor
            {
                Fun& fun;

                // Fmap f (Read n) = Read (f . n)
                Prog<Res> operator()(Read<Next> const& read) const
                {
                    return MakeRead(Compose(fun, read.next));
                }

                // Famp f (Write x n) = Write x (f . n)
                Prog<Res> operator()(Write<Next> const& write) const
                {
                    return MakeWrite(write.x, Compose(fun, write.next));
                }
            };

            return std::visit(Visitor{ fun }, prog.v);
        }
    };
}

template <typename A>
struct Program
    : Free::Free<Prog, A>
{
    using WrappedFree = Free::Free<Prog, A>;

    Program(WrappedFree const& free)
        : Free::Free<Prog, A>(free)
    {

    }
};

// readF :: Free (Prog Int)
// readF = liftFree (Read Id)
Program<int> ReadF = Free::LiftFree(MakeRead(&Id<int>));

// writeF :: Int -> Free (Prog ())
// writeF x = liftFree (Write x id)
Program<Unit> WriteF(int x)
{
    return Free::LiftFree(MakeWrite(x, &Id<Unit>));
}

// Interpret :: Prog a -> List a
// Interpret (Read n) = Fmap n STATE
// Interpret (Write x n) = STATE += x; n ()
struct Interpreter
{
    List<int> l;    // the values written so far

    template <typename A>
    List<A> operator()(Prog<A> prog)
    {
        struct Visitor
        {
            List<int>& l;

            List<A> operator()(Read<A> const& r)
            {
                return Functor::Fmap(r.next, l);
            }

            List<A> operator()(Write<A> const& w)
            {
                l.push_back(w.x);

                return { w.next(Unit{}) };
            }
        };
       
        Visitor v{ l };

        return std::visit(v, prog.v);
    }
};

// run :: Program a -> List a
// run p = foldFree Interpret p
template <typename A>
List<A> Run(Program<A> program)
{
    Interpreter i;

    return Free::FoldFree<List>(i, program);
}

int main()
{
    List<int> const l{ 1, 2, 3 };

    // fmap
    {
        auto l0{ Fmap([](int x) { return x + 1; }, l) };

        assert((l0 == List<int>{ 2, 3, 4}));
        assert((Fmap([](int x) { return -x; }, l) == List<int>{ -1, -2, -3 }));
    }

    // pure
    {
        assert((Pure<List>("Hi"s) == List<std::string>{ "Hi" }));
    }

    // bind
    {
        assert((
            Bind(l, [](int x) {
                return Bind(Pure<List>(x * x), [x](int y) { return List<int>{ x, y }; });
            }) == List<int>{ 1, 1, 2, 4, 3, 9 }
        ));

        assert((
            Bind(l, [](int x) {
                return Bind(Pure<List>(x * x), [x](int y) { return List<int>{ x, y }; });
            }) == List<int>{ 1, 1, 2, 4, 3, 9 }
        ));

        assert((
            (l >>= [](int x) {
                return Pure<List>(x * x) >>= [x](int y) { return List<int>{ x, y }; };
            }) == List<int>{ 1, 1, 2, 4, 3, 9 }
        ));

        auto l1{ Bind(l, [](int x) {
            return Bind(Pure<List>(x * x), [x](int y) {
                return List<int>{ x, y };
            });
        }) };

        assert((l1 == List<int>{ 1, 1, 2, 4, 3, 9 }));
        
        auto l2{ l >>= [](int x) {
            return Pure<List>(x * 2) >>= [&](int y) { return List<int>{ x, y }; };
        } };

        assert((l2 == List<int>{ 1, 2, 2, 4, 3, 6 }));

        std::println("{}", l2);
    }

    // Free monad for prog
    {
        auto pure{ [](auto&& x) { return Pure<Program>(x); } };

        // do
        //  readF
        {
            auto free{ ReadF };

            assert((Run(free) == List<int>{}));
        }

        // do
        //  x <- readF
        //  pure (Show x)
        {
            auto free{ 
                ReadF >>= [pure](int x) { return 
                pure(std::to_string(x)); 
            } };

            assert((Run(free) == List<std::string>{}));
        }

        // do
        //  wirteF 10
        //  x <- readF
        //  wirteF 20
        //  y <- readF
        //  pure (x + y)
        {
            auto free{
                WriteF(10) >>
                ReadF >>= [=](int x) { return
                WriteF(20) >>
                ReadF >>= [=](int y) { return
                pure(x + y);
            }; } };

            assert((Run(free) == List<int>{ 20, 30 }));
        }
    }
}
