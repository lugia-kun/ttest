#ifndef TTEST_EXPECT_HPP
#define TTEST_EXPECT_HPP

// IWYU pragma: private, include "ttest.hpp"

#include <Teuchos_Array.hpp>
#include <Teuchos_CommHelpers.hpp>
#include <Teuchos_EReductionType.hpp>
#include <Teuchos_FancyOStream.hpp>
#include <Teuchos_TypeNameTraits.hpp>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <ios>
#include <ostream>

#include <stdexcept>
#include <type_traits>

#include "ttest_base.hpp"

namespace ttest
{
template <typename U> class ValuePrinter
{
public:
  static std::ostream &write(std::ostream &os, const U &value)
  {
    os << value;
    return os;
  }
};

template <> class ValuePrinter<bool>
{
public:
  static std::ostream &write(std::ostream &os, const bool &value)
  {
    os << std::boolalpha << value;
    return os;
  }
};

class FloatValuePrinter
{
public:
  template <typename U>
  static std::ostream &write(std::ostream &os, const U &value)
  {
    os << std::showpoint << value;
    return os;
  }
};

template <> class ValuePrinter<float> : public FloatValuePrinter
{
};

template <> class ValuePrinter<double> : public FloatValuePrinter
{
};

template <> class ValuePrinter<long double> : public FloatValuePrinter
{
};

template <typename WriteCharEscaped> class StringValuePrinter
{
public:
  template <typename InIt>
  static std::ostream &write(std::ostream &os, const InIt &b, const InIt &e)
  {
    os << WriteCharEscaped::quote_in;
    for (InIt it = b; it != e; it++)
      WriteCharEscaped::write(os, *b);
    os << WriteCharEscaped::quote_out;
    return os;
  }

  static std::ostream &write(std::ostream &os, const std::string &source)
  {
    return write(os, source.cbegin(), source.cend());
  }

  /* Assumes string literal (automatically added '\0' at end) */
  template <std::size_t N>
  static std::ostream &write(std::ostream &os, const char (&str)[N])
  {
    return write(os, &str[0], &str[N - 1]);
  }
};

class WriteCharBaseStringEscaped
{
public:
  static std::ostream &write(std::ostream &os, unsigned char uch)
  {
    switch (uch) {
    case '\0':
      return os << "\\0";
    case '\a':
      return os << "\\a";
    case '\b':
      return os << "\\b";
    case '\f':
      return os << "\\f";
    case '\n':
      return os << "\\n";
    case '\r':
      return os << "\\r";
    case '\t':
      return os << "\\t";
    case '\v':
      return os << "\\v";
    }
    if (!std::isprint(uch) || std::iscntrl(uch))
      return os << "\\" << std::setw(3) << std::setfill('0') << std::oct
                << (unsigned int)uch;
    return os << uch;
  }
};

class WriteCharStringEscaped
{
public:
  static std::ostream &write(std::ostream &os, unsigned char uch)
  {
    switch (uch) {
    case '"':
      return os << "\\\"";
    case '\\':
      return os << "\\\\";
    }
    return WriteCharBaseStringEscaped::write(os, uch);
  }

  static constexpr const char quote_in[] = "\"";
  static constexpr const char quote_out[] = "\"";
};

class WriteCharCharEscaped
{
public:
  static std::ostream &write(std::ostream &os, unsigned char uch)
  {
    switch (uch) {
    case '\'':
      return os << "\\'";
    case '\\':
      return os << "\\\\";
    }
    return WriteCharBaseStringEscaped::write(os, uch);
  }

