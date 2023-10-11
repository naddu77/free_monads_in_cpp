#pragma once

#include "Functor.h"
#include "Monad.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <vector>
#include <ranges>
#include <string_view>
#include <sstream>

using namespace std::literals;

template <typename A>
struct List
	: std::vector<A>
{
	using std::vector<A>::vector;
};

template <typename A>
std::ostream& operator<<(std::ostream& os, List<A> const& l)
{
	return os << '[' << (l | std::views::transform([](auto const& e) { return std::format("{}", e); }) | std::views::join_with(","s) | std::ranges::to<std::string>()) << ']';
}

template <typename A>
struct std::formatter<List<A>>
	: std::formatter<std::string>
{
	template<class FormatContext>
	auto format(List<A> l, FormatContext& fc) const
	{
		auto out{ fc.out() };

		*out++ = '[';

		std::ranges::copy(l | std::views::transform([](auto const& e) { return std::format("{}", e); }) | std::views::join_with(","s) | std::ranges::to<std::string>(), out);

		*out++ = ']';

		return out;
	}
};

namespace Functor
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

namespace Monad
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