#pragma once

import Free;
import List;
import Util;
import std;

#define STATICALLY_WIDEN(STR)									\
	[] {														\
		if constexpr (std::is_same_v<CharT, char>)				\
		{														\
			return STR;											\
		}														\
		else if constexpr (std::is_same_v<CharT, wchar_t>)		\
		{														\
			return L##STR;										\
		}														\
	}()

// For containers
template <typename T, typename CharT>
struct std::formatter<List<T>, CharT>
{
public:
	std::formatter<T> value_formatter;

	template <typename ParseContext>
	constexpr typename ParseContext::iterator parse(ParseContext& ctx)
	{
		return value_formatter.parse(ctx);
	}

	template <typename FormatContext>
	constexpr typename FormatContext::iterator format(List<T> const& elems, FormatContext& ctx) const
	{
		auto out{ ctx.out() };

		out = std::format_to(out, STATICALLY_WIDEN("["));

		for (std::size_t i{}; i < std::size(elems); ++i)
		{
			out = value_formatter.format(elems[i], ctx);

			if (i < std::size(elems) - 1)
			{
				out = std::format_to(out, STATICALLY_WIDEN(","));
			}
		}

		out = std::format_to(out, STATICALLY_WIDEN("]"));

		return out;
	}
};

template <typename T, typename CharT = char>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, List<T> const& l)
{
	os << "[";

	auto first{ true };

	for (auto const& x : l)
	{
		if (not std::exchange(first, false))
		{
			os << ",";
		}

		os << x;
	}

	os << "]";

	return os;
}

// For Free
template <template <typename...> typename F, typename A, typename CharT>
struct std::formatter<Free::Free<F, A>, CharT>
{
public:
	std::formatter<A> value_formatter;

	template <typename ParseContext>
	constexpr typename ParseContext::iterator parse(ParseContext& ctx)
	{
		return value_formatter.parse(ctx);
	}

	template <typename FormatContext>
	constexpr typename FormatContext::iterator format(Free::Free<F, A> const& free, FormatContext& ctx) const
	{
		auto out{ ctx.out() };

		out = std::format_to(out, STATICALLY_WIDEN("{}"), std::visit(Util::Overloaded{
			[](Free::Return<F, A> const& r) {
				return std::format(STATICALLY_WIDEN("Return{{{}}}"), r.a);
			},
			[](Free::Bind<F, A> const& b) {
				return std::format(STATICALLY_WIDEN("Bind{{{}}}"), b.x);
			}
		}, free.v));

		return out;
	}
};

template <template <typename...> typename F, typename A, typename CharT = char>
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, Free::Free<F, A> const& free)
{
	return std::visit(Util::Overloaded{
		[&os](Free::Return<F, A> const& r) -> std::basic_ostream<CharT>& {
			return os << STATICALLY_WIDEN("Return{") << r.a << STATICALLY_WIDEN("}");
		},
		[&os](Free::Bind<F, A> const& b) -> std::basic_ostream<CharT>& {
			return os << STATICALLY_WIDEN("Bind{") << b.x << STATICALLY_WIDEN("}");
		}
	}, free.v);
}