  static constexpr const char quote_in[] = "'";
  static constexpr const char quote_out[] = "'";
};

template <>
class ValuePrinter<std::string>
    : public StringValuePrinter<WriteCharStringEscaped>
{
};

template <std::size_t N>
class ValuePrinter<const char (&)[N]>
    : public StringValuePrinter<WriteCharStringEscaped>
{
};

class CharValuePrinter
{
public:
  template <typename U>
  static std::ostream &write(std::ostream &os, const U &value)
  {
    static_assert(sizeof(U) == sizeof(char));
    char str[2];
    str[0] = value;
    str[1] = '\0';
    return StringValuePrinter<WriteCharCharEscaped>::write(os, str);
  }
};

template <> class ValuePrinter<char> : public CharValuePrinter
{
};

template <> class ValuePrinter<signed char> : public CharValuePrinter
{
};

template <> class ValuePrinter<unsigned char> : public CharValuePrinter
{
};

template <typename ElementPrinter> class ArrayLikePrinter
{
public:
  template <typename U>
  static std::ostream &write(std::ostream &os, const U &value)
  {
    bool first = true;
    os << "{";
    for (auto &item : value) {
      if (!first)
        os << ", ";
      ElementPrinter::write(os, item);
    }
    os << "}";
    return os;
  }
};

template <typename U>
class ValuePrinter<Teuchos::Array<U>> : public ArrayLikePrinter<ValuePrinter<U>>
{
};

template <typename U>
class ValuePrinter<Teuchos::Array<const U>>
    : public ArrayLikePrinter<ValuePrinter<U>>
{
};

template <typename U>
class ValuePrinter<Teuchos::ArrayView<U>>
    : public ArrayLikePrinter<ValuePrinter<U>>
{
};

template <typename U>
class ValuePrinter<Teuchos::ArrayView<const U>>
    : public ArrayLikePrinter<ValuePrinter<U>>
{
};

template <typename U>
class ValuePrinter<Teuchos::ArrayRCP<U>>
    : public ArrayLikePrinter<ValuePrinter<U>>
{
};

template <typename U>
class ValuePrinter<Teuchos::ArrayRCP<const U>>
    : public ArrayLikePrinter<ValuePrinter<U>>
{
};

class StdExceptionPrinter
{
public:
  static std::ostream &write(std::ostream &os, const std::exception &value)
  {
    const char *what = value.what();
    if (std::string(what).find('\n') == std::string::npos) {
      return os << "what(): \"" << what << "\"";
    } else {
      os << "what():\n";
      {
        Teuchos::OSTab t1(os, 2);
        os << what;
      }
      return os;
    }
  }
};

template <> class ValuePrinter<std::exception> : public StdExceptionPrinter
{
};

template <> class ValuePrinter<std::runtime_error> : public StdExceptionPrinter
{
};

class VerbBase
{
public:
  /* Subclass needs the implemetation */
  template <typename U> bool test(const U &value) const;

  virtual const char *textRepresentation() const = 0;
  virtual std::ostream &writeExpected(std::ostream &os) const = 0;
  virtual const char *expectedTitle() const { return "Expected"; }
  virtual const char *actualTitle() const { return "Got"; }

  virtual Teuchos::FancyOStream &
  writeAdditionalAttributes(Teuchos::FancyOStream &os) const
  {
    return os;
  }
};

template <typename T, template <typename> typename Tester>
class BeExpressionType : public VerbBase
{
public:
  const T &expected;
  const char *expr;

  BeExpressionType(const T &value, const char *expression)
      : expected(value), expr(expression)
  {
  }

  template <typename U>
  std::enable_if_t<(std::is_integral_v<U> && std::is_unsigned_v<U>) &&
                       (std::is_integral_v<T> && std::is_unsigned_v<T>),
                   bool>
  test(const U &value) const
  {
    return Tester<uintmax_t>{}(value, expected);
  }

  template <typename U>
  std::enable_if_t<(std::is_integral_v<U> && std::is_signed_v<U>) &&
                       (std::is_integral_v<T> && std::is_signed_v<T>),
                   bool>
  test(const U &value) const
  {
    return Tester<intmax_t>{}(value, expected);
  }

  template <typename U>
  std::enable_if_t<(std::is_integral_v<U> || std::is_integral_v<T>) &&
                       std::is_signed_v<T> != std::is_signed_v<U>,
                   bool>
  test(const U &value) const
  {
    static_assert(std::is_same_v<U, T>,
                  "Trying to compare different signess integral types, add 'u' "
                  "suffix if expected value is literal constant");
    return Tester<T>{}(value, expected);
  }

  template <typename U>
  std::enable_if_t<std::is_floating_point_v<U> && std::is_floating_point_v<T> &&
                       (sizeof(U) > sizeof(T)),
                   bool>
  test(const U &value) const
  {
    /* Assume upcast is lossless */
    return Tester<U>{}(value, expected);
  }

  template <typename U>
  std::enable_if_t<std::is_floating_point_v<U> && std::is_floating_point_v<T> &&
                       (sizeof(U) <= sizeof(T)),
                   bool>
  test(const U &value) const
  {
    /* Assume upcast is lossless */
    return Tester<T>{}(value, expected);
  }

  template <typename U>
  std::enable_if_t<(!std::is_integral_v<U> || !std::is_integral_v<T>) &&
                       (!std::is_floating_point_v<U> ||
                        !std::is_floating_point_v<T>),
                   bool>
  test(const U &value) const
  {
    static_assert(std::is_same_v<U, T>, "Comparing types are different");
    return Tester<T>{}(value, expected);
  }

