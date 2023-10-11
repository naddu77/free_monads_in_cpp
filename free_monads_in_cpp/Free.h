#pragma once

#include "Functor.h"
#include "Monad.h"

#include <variant>

namespace Free
{
	// data Free f a = Return a | Bind (f (Free f a))
	// instance (Functor f) => Monad (Free f) where
	// ...
	template <template <typename> typename F, typename A>
	struct Return;

	template <template <typename> typename F, typename A>
	struct Bind;

	template <template <typename> class F, typename A>
	struct Free
	{
		using ContainedType = A;
		using ReturnType = Return<F, A>;

		std::variant<Return<F, A>, Bind<F, A>> v;
	};

	template <template <typename> typename F, typename A>
	struct Return
	{
		A a;
	};

	template <template <typename> typename F, typename A>
	struct Bind
	{
		F<Free<F, A>> x;
	};

	template <template <typename> typename F, typename A>
	Free<F, A> MakeReturn(A const& x)
	{
		return { Return<F, A>{ x } };
	}

	template <typename FA>
	FA MakeReturn(typename FA::ContainedType const& x)
	{
		return { typename FA::ReturnType{ x } };
	}

	template <template <typename> typename F, typename A>
	Free<F, A> MakeBind(F<Free<F, A>> const& x)
	{
		return { Bind<F, A>{ x } };
	}

	template <template <typename> typename F, typename A>
	std::ostream& operator<<(std::ostream& os, Free<F, A> const& free)
	{
		struct Visitor
		{
			std::ostream& os;

			std::ostream& operator()(Return<F, A> const& r)
			{
				return os << std::format("Return{{{}}}", r.a);
			}

			std::ostream& operator()(Bind<F, A> const& b)
			{
				return os << +std::format("Bind{{{}}}", b.x);
			}
		};

		Visitor v{ os };

		return std::visit(v, free.v);
	}

	// instance Functor f => Functor (Free f) where
	//	fmap fun (Return x) = Return (fun x)
	//	fmap fun (Bind x) = Bind (fmap (fmap fun) x)
	template <template  <typename> typename Wrapper>
	struct FuntorImpl
	{
		template <typename A, typename Fn>
		struct Visitor
		{
			Fn& fun;

			template <template <typename> typename F>
			auto operator()(Return<F, A> const& r) const
			{
				return MakeReturn<F>(fun(r.a));
			}

			template <template <typename> typename F>
			auto operator()(Bind<F, A> const& b) const
			{
				using Functor::Fmap;

				return MakeBind(Fmap([&](auto const& f) { return Fmap(fun, f); }, b.x));
			}
		};

		// fmap :: (a -> b) -> Free f a -> Free f b
		template <typename A, std::invocable<A> Fn>
		static Wrapper<std::invoke_result_t<Fn, A>> Fmap(Fn&& fun, Wrapper<A> const& f)
		{
			return std::visit(Visitor<A, Fn>(fun), f.v);
		}
	};

	template <template <typename> typename Wrapper>
	struct MonadImpl
	{
		template <typename A>
		using M = Wrapper<A>;

		// pure :: a -> m a
		// pure :: a -> Free<F>
		template <typename A>
		static M<A> Pure(A const& x)
		{
			return MakeReturn<typename Wrapper<A>::WrappedFree>(x);
		}

		// bind :: Free f a -> (a -> Free f b) -> Free f b
		template <typename A, std::invocable<A> Fn>
		struct BindVisitor
		{
			using ResultType = std::invoke_result_t<Fn, A>;
			using B = typename ResultType::ContainedType;

			static_assert(std::is_same_v<ResultType, Wrapper<B>>);

			Fn&& fun;

			// bind (Bind x) f = Bind (Fmap (\m -> bind m f) x)
			template <template <typename> typename F>
			ResultType operator()(Bind<F, A> const& b)
			{
				auto& f{ this->fun };

				return MakeBind(Functor::Fmap(
					[f](Free<F, A> const& m) {
						return static_cast<Free<F, B>>(Monad::Bind(Wrapper<A>(m), f));
					}, b.x));
			}

			// bind (Return r) f = f r
			template <template <typename> typename F>
			ResultType operator()(Return<F, A> const& r)
			{
				return fun(r.a);
			}
		};

		template <typename A, std::invocable<A> Fn>
		static auto Bind(M<A> const& x, Fn&& fun)
		{
			BindVisitor<A, Fn> v{ std::forward<Fn>(fun) };

			return std::visit(v, x.v);
		}
	};

	// liftFree :: (Functor f) => f a -> Free f a
	// liftFree x = bind (fmap Return x)
	template <template <typename> typename F, typename A>
	Free<F, A> LiftFree(F<A> const& x)
	{
		return MakeBind<F>(Functor::Fmap(&MakeReturn<F, A>, x));
	}

	// foldFree :: Monad m => (forall x . f x) -> Free f a -> m a
	// foldFree _ (Return a) = return a
	// foldFree f (Bind as) = f as >>= FoldFree f
	template <template <typename> typename M, template <typename> typename F, typename A, std::invocable<F<Free<F, A>>> Fun>
	M<A> FoldFree(Fun fun, Free<F, A> const& free)
	{
		struct Visitor
		{
			Fun fun;

			auto operator()(Return<F, A> const& r)
			{
				return Monad::Pure<M>(r.a);
			}

			auto operator()(Bind<F, A> const& b)
			{
				return fun(b.x) >>= [&](auto const& x) { return FoldFree<M>(fun, x); };
			}
		};

		Visitor v{ fun };

		return std::visit(v, free.v);
	}
}

namespace Functor
{
	template <template <typename> typename Wrapper>
	struct Functor<Wrapper>
		: Free::FuntorImpl<Wrapper>
	{

	};
}

namespace Monad
{
	template <template <typename> typename Wrapper>
	struct Monad<Wrapper>
		: Free::MonadImpl<Wrapper>
	{

	};
}

namespace Free
{
	template <typename A>
	struct NullFreeWrapper
		: Free<Functor::Test::NullFunctor, A>
	{
		using WrappedFree = Free<Functor::Test::NullFunctor, A>;
	};

	static_assert(Monad::IsMonad<NullFreeWrapper>, "NullFreeWrapper must be a  Monad.");
}
