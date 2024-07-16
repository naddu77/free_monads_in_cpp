export module Free;

import Functor;
import Monad;
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

export namespace Free
{
	template <typename Func, typename A>
	concept FreeMonadFunctionType = requires {
		typename std::invoke_result_t<Func, A>::ContainedType;
	};

	// data Free f a = Return a | Bind (f (Free f a))
	// instance (Functor f) => Monad (Free f) where
	// ...
	template <template <typename> typename F, typename A>
	struct Return
	{
		A a;
	};

	template <template <typename> typename F, typename A>
	struct Bind;

	template <template <typename> typename F, typename A>
	requires Functor::Functorable<F>
	struct Free
	{
		using ContainedType = A;
		using ReturnType = Return<F, A>;

		std::variant<Return<F, A>, Bind<F, A>> v;

		// fmap :: (a -> b) -> Free f a -> Free f b
		template <std::invocable<A> Func>
		Free<F, std::invoke_result_t<Func, A>> Fmap(Func&& func) const
		{
			return std::visit(Util::Overloaded{
				[&func](Return<F, A> const& r) -> Free<F, std::invoke_result_t<Func, A>> {
					return { func(r.a) };
				},
				[&func](Bind<F, A> const& b) -> Free<F, std::invoke_result_t<Func, A>> {
					return { b.x.Fmap([&](auto const& f) { return f.Fmap(func); }) };
				}
			}, v);
		}

		// pure :: a -> m a
		// pure :: a -> Free<F>
		Free(A&& a)
			: v{ Return<F, A>{ std::forward<A>(a) } }
		{

		}

		Free(A const& a)
			: v{ Return<F, A>{ a } }
		{

		}

		Free(F<Free<F, A>> const& x)
			: v{ Bind<F, A>{ x } }
		{

		}

		Free(F<Free<F, A>>&& x)
			: v{ Bind<F, A>{ std::move(x) } }
		{

		}

		~Free() = default;

		// bind :: Free f a -> (a -> Free f b) -> Free f b
		template <FreeMonadFunctionType<A> Func>
		struct BindVisitor
		{
			using ResultType = std::invoke_result_t<Func, A>;
			using B = typename ResultType::ContainedType;

			Func&& func;

			// bind (Bind x) f = Bind (Fmap (\m -> bind m f) x)
			template <template <typename> typename F>
			ResultType operator()(Bind<F, A> const& b) const
			{
				return { b.x.Fmap(
					[func = std::move(func)](Free<F, A> const& m) {
						return static_cast<Free<F, B>>(m.MBind(func));
					}) };
			}

			// bind (Return r) f = f r
			template <template <typename> typename F>
			ResultType operator()(Return<F, A> const& r) const
			{
				return func(r.a);
			}
		};

		template <FreeMonadFunctionType<A> Func>
		auto MBind(Func&& func) const
		{
			return std::visit(BindVisitor{ std::forward<Func>(func) }, v);
		}
	};

	template <template <typename> typename F, typename A>
	struct Bind
	{
		F<Free<F, A>> x;
	};

	// liftFree :: (Functor f) => f a -> Free f a
	// liftFree x = bind (fmap Return x)
	template <template <typename> typename F, typename A>
	requires Functor::Functorable<F>
	auto LiftFree(F<A> const& x)
	{
		return x.Fmap([](A const& x) { return Free<F, A>{ x }; });
	}

	// foldFree :: Monad m => (forall x . f x) -> Free f a -> m a
	// foldFree _ (Return a) = return a
	// foldFree f (Bind as) = f as >>= FoldFree f
	template <template <typename> typename M, template <typename> typename F, typename A, std::invocable<F<Free<F, A>>> Fun>
	requires Monad::Monadable<M>
	M<A> FoldFree(Fun fun, Free<F, A> const& free)
	{
		return std::visit(Util::Overloaded{
			[&fun](Return<F, A> const& r) {
				return Monad::Pure<M>(r.a);
			},
			[&fun](Bind<F, A> const& b) {
				return fun(b.x) >>= [&](auto const& x) { return FoldFree<M>(fun, x); };
			}
		}, free.v);
	}
}

namespace Free
{
	namespace Test
	{
		template <typename A>
		struct NullFunctor
		{
			template <std::invocable<A> Func>
			NullFunctor<std::invoke_result_t<Func, A>> Fmap(Func&&) const
			{
				return {};
			}
		};
	}

	template <typename A>
	struct NullMonad
		: Free::Free<Test::NullFunctor, A>
	{

	};

	static_assert(Functor::Functorable<Test::NullFunctor>, "NullFunctor should be a Functor.");
	static_assert(not Monad::Monadable<Test::NullFunctor>, "NullFunctor should not be a Monad.");
	static_assert(Functor::Functorable<NullMonad>, "NullMonad should be a Functor.");
	static_assert(Monad::Monadable<NullMonad>, "NullMonad should be a Monad.");
}