  const char *textRepresentation() const override { return expr; }

  std::ostream &writeExpected(std::ostream &os) const override
  {
    return ValuePrinter<T>::write(os, expected);
  }
};

#define TTEST_DEFINE_BE_BIN_OP(class_name, op)                                 \
  template <typename T> class class_name##Tester                               \
  {                                                                            \
  public:                                                                      \
    bool operator()(const T &actual, const T &expected) const                  \
    {                                                                          \
      return actual op expected;                                               \
    }                                                                          \
  };                                                                           \
  template <typename T>                                                        \
  using class_name = BeExpressionType<T, class_name##Tester>

TTEST_DEFINE_BE_BIN_OP(BeEq, ==);
TTEST_DEFINE_BE_BIN_OP(BeNeq, !=);
TTEST_DEFINE_BE_BIN_OP(BeLess, <);
TTEST_DEFINE_BE_BIN_OP(BeGreater, >);
TTEST_DEFINE_BE_BIN_OP(BeLessEq, <=);
TTEST_DEFINE_BE_BIN_OP(BeGreaterEq, >=);

class Be
{
public:
  const char *expr;

  Be(const char *e) : expr(e) {}

  template <typename T> BeEq<T> operator==(const T &value) const
  {
    return BeEq<T>(value, expr);
  }

  template <typename T> BeNeq<T> operator!=(const T &value) const
  {
    return BeNeq<T>(value, expr);
  }

  template <typename T> BeLess<T> operator<(const T &value) const
  {
    return BeLess<T>(value, expr);
  }

  template <typename T> BeGreater<T> operator>(const T &value) const
  {
    return BeGreater<T>(value, expr);
  }

  template <typename T> BeLessEq<T> operator<=(const T &value) const
  {
    return BeLessEq<T>(value, expr);
  }

  template <typename T> BeGreaterEq<T> operator>=(const T &value) const
  {
    return BeGreaterEq<T>(value, expr);
  }
};

template <typename T> class BeNear : public VerbBase
{
public:
  const char *expr;
  T expected;
  T delta;

  BeNear(const T &expected, const T &delta, const char *expr)
      : expected(expected), delta(delta), expr(expr)
  {
  }

  template <typename U> bool test(const U &value) const
  {
    static_assert(std::is_same_v<U, T>, "Comparing different types");
    return std::abs(expected - value) <= delta;
  }

  const char *textRepresentation() const override { return expr; }

  std::ostream &writeExpected(std::ostream &os) const override
  {
    return ValuePrinter<T>::write(os, expected);
  }

  virtual Teuchos::FancyOStream &
  writeAdditionalAttributes(Teuchos::FancyOStream &os) const override
  {
    os << "Delta:\n";
    Teuchos::OSTab t1(os, 2);
    ValuePrinter<T>::write(os, delta);
    os << "\n";
    return os;
  }
};

template <typename Unit> class ExpectAct
{
protected:
  Unit *unit;

  ExpectAct(Unit *unit) : unit(unit) {}

  /**
   * Synchronize result among processes, if needed. Base implementation
   * does not do.
   *
   * @retval true  (any of processes) failed the expectation.
   * @ratval false (all of processes) passed the expectation.
   */
  virtual bool syncResult(bool this_result) const { return this_result; }

  /**
   * throw or just mark and continue after failed expectation. Base
   * implementation just mark and continue running.
   */
  virtual void fail() const { unit->markFailed(); }
};

template <typename Base> class SyncExpectAct : public Base
{
protected:
  template <typename Unit> SyncExpectAct(Unit *unit) : Base(unit) {}

  /**
   * Synchronize result of unit communicator
   */
  virtual bool syncResult(bool this_result) const override
  {
    auto comm = this->unit->comm();
    if (comm) {
      int ival = this_result;
      int oval = this_result;
      Teuchos::reduceAll(*comm, Teuchos::REDUCE_MAX, 1, &ival, &oval);
      this_result = oval;
    }
    return this_result;
  }
};

template <typename Base> class HardExpectAct : public Base
{
protected:
  template <typename Unit> HardExpectAct(Unit *unit) : Base(unit) {}

  /**
   * throw and abort running test unit or case.
   */
  virtual void fail() const override { this->unit->throwFail("Expect Failed"); }
};

template <typename Unit> using BaseAct = ExpectAct<Unit>;
template <typename Unit> using SyncAct = SyncExpectAct<BaseAct<Unit>>;
template <typename Unit> using HardAct = HardExpectAct<BaseAct<Unit>>;
template <typename Unit> using HardSyncAct = HardExpectAct<SyncAct<Unit>>;

template <typename ActBase, typename T> class Expect : public ActBase
{
protected:
  const char *expr;
  const T &value;
  const char *file;
  long line;

  template <typename Verb>
  void writeFail(const char *adj, const Verb &verb) const
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    out << "\n" << expr << adj << " " << verb.textRepresentation() << "\n\n";

    {
      Teuchos::OSTab t1(out, 2);
      out << verb.expectedTitle() << ":\n";
      {
        Teuchos::OSTab t2(out, 2);
        verb.writeExpected(out);
        out << "\n";
      }
      out << verb.actualTitle() << ":\n";
      {
        Teuchos::OSTab t2(out, 2);
        ValuePrinter<T>::write(out, value);
        out << "\n";
      }
      verb.writeAdditionalAttributes(out);
    }

    out << "\n(defined at " << file << ":" << line << ")\n";
  }

public:
  template <typename Unit>
  Expect(Unit *unit, const T &value, const char *expr, const char *file,
         long line)
      : ActBase(unit), value(value), expr(expr), file(file), line(line)
  {
  }

