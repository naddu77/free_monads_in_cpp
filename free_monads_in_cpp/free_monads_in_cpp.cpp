#include "CustomFormatter.h"
#include <cassert>

import List;
import Free;
import Functor;
import Monad;
import std;

using namespace std::literals;

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

    template <typename Func>
    Prog<std::invoke_result_t<Func, Next>> Fmap(Func&& func) const
    {
        return std::visit(Util::Overloaded{
            [func](Read<Next> const& read) {
                 return Prog<std::invoke_result_t<Func, Next>>{ 
                     Read<std::invoke_result_t<decltype(Compose(func, read.next)), int>>{ Compose(func, read.next) } 
                 };
            },
            [func](Write<Next> const& write) {
                return Prog<std::invoke_result_t<Func, Next>>{ 
                    Write<std::invoke_result_t<decltype(Compose(func, write.next)), Unit>>{ write.x, Compose(func, write.next) }
                };
            }
        }, v);
    }
};

template <typename Next, typename CharT>
struct std::formatter<Prog<Next>, CharT>
{
public:
	std::formatter<Next> value_formatter;

	template <typename ParseContext>
	constexpr typename ParseContext::iterator parse(ParseContext& ctx)
	{
		return value_formatter.parse(ctx);
	}

	template <typename FormatContext>
	constexpr typename FormatContext::iterator format(Prog<Next> const& prog, FormatContext& ctx) const
	{
		auto out{ ctx.out() };

		out = std::format_to(out, STATICALLY_WIDEN("{}"), std::visit(Util::Overloaded{
            [](Read<Next> const& read) {
                return "Read{}"s;
            },
            [](Write<Next> const& write) {
                return std::format(STATICALLY_WIDEN("Write{{{}}}"), write.x);
            }
        }, prog.v));

		return out;
	}
};

template <typename Next, typename CharT = char>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, Prog<Next> const& prog)
{
	return os << std::format(STATICALLY_WIDEN("{}"), prog);
}

template <typename A>
struct Program
    : Free::Free<Prog, A>
{

};

template <typename A, typename CharT>
struct std::formatter<Program<A>, CharT>
{
public:
	std::formatter<A> value_formatter;

	template <typename ParseContext>
	constexpr typename ParseContext::iterator parse(ParseContext& ctx)
	{
		return value_formatter.parse(ctx);
	}

	template <typename FormatContext>
	constexpr typename FormatContext::iterator format(Program<A> const& program, FormatContext& ctx) const
	{
		auto out{ ctx.out() };

		out = std::format_to(out, STATICALLY_WIDEN("{}"), static_cast<Free::Free<Prog, A>>(program));

		return out;
	}
};

template <template <typename...> typename F, typename A, typename CharT = char>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, Program<A> const& program)
{
	return os << std::format(STATICALLY_WIDEN("{}"), program);
}

// readF :: Free (Prog Int)
// readF = liftFree (Read Id)
Program<int> ReadF{ Free::LiftFree(Prog<std::invoke_result_t<decltype(&Id<int>), int>>{ Read<std::invoke_result_t<decltype(&Id<int>), int>>{ &Id<int> }}) };

// writeF :: Int -> Free (Prog ())
// writeF x = liftFree (Write x id)
Program<Unit> WriteF(int x)
{
    return { Free::LiftFree(Prog<std::invoke_result_t<decltype(&Id<Unit>), Unit>>{ Write<std::invoke_result_t<decltype(&Id<Unit>), Unit>>{ x, &Id<Unit> }}) };
}

// Interpret :: Prog a -> List a
// Interpret (Read n) = Fmap n STATE
// Interpret (Write x n) = STATE += x; n ()
struct Interpreter
{
    List<int> l;    // the values written so far

    template <typename A>
    List<A> operator()(Prog<A> const& prog)
    {
        return std::visit(Util::Overloaded{
            [this](Read<A> const& r) ->List<A> {
                return Functor::Fmap(r.next, l);
            },
            [this](Write<A> const& w) -> List<A> {
                l.push_back(w.x);

                return { w.next(Unit{}) };
            }
        }, prog.v);
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

static_assert(Functor::Functorable<Prog>);
static_assert(Functor::Functorable<Program>);
static_assert(Functor::Functorable<List>);
static_assert(not Functor::Functorable<std::vector>);
static_assert(not Monad::Monadable<Prog>);
static_assert(Monad::Monadable<Program>);
static_assert(Monad::Monadable<List>);
static_assert(not Monad::Monadable<std::vector>);

int main()
{
    List<int> const l{ 1, 2, 3 };

    // fmap
    {
        auto l0{ Functor::Fmap([](int x) { return x + 1; }, l) };

        assert((l0 == List<int>{ 2, 3, 4}));
        assert((Functor::Fmap([](int x) { return -x; }, l) == List<int>{ -1, -2, -3 }));
    }

    // pure
    {
        assert((Monad::Pure<List>("Hi"s) == List<std::string>{ "Hi" }));
    }

    // bind
    {
        assert((
            Monad::MBind(l, [](int x) {
                return Monad::MBind(Monad::Pure<List>(x * x), [x](int y) { return List<int>{ x, y }; });
            }) == List<int>{ 1, 1, 2, 4, 3, 9 }
        ));

        assert((
            Monad::MBind(l, [](int x) {
                return Monad::MBind(Monad::Pure<List>(x * x), [x](int y) { return List<int>{ x, y }; });
            }) == List<int>{ 1, 1, 2, 4, 3, 9 }
        ));

        assert((
            (l >>= [](int x) {
                return Monad::Pure<List>(x * x) >>= [x](int y) { return List<int>{ x, y }; };
            }) == List<int>{ 1, 1, 2, 4, 3, 9 }
        ));

        auto l1{ Monad::MBind(l, [](int x) {
            return Monad::MBind(Monad::Pure<List>(x * x), [x](int y) {
                return List<int>{ x, y };
            });
        }) };

        assert((l1 == List<int>{ 1, 1, 2, 4, 3, 9 }));
        
        auto l2{ l >>= [](int x) {
            return Monad::Pure<List>(x * 2) >>= [&](int y) { return List<int>{ x, y }; };
        } };

        assert((l2 == List<int>{ 1, 2, 2, 4, 3, 6 }));

        std::println("{}", l2);
    }    

    // Free monad for prog
    {
        auto pure{ [](auto&& x) { return Monad::Pure<Program>(x); } };        

        // do
        //  readF
        {
            auto free{ ReadF };

            std::println("{}", free);           

            assert((Run(free) == List<int>{}));

            std::println("{}", Run(free));
        }

        // do
        //  x <- readF
        //  pure (Show x)
        {
            auto free{ 
                ReadF >>= [pure](int x) { return 
                pure(std::to_string(x)); 
            } };

            std::println("{}", free);

            assert((Run(free) == List<std::string>{}));

            std::println("{}", Run(free));
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

            std::println("{}", free);

            assert((Run(free) == List<int>{ 20, 30 }));

            std::println("{}", Run(free));
        }
    }
}
