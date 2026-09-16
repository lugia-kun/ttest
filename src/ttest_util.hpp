#ifndef TTEST_UTIL_HPP
#define TTEST_UTIL_HPP

// IWYU pragma: private, include "ttest.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace ttest
{
class TestDescriptionConcatenate
{
  template <typename C, std::size_t... I>
  static constexpr auto to_seq_impl(std::integer_sequence<std::size_t, I...>)
  {
    return std::integer_sequence<char, C::name[I]...>{};
  }

  template <typename C, std::size_t N>
  static constexpr auto to_seq_impl(const char (&)[N])
  {
    return to_seq_impl<C>(std::make_index_sequence<N - 1>{});
  }

  template <typename C> static constexpr auto to_seq(C)
  {
    static_assert(std::is_class_v<C>);
    return to_seq_impl<C>(C::name);
  }

  template <char... A>
  static constexpr auto to_seq(std::integer_sequence<char, A...>)
  {
    return std::integer_sequence<char, A...>{};
  }

  template <char... A, char... B, typename... C>
  constexpr auto concat(std::integer_sequence<char, A...>,
                        std::integer_sequence<char, B...>, C... seqs) const
  {
    return concat(std::integer_sequence<char, A..., B...>{}, seqs...);
  }

  template <char... A>
  constexpr auto concat(std::integer_sequence<char, A...>) const
  {
    return std::integer_sequence<char, A...>{};
  }

public:
  template <typename... T> constexpr auto operator()(const T &...args) const
  {
    return concat(to_seq(args)...);
  }
};

template <typename U, typename C> class TestDescriptionMake
{
  template <typename X = typename C::parent, typename D = void>
  class Next : public TestDescriptionMake<U, X>
  {
  public:
    static constexpr auto name()
    {
      return TestDescriptionConcatenate{}(
          TestDescriptionMake<U, X>::name(),
          std::integer_sequence<char, ':', ':'>{});
    }
  };

  template <typename D>
  class Next<void, D>
  {
  public:
    static constexpr auto name()
    {
      return std::integer_sequence<char>{};
    }
  };

public:
  template <typename F> static constexpr auto description()
  {
    return TestDescriptionConcatenate{}(
        name(), std::integer_sequence<char, ' '>{}, F{});
  }

  static constexpr auto name()
  {
    return TestDescriptionConcatenate{}(Next<>::name(), C{});
  }
};

template <typename U> class TestDescriptionMake<U, void>
{
public:
  template <typename F> static constexpr auto description()
  {
    return TestDescriptionConcatenate{}(name(), F{});
  }

  static constexpr auto name() { return std::integer_sequence<char>{}; }
};

template <typename S> class TestDescriptionString;

template <char... D>
class TestDescriptionString<std::integer_sequence<char, D...>>
{
public:
  static constexpr const char name[] = {D..., '\0'};
};

template <typename B, typename P, typename L>
class ItDescription
    : public TestDescriptionString<
          decltype(TestDescriptionMake<B, P>::template description<L>())>
{
};
} // namespace ttest

#endif