  template <typename Verb> void should(const Verb &verb) const
  {
    bool this_result = !verb.test(value);
    if (this->syncResult(this_result)) {
      if (this_result)
        writeFail(" should", verb);
      this->fail();
    }
  }

  template <typename Verb> void shouldNot(const Verb &verb) const
  {
    bool this_result = !!verb.test(value);
    if (this->syncResult(this_result)) {
      if (this_result)
        writeFail(" should not", verb);
      this->fail();
    }
  }
};

template <typename T, typename Unit>
auto expect(const T &value, Unit *unit, const char *expr, const char *file,
            long line)
{
  return Expect<BaseAct<Unit>, T>(unit, value, expr, file, line);
}

template <typename T, typename Unit>
auto hardExpect(const T &value, Unit *unit, const char *expr, const char *file,
                long line)
{
  return Expect<HardAct<Unit>, T>(unit, value, expr, file, line);
}

template <typename T, typename Unit>
auto syncedExpect(const T &value, Unit *unit, const char *expr,
                  const char *file, long line)
{
  return Expect<SyncAct<Unit>, T>(unit, value, expr, file, line);
}

template <typename T, typename Unit>
auto syncedHardExpect(const T &value, Unit *unit, const char *expr,
                      const char *file, long line)
{
  return Expect<HardSyncAct<Unit>, T>(unit, value, expr, file, line);
}

/**
 * This class is used for mark that test function did not throw.
 */
class NoThrow;

/**
 * This class is used for mark that test function throws an other class.
 */
class OtherThrow;

template <typename Exception, typename CatchType> class ExpectThrowFailPrinter
{
public:
  static std::ostream &write(std::ostream &os, const char *expr,
                             const CatchType &e, const char *file, long line,
                             const char *expected_message = nullptr)
  {
    os << "\nFollowing expression expected to throw "
       << Teuchos::TypeNameTraits<Exception>::name() << ", but threw "
       << Teuchos::typeName(e) << "\n";
    {
      Teuchos::OSTab t1(os, 2);
      os << "Expression:\n";
      {
        Teuchos::OSTab t2(os, 2);
        os << expr << ";\n";
      }
      if (expected_message) {
        os << "Expected:\n";
        {
          Teuchos::OSTab t2(os, 2);
          os << "what(): " << expected_message << "\n";
        }
      }
      os << "Thrown:\n";
      {
        Teuchos::OSTab t2(os, 2);
        ValuePrinter<CatchType>::write(os, e);
        os << "\n";
      }
    }
    os << "\n(defined at " << file << ":" << line << ")\n";
    return os;
  }
};

template <typename CatchType> class ExpectThrowFailPrinter<NoThrow, CatchType>
{
public:
  static std::ostream &write(std::ostream &os, const char *expr,
                             const CatchType &e, const char *file, long line)
  {
    os << "\nFollowing expression expected not to throw, but it threw "
       << Teuchos::typeName(e) << "\n";
    {
      Teuchos::OSTab t1(os, 2);
      os << "Expression:\n";
      {
        Teuchos::OSTab t2(os, 2);
        os << expr << ";\n";
      }
      os << "Thrown:\n";
      {
        Teuchos::OSTab t2(os, 2);
        ValuePrinter<CatchType>::write(os, e);
        os << "\n";
      }
    }
    os << "\n(defined at " << file << ":" << line << ")\n";
    return os;
  }
};

template <typename Exception>
class ExpectThrowFailPrinter<Exception, OtherThrow>
{
public:
  static std::ostream &write(std::ostream &os, const char *expr,
                             const char *file, long line)
  {
    os << "\nFollowing expression expected to throw "
       << Teuchos::TypeNameTraits<Exception>::name()
       << ", but threw an exception of unknown type\n";
    {
      Teuchos::OSTab t1(os, 2);
      os << "Expression:\n";
      {
        Teuchos::OSTab t2(os, 2);
        os << expr << ";\n";
      }
    }
    os << "\n(defined at " << file << ":" << line << ")\n";
    return os;
  }
};

template <typename Exception> class ExpectThrowFailPrinter<Exception, NoThrow>
{
public:
  static std::ostream &write(std::ostream &os, const char *expr,
                             const char *file, long line)
  {
    os << "\nFollowing expression expected to throw "
       << Teuchos::TypeNameTraits<Exception>::name()
       << ", but did not throw an exception\n\n";
    {
      Teuchos::OSTab t1(os, 2);
      os << "Expression:\n";
      {
        Teuchos::OSTab t2(os, 2);
        os << expr << ";\n";
      }
    }
    os << "\n(defined at " << file << ":" << line << ")\n";
    return os;
  }
};

template <> class ExpectThrowFailPrinter<NoThrow, NoThrow>
{
  // not allowed
};

template <> class ExpectThrowFailPrinter<NoThrow, OtherThrow>
{
public:
  static std::ostream &write(std::ostream &os, const char *expr,
                             const char *file, long line)
  {
    os << "\nFollowing expression expected not to throw, but it threw an "
          "exception of unknown "
          "type\n";
    {
      Teuchos::OSTab t1(os, 2);
      os << "Expression:\n";
      {
        Teuchos::OSTab t2(os, 2);
        os << expr << ";\n";
      }
    }
    os << "\n(defined at " << file << ":" << line << ")\n";
    return os;
  }
};

/**
 * Try to catch Exception. If other CatchType will be reported as failure and
 * rteurs. If CatchOthers is true, other exceptions will be reported as failure
 * and returns. If CatchOthers is false, other exceptions will be reoprted as
 * failure and rethrow it.
 *
 * ```c++
 * try {
 *   proc()
 * } catch(Exception &e) {
 *   // pass expectation
 *   return;
 * } catch(CatchType &) {
 *   // fail expectation
 *   return;
 * } catch(...) {
 *   // fail expectation
 *   if (!CatchOthers)
 *     throw;
 *   return;
 * }
 * // fail expectation
 * ```
 */
template <typename Exception, typename CatchType, bool CatchOthers,
          typename Tester, typename ActBase>
class ExpectThrow : public ActBase
{
protected:
  virtual void writeFailed(const CatchType &e) const
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    ExpectThrowFailPrinter<Exception, CatchType>::write(out, expr, e, file,
                                                        line);
  }

  virtual void writeFailedOthers() const
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    ExpectThrowFailPrinter<Exception, OtherThrow>::write(out, expr, file, line);
  }

  virtual void writeFailedNoThrow() const
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    ExpectThrowFailPrinter<Exception, NoThrow>::write(out, expr, file, line);
  }

  const char *expr;
  const char *file;
  long line;
  Tester tester;

public:
  template <typename Unit>
  ExpectThrow(const Tester &tester, const char *expr, const char *file,
              long line, Unit *unit)
      : ActBase(unit), tester(tester), expr(expr), file(file), line(line)
  {
  }

  template <typename Func>
  std::enable_if_t<std::is_same_v<Func, Func> &&
                   !std::is_same_v<Exception, CatchType>>
  start(const Func &func) const
  {
    try {
      func();
    } catch (const Exception &e) {
      bool this_result = !tester(e);
      if (this->syncResult(this_result)) {
        if (this_result)
          tester.writeFailed(e, expr, file, line);
        this->fail();
      }
      return;
    } catch (const CatchType &c) {
      this->syncResult(true);
      writeFailed(c);
      this->fail();
      return;
    } catch (...) {
      this->syncResult(true);
      writeFailedOthers();
      if (!CatchOthers)
        throw;
      this->fail();
      return;
    }
    this->syncResult(true);
    writeFailedNoThrow();
    this->fail();
  }

  template <typename Func>
  std::enable_if_t<std::is_same_v<Func, Func> &&
                   std::is_same_v<Exception, CatchType>>
  start(const Func &func) const
  {
    try {
      func();
    } catch (const Exception &e) {
      bool this_result = !tester(e);
      if (this->syncResult(this_result)) {
        if (this_result)
          tester.writeFailed(e, expr, file, line);
        this->fail();
      }
      return;
    } catch (...) {
      this->syncResult(true);
      writeFailedOthers();
      if (!CatchOthers)
        throw;
      this->fail();
      return;
    }
    this->syncResult(true);
    writeFailedNoThrow();
    this->fail();
  }
};

