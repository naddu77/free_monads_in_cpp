export module Util;

export namespace Util
{
	struct Dummy1{};
	struct Dummy2{};

	template <typename... Ts>
		struct Overloaded
		: Ts...
	{

	};

	template <typename T>
	struct ValueTypeGetter;

	template <template <typename...> typename M, typename A>
	struct ValueTypeGetter<M<A>>
	{
		using value_type = A;
	};
}
