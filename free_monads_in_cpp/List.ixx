export module List;

import Functor;
import Monad;
import std;

using namespace std::literals;

export template <typename A>
struct List
	: std::vector<A>
{
	// pure :: a -> List a
	using std::vector<A>::vector;

	// fmap :: (a -> b) -> List a -> List b
	template <std::invocable<A> Func>
	auto Fmap(Func&& func) const
	{
		return *this 
			| std::views::transform(std::forward<Func>(func)) 
			| std::ranges::to<List<std::invoke_result_t<Func, A>>>();
	}	

	// bind :: List a -> (a -> List b) -> List b
	template <std::invocable<A> Func>
	auto MBind(Func&& func) const
	{
		return *this 
			| std::views::transform(std::forward<Func>(func)) 
			| std::views::join
			| std::ranges::to<std::invoke_result_t<Func, A>>();
	}
};

static_assert(Functor::Functorable<List>, "List should be a Functor.");
static_assert(Monad::Monadable<List>, "List should be a Monad.");