/**
 *
 */
template <typename CatchType, bool CatchOthers, typename ActBase>
class ExpectNoThrow : public ActBase
{
protected:
  virtual void writeFailed(const CatchType &e) const
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    ExpectThrowFailPrinter<NoThrow, CatchType>::write(out, expr, e, file, line);
  }

  virtual void writeFailedOthers() const
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    ExpectThrowFailPrinter<NoThrow, OtherThrow>::write(out, expr, file, line);
  }

  const char *expr;
  const char *file;
  long line;

public:
  template <typename Unit>
  ExpectNoThrow(const char *expr, const char *file, long line, Unit *unit)
      : ActBase(unit), expr(expr), file(file), line(line)
  {
  }

  template <typename Func> void start(const Func &func) const
  {
    try {
      func();
    } catch (const CatchType &c) {
      this->syncResult(true);
      writeFailed(c);
      this->fail();
      return;
    } catch (...) {
      this->syncResult(true);
      writeFailedOthers();
      if (!CatchOthers)
        throw;
      this->fail();
      return;
    }
    bool this_result = false;
    if (this->syncResult(this_result))
      this->fail();
  }
};

template <typename CatchType, bool CatchOthers, typename Tester,
          typename ActBase>
class ExpectThrow<NoThrow, CatchType, CatchOthers, Tester, ActBase>
    : public ExpectNoThrow<CatchType, CatchOthers, ActBase>
{
public:
public:
  template <typename Unit>
  ExpectThrow(const char *expr, const char *file, long line, Unit *unit)
      : ExpectNoThrow<CatchType, CatchOthers, ActBase>(expr, file, line, unit)
  {
  }
};

class ExpectThrowTesterNOP
{
public:
  template <typename E> bool operator()(const E &e) const { return true; }

  template <typename E>
  void writeFailed(const E &e, const char *expr, const char *file,
                   long line) const
  {
  }
};

class ExpectThrowTesterWhatEq
{
public:
  const char *expected_message;

  template <typename E>
  std::enable_if_t<std::is_base_of_v<std::exception, E>, bool>
  operator()(const E &e) const
  {
    if (!expected_message)
      return true;
    return std::string(e.what()) == std::string(expected_message);
  }

  template <typename E>
  std::enable_if_t<!std::is_base_of_v<std::exception, E>, bool>
  operator()(const E &e) const
  {
    if (!expected_message)
      return true;
    return false;
  }

  template <typename E>
  std::enable_if_t<std::is_base_of_v<std::exception, E>>
  writeFailed(const E &e, const char *expr, const char *file, long line)
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    out << "Following expression threw " << Teuchos::typeName(e)
        << ", but with different message\n";
    {
      Teuchos::OSTab t1(out, 2);
      out << "Expression:\n";
      {
        Teuchos::OSTab t2(out, 2);
        out << expr << ";\n";
      }
      out << "Expected:\n";
      {
        Teuchos::OSTab t2(out, 2);
        ValuePrinter<std::string>::write(out, std::string(expected_message));
        out << "\n";
      }
      out << "Actual:\n";
      {
        Teuchos::OSTab t2(out, 2);
        ValuePrinter<std::string>::write(out, std::string(e.what()));
        out << "\n";
      }
      out << "\n(defined at " << file << ":" << line << ")\n";
    }
  }

  template <typename E>
  std::enable_if_t<!std::is_base_of_v<std::exception, E>>
  writeFailed(const E &e, const char *expr, const char *file, long line)
  {
    auto outp = getOutputStream();
    auto &out = *outp;
    out << "Following expression threw " << Teuchos::typeName(e)
        << ", but it does not derive std::exception\n";
    {
      Teuchos::OSTab t1(out, 2);
      out << "Expression:\n";
      {
        Teuchos::OSTab t2(out, 2);
        out << expr << ";\n";
      }
      out << "Expected:\n";
      {
        Teuchos::OSTab t2(out, 2);
        ValuePrinter<std::string>::write(out, std::string(expected_message));
        out << "\n";
      }
      out << "Actual:\n";
      {
        Teuchos::OSTab t2(out, 2);
        out << "(not available)\n";
      }
      out << "\n(defined at " << file << ":" << line << ")\n";
    }
  }
};

