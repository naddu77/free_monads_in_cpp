export module List;

import Functor;
import Monad;
import std;

using namespace std::literals;

export template <typename A>
struct List
	: std::vector<A>
{
	using std::vector<A>::vector;
};

export template <typename A>
std::ostream& operator<<(std::ostream& os, List<A> const& l)
{
	return os << '[' << (l | std::views::transform([](auto const& e) { return std::format("{}", e); }) | std::views::join_with(","s) | std::ranges::to<std::string>()) << ']';
}

export namespace Functor
{
	template <>
	struct Functor<List>
	{
		// fmap :: (a -> b) -> List a -> List b
		template <typename A, std::invocable<A> Func>
		static auto Fmap(Func&& func, List<A> const& l)
		{
			return l | std::views::transform(std::forward<Func>(func)) | std::ranges::to<List<std::invoke_result_t<Func, A>>>();
		}
	};

	static_assert(IsFunctor<List>, "List should be a Functor.");
}

export namespace Monad
{
	template <>
	struct Monad<List>
	{
		// pure :: a -> List a
		template <typename A>
		static List<A> Pure(A const& x)
		{
			return List<A>({ x });
		}

		// bind :: List a -> (a -> List b) -> List b
		template <typename A, std::invocable<A> Func>
		static std::invoke_result_t<Func, A> Bind(List<A> const& l, Func&& func)
		{
			return l | std::views::transform(std::forward<Func>(func)) | std::views::join | std::ranges::to<std::invoke_result_t<Func, A>>();
		}
	};
}