template <typename Exception, typename CatchClass, bool CatchOthers>
class ExpectThrowsGen
{
public:
  template <typename ActBase, typename Tester>
  using type = ExpectThrow<Exception, CatchClass, CatchOthers, Tester, ActBase>;
};

template <typename CatchClass, bool CatchOthers> class ExpectNoThrowsGen
{
public:
  template <typename ActBase>
  using type = ExpectNoThrow<CatchClass, CatchOthers, ActBase>;
};

template <template <typename, typename> typename ExpectThrowsGenType,
          template <typename> typename ActBase, typename Tester, typename Unit>
auto expectThrowBase(const Tester &tester, const char *expr, Unit *unit,
                     const char *file, long line)
{
  return ExpectThrowsGenType<ActBase<Unit>, Tester>(tester, expr, file, line,
                                                    unit);
}

template <template <typename> typename ExpectNoThrowsGenType,
          template <typename> typename ActBase, typename Unit>
auto expectNoThrowBase(const char *expr, Unit *unit, const char *file,
                       long line)
{
  return ExpectNoThrowsGenType<ActBase<Unit>>(expr, file, line, unit);
}

template <template <typename, typename> typename ExpectThrowsGenType,
          typename Tester, typename Unit>
auto expectThrow(const Tester &tester, const char *expr, Unit *unit,
                 const char *file, long line)
{
  return expectThrowBase<ExpectThrowsGenType, BaseAct>(tester, expr, unit, file,
                                                       line);
}

template <template <typename, typename> typename ExpectThrowsGenType,
          typename Tester, typename Unit>
auto syncedExpectThrow(const Tester &tester, const char *expr, Unit *unit,
                       const char *file, long line)
{
  return expectThrowBase<ExpectThrowsGenType, SyncAct>(tester, expr, unit, file,
                                                       line);
}

template <template <typename, typename> typename ExpectThrowsGenType,
          typename Tester, typename Unit>
auto hardExpectThrow(const Tester &tester, const char *expr, Unit *unit,
                     const char *file, long line)
{
  return expectThrowBase<ExpectThrowsGenType, HardAct>(tester, expr, unit, file,
                                                       line);
}

template <template <typename, typename> typename ExpectThrowsGenType,
          typename Tester, typename Unit>
auto syncedHardExpectThrow(const Tester &tester, const char *expr, Unit *unit,
                           const char *file, long line)
{
  return expectThrowBase<ExpectThrowsGenType, HardSyncAct>(tester, expr, unit,
                                                           file, line);
}

template <template <typename> typename ExpectNoThrowsGenType, typename Unit>
auto expectNoThrow(const char *expr, Unit *unit, const char *file, long line)
{
  return expectNoThrowBase<ExpectNoThrowsGenType, BaseAct>(expr, unit, file,
                                                           line);
}

template <template <typename> typename ExpectNoThrowsGenType, typename Unit>
auto syncedExpectNoThrow(const char *expr, Unit *unit, const char *file,
                         long line)
{
  return expectNoThrowBase<ExpectNoThrowsGenType, SyncAct>(expr, unit, file,
                                                           line);
}

template <template <typename> typename ExpectNoThrowsGenType, typename Unit>
auto hardExpectNoThrow(const char *expr, Unit *unit, const char *file,
                       long line)
{
  return expectNoThrowBase<ExpectNoThrowsGenType, HardAct>(expr, unit, file,
                                                           line);
}

template <template <typename> typename ExpectNoThrowsGenType, typename Unit>
auto syncedHardExpectNoThrow(const char *expr, Unit *unit, const char *file,
                             long line)
{
  return expectNoThrowBase<ExpectNoThrowsGenType, HardSyncAct>(expr, unit, file,
                                                               line);
}

template <typename Exception>
using DefaultExpectThrow = ExpectThrowsGen<Exception, std::exception, false>;

using DefaultExpectNoThrow = ExpectNoThrowsGen<std::exception, false>;

} // namespace ttest

#define expect(value) ::ttest::expect(value, this, #value, __FILE__, __LINE__)
#define syncedExpect(value)                                                    \
  ::ttest::syncedExpect(value, this, #value, __FILE__, __LINE__)
#define hardExpect(value)                                                      \
  ::ttest::hardExpect(value, this, #value, __FILE__, __LINE__)
#define syncedHardExpect(value)                                                \
  ::ttest::syncedHardExpect(value, this, #value, __FILE__, __LINE__)

#define BE(expr) ::ttest::Be("be " #expr) expr
#define EQ(value) ::ttest::Be("be equal to " #value) == value
#define NE(value) ::ttest::Be("be unequal to " #value) != value
#define LT(value) ::ttest::Be("be less than " #value) < value
#define GT(value) ::ttest::Be("be greater than " #value) > value
#define LE(value) ::ttest::Be("be less than or equal to " #value) <= value
#define GE(value) ::ttest::Be("be greater than or equal to " #value) >= value

#define BE_NEAR(value, delta)                                                  \
  ::ttest::BeNear(value, delta, "be near " #value " (+/- " #delta ")")

#define BE_TRUE() ::ttest::Be("be true") == true
#define BE_FALSE() ::ttest::Be("be false") == false

#define expectThrowBase(tester, expr, ...)                                     \
  ::ttest::expectThrow<__VA_ARGS__>(tester, #expr, this, __FILE__, __LINE__)   \
      .start([&]() { expr; })

#define syncedExpectThrowBase(tester, expr, ...)                               \
  ::ttest::syncedExpectThrow<__VA_ARGS__>(tester, #expr, this, __FILE__,       \
                                          __LINE__)                            \
      .start([&]() { expr; })

#define hardExpectThrowBase(tester, expr, ...)                                 \
  ::ttest::hardExpectThrow<__VA_ARGS__>(tester, #expr, this, __FILE__,         \
                                        __LINE__)                              \
      .start([&]() { expr; })

#define syncedHardExpectThrowBase(tester, expr, ...)                           \
  ::ttest::syncedHardExpectThrow<__VA_ARGS__>(tester, #expr, this, __FILE__,   \
                                              __LINE__)                        \
      .start([&]() { expr; })

#define expectNoThrowBase(expr, ...)                                           \
  ::ttest::expectNoThrow<__VA_ARGS__>(#expr, this, __FILE__, __LINE__)         \
      .start([&]() { expr; })

#define syncedExpectNoThrowBase(expr, ...)                                     \
  ::ttest::syncedExpectNoThrow<__VA_ARGS__>(#expr, this, __FILE__, __LINE__)   \
      .start([&]() { expr; })

#define hardExpectNoThrowBase(expr, ...)                                       \
  ::ttest::hardExpectNoThrow<__VA_ARGS__>(#expr, this, __FILE__, __LINE__)     \
      .start([&]() { expr; })

#define syncedHardExpectNoThrowBase(expr, ...)                                 \
  ::ttest::syncedHardExpectNoThrow<__VA_ARGS__>(#expr, this, __FILE__,         \
                                                __LINE__)                      \
      .start([&]() { expr; })

#define expectThrow(expr, klass)                                               \
  expectThrowBase(::ttest::ExpectThrowTesterNOP{}, expr,                       \
                  ::ttest::DefaultExpectThrow<klass>::type)

#define syncedExpectThrow(expr, klass)                                         \
  syncedExpectThrowBase(::ttest::ExpectThrowTesterNOP{}, expr,                 \
                        ::ttest::DefaultExpectThrow<klass>::type)

#define hardExpectThrow(expr, klass)                                           \
  hardExpectThrowBase(::ttest::ExpectThrowTesterNOP{}, expr,                   \
                      ::ttest::DefaultExpectThrow<klass>::type)

#define syncedHardExpectThrow(expr, klass)                                     \
  syncedHardExpectThrowBase(::ttest::ExpectThrowTesterNOP{}, expr,             \
                            ::ttest::DefaultExpectThrow<klass>::type)

#define expectNoThrow(expr)                                                    \
  expectNoThrowBase(expr, ::ttest::DefaultExpectNoThrow::type)

#define syncedExpectNoThrow(expr)                                              \
  syncedExpectNoThrowBase(expr, ::ttest::DefaultExpectNoThrow::type)

#define hardExpectNoThrow(expr)                                                \
  hardExpectNoThrowBase(expr, ::ttest::DefaultExpectNoThrow::type)

#define syncedHardExpectNoThrow(expr)                                          \
  syncedHardExpectNoThrowBase(expr, ::ttest::DefaultExpectNoThrow::type)

#endif
