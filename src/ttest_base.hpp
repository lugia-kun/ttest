#ifndef TTEST_BASE_HPP
#define TTEST_BASE_HPP

// IWYU pragma: private, include "ttest.hpp"

#include "ttest_util.hpp" // IWYU pragma: keep // for using in macro

#include <Teuchos_Comm.hpp>
#include <Teuchos_CommHelpers.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_EReductionType.hpp>
#include <Teuchos_FancyOStream.hpp>
#include <Teuchos_RCP.hpp>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace ttest
{
using Teuchos::RCP;

template <typename Ordinal> RCP<const Teuchos::Comm<Ordinal>> globalComm()
{
  return Teuchos::DefaultComm<Ordinal>::getComm();
}

/*
 * Executable of program. Taken from argv[0] of `runAllTests()`.
 */
const char *runnerExec();

template <typename Unit, typename Base>
class UnitTestStatusException : public Base
{
public:
  UnitTestStatusException(const std::string &what) : Base(what) {}

  const char *unitName() const override;
};

template <typename Unit, typename Case, typename Base>
class UnitCaseTestStatusException : public UnitTestStatusException<Unit, Base>
{
public:
  UnitCaseTestStatusException(const std::string &what)
      : UnitTestStatusException<Unit, Base>(what)
  {
  }

  const char *caseName() const override;
};

class TestStatusException : public std::runtime_error
{
public:
  template <typename X>
  TestStatusException(X &&arg) : std::runtime_error(std::forward<X>(arg))
  {
  }

  virtual const char *unitName() const { return nullptr; }
  virtual const char *caseName() const { return nullptr; }
};

class TestFailed : public TestStatusException
{
public:
  TestFailed(const std::string &what) : TestStatusException(what) {}
};

class TestPassed : public TestStatusException
{
public:
  TestPassed(const std::string &what) : TestStatusException(what) {}
};

class TestSkipped : public TestStatusException
{
public:
  TestSkipped(const std::string &what) : TestStatusException(what) {}
};

template <typename Unit>
using TestUnitPassed = UnitTestStatusException<Unit, TestPassed>;

template <typename Unit>
using TestUnitFailed = UnitTestStatusException<Unit, TestFailed>;

template <typename Unit>
using TestUnitSkipped = UnitTestStatusException<Unit, TestSkipped>;

template <typename Unit, typename Case>
using TestCasePassed = UnitCaseTestStatusException<Unit, Case, TestPassed>;

template <typename Unit, typename Case>
using TestCaseFailed = UnitCaseTestStatusException<Unit, Case, TestFailed>;

template <typename Unit, typename Case>
using TestCaseSkipped = UnitCaseTestStatusException<Unit, Case, TestSkipped>;

int runAllTests(int argc, char **argv);

void setOutputStream(std::ostream &os);
void setOutputStream(const RCP<Teuchos::FancyOStream> &os);
RCP<Teuchos::FancyOStream> getOutputStream(void);

enum TestStatus
{
  Skip = -1,     // test skipped (test is not run in this rank)
  Success = 0,   // pass
  Failed = 1,    // test failed (test failure exception thrown)
  Exception = 2, // unhandled exception thrown
};

class TestUnit
{
  bool failed = false;

public:
  virtual const char *name() const = 0;
  virtual int requiredSize() const { return 1; }

  // mark as 'failed' without aborting test
  void markFailed() { failed = true; }
  bool isFailed() const { return failed; }

  virtual void setup() = 0;
  virtual void teardown(TestStatus stat, const std::exception *exception) = 0;
  virtual void execute() = 0;

  [[noreturn]] virtual void throwPass(const std::string &msg) const;
  [[noreturn]] virtual void throwSkip(const std::string &msg) const;
  [[noreturn]] virtual void throwFail(const std::string &msg) const;
};

template <typename Ordinal> class TestUnitBase : public TestUnit
{
public:
  /**
   * TestUnit-local communicator
   */
  virtual RCP<const Teuchos::Comm<Ordinal>> comm() const = 0;

  /**
   * Global communicator
   */
  RCP<const Teuchos::Comm<Ordinal>> globalComm() const
  {
    return ttest::globalComm<Ordinal>();
  }
};

class SingleTestUnit : public TestUnitBase<int>
{
public:
  virtual RCP<const Teuchos::Comm<int>> comm() const override
  {
    return Teuchos::DefaultComm<int>::getDefaultSerialComm(globalComm());
  }

  virtual void setup() override
  {
    if (requiredSize() > 1)
      throw std::logic_error(
          "Base TestUnit class does not support running in parallel");

    auto comm = globalComm();
    if (comm && comm->getRank() > 0)
      throwSkip("Not running test in excess rank");
  }

  virtual void teardown(TestStatus stat,
                        const std::exception *exception) override
  {
    auto comm = globalComm();
    int istat = stat;
    int ostat = stat;
    if (comm)
      Teuchos::reduceAll(*comm, Teuchos::REDUCE_MAX, 1, &istat, &ostat);
    if (ostat != stat) {
      switch ((TestStatus)ostat) {
      case Skip:
        break;
      case Success:
        throwPass("In another rank, test has been run and passed");
      case Failed:
        throwFail("In another rank, test has been failed");
      case Exception:
        throw std::runtime_error(
            "An exception has been thrown in another rank");
      }
      throwSkip("(unreachable reached)");
    }
  }
};

template <typename Base>
class UnitTestStatusException<TestUnit, Base> : public Base
{
  const char *unit_name;

public:
  UnitTestStatusException(const char *u, const std::string &msg)
      : unit_name(u), Base(msg)
  {
  }

  virtual const char *unitName() const override { return unit_name; }
};

inline void TestUnit::throwPass(const std::string &msg) const
{
  throw UnitTestStatusException<TestUnit, TestPassed>(name(), msg);
}

inline void TestUnit::throwSkip(const std::string &msg) const
{
  throw UnitTestStatusException<TestUnit, TestSkipped>(name(), msg);
}

inline void TestUnit::throwFail(const std::string &msg) const
{
  throw UnitTestStatusException<TestUnit, TestFailed>(name(), msg);
}

template <typename Unit> class TestCase
{
  bool failed = false;

public:
  Unit *const unit;

  TestCase(Unit *u) : unit(u) {}

  void markFailed() { failed = true; }
  bool isFailed() const { return failed; }

  auto comm() const { return unit->comm(); }

  virtual const char *name() const = 0;
  virtual void execute() = 0;

  [[noreturn]] virtual void throwPass(const std::string &msg) const;
  [[noreturn]] virtual void throwSkip(const std::string &msg) const;
  [[noreturn]] virtual void throwFail(const std::string &msg) const;
};

template <typename Unit, typename Base>
class UnitTestStatusException<TestCase<Unit>, Base> : public Base
{
  const char *unit_name;
  const char *case_name;

public:
  UnitTestStatusException(const char *u, const char *c, const std::string &msg)
      : unit_name(u), case_name(c), Base(msg)
  {
  }

  virtual const char *unitName() const override { return unit_name; }
  virtual const char *caseName() const override { return case_name; }
};

template <typename Unit>
void TestCase<Unit>::throwPass(const std::string &msg) const
{
  throw UnitTestStatusException<TestCase<Unit>, TestPassed>(unit->name(),
                                                            name(), msg);
}

template <typename Unit>
void TestCase<Unit>::throwSkip(const std::string &msg) const
{
  throw UnitTestStatusException<TestCase<Unit>, TestSkipped>(unit->name(),
                                                             name(), msg);
}

template <typename Unit>
void TestCase<Unit>::throwFail(const std::string &msg) const
{
  throw UnitTestStatusException<TestCase<Unit>, TestFailed>(unit->name(),
                                                            name(), msg);
}

template <typename Unit, typename Case> class TestUnitCaseGenerator
{
public:
  virtual RCP<Case> generate(Unit *unit) const = 0;
};

template <typename Unit> class TestUnitCaseAdder;

template <typename Unit, typename Case> class TestUnitCommon
{
public:
  using TestCaseGeneratorType = TestUnitCaseGenerator<Unit, Case>;

private:
  using case_list_type = std::vector<const TestCaseGeneratorType *>;

  static case_list_type &cases()
  {
    static case_list_type list;
    return list;
  }

  friend TestUnitCaseAdder<Unit>;

  template <typename U> void check()
  {
    static_assert(std::is_base_of_v<TestUnitCommon<U, Case>, U>,
                  "Unit does not derive TestUnitCommon<Unit>");
  }

protected:
  virtual ~TestUnitCommon() = default;

  virtual void setupCase(const RCP<const Case> &testcase) = 0;
  virtual void teardownCase(const RCP<const Case> &testcase, TestStatus stat,
                            const std::exception *exception) = 0;
  virtual void markFailed() = 0;

  void runCases()
  {
    RCP<Teuchos::FancyOStream> outp = getOutputStream();
    Teuchos::FancyOStream &out = *outp;
    bool showProcRank =
        dynamic_cast<Teuchos::FancyOStream::streambuf_t *>(out.rdbuf())
            ->getShowProcRank();
    int root = out.getOutputToRootOnly();

    check<Unit>();
    Unit *unit = dynamic_cast<Unit *>(this);
    for (auto &generator : cases()) {
      auto testcase = generator->generate(unit);
      out.setShowProcRank(showProcRank);
      out.setOutputToRootOnly(root);
      try {
        try {
          setupCase(testcase);
          testcase->execute();
          if (testcase->isFailed())
            testcase->throwFail("Soft failed");
        } catch (const TestSkipped &) {
          teardownCase(testcase, Skip, nullptr);
          throw;
        } catch (const TestFailed &) {
          teardownCase(testcase, Failed, nullptr);
          throw;
        } catch (const std::exception &e) {
          teardownCase(testcase, Exception, &e);
          throw;
        } catch (...) {
          teardownCase(testcase, Exception, nullptr);
          throw;
        }
        teardownCase(testcase, Success, nullptr);
        testcase->throwPass("");
      } catch (const TestPassed &) {
        out.setShowProcRank(false);
        out.setOutputToRootOnly(0);
        out << "PASS: " << testcase->name() << "\n";
      } catch (const TestSkipped &) {
        out.setShowProcRank(false);
        out.setOutputToRootOnly(0);
        out << "SKIP: " << testcase->name() << "\n";
      } catch (const TestFailed &) {
        out.setShowProcRank(false);
        out.setOutputToRootOnly(0);
        out << "FAIL: " << testcase->name() << "\n";
        markFailed();
      } catch (std::exception &) {
        out.setShowProcRank(false);
        out.setOutputToRootOnly(0);
        out << "EXCEPTION: " << testcase->name() << "\n";
        out.setShowProcRank(showProcRank);
        out.setOutputToRootOnly(root);
        throw;
      } catch (...) {
        out.setShowProcRank(false);
        out.setOutputToRootOnly(0);
        out << "EXCEPTION: " << testcase->name() << "\n";
        out.setShowProcRank(showProcRank);
        out.setOutputToRootOnly(root);
        throw;
      }
    }
  }
};

template <typename Unit> class TestUnitCaseAdder
{
public:
  template <typename S> TestUnitCaseAdder(const S *generator)
  {
    Unit::cases().push_back(generator);
  }
};

template <typename D, long L> class It
{
};

template <typename Base, typename Unit, typename Case>
class CommonBase : public Base, public TestUnitCommon<Unit, Case>
{
  class MarkFailed
  {
  public:
    CommonBase<Base, Unit, Case> *this_p;

    void operator()() const { this_p->markFailed(); }
  };

public:
  virtual const char *name() const override;
  virtual void setup() override { Base::setup(); }
  virtual void markFailed() override { Base::markFailed(); }

  virtual void teardown(TestStatus stat, const std::exception *e) override
  {
    Base::teardown(stat, e);
  }

  virtual void execute() override { TestUnitCommon<Unit, Case>::runCases(); }
};

template <typename Unit>
class CommonTestUnit : public CommonBase<SingleTestUnit, Unit, TestCase<Unit>>
{
public:
  using TestCaseType = TestCase<Unit>;

protected:
  virtual void setupCase(const RCP<const TestCase<Unit>> &testcase) override {}

  virtual void teardownCase(const RCP<const TestCase<Unit>> &testcase,
                            TestStatus stats, const std::exception *e) override
  {
  }
};

class TestUnitGeneratorBase
{
protected:
  void addTest() const;

public:
  TestUnitGeneratorBase();

  virtual RCP<TestUnit> generate() const = 0;
  virtual const char *name() const = 0;
};

template <typename Unit> class TestUnitNameTrait;

template <typename Unit> class TestUnitGenerator : public TestUnitGeneratorBase
{
public:
  TestUnitGenerator() : TestUnitGeneratorBase() { addTest(); }

  RCP<TestUnit> generate() const override { return Teuchos::rcp(new Unit); }
  const char *name() const override { return TestUnitNameTrait<Unit>::name; }
};

template <typename Unit, typename Base>
const char *UnitTestStatusException<Unit, Base>::unitName() const
{
  return TestUnitNameTrait<Unit>::name;
}

template <typename Unit, typename Case, typename Base>
const char *UnitCaseTestStatusException<Unit, Case, Base>::caseName() const
{
  return Case::NameData::name;
}

template <typename Base, typename Unit, typename Case>
const char *CommonBase<Base, Unit, Case>::name() const
{
  return TestUnitNameTrait<Unit>::name;
}

/**
 * Test unit that just write list of all tests
 */
class ListAllTests : public SingleTestUnit
{
public:
  const char *name() const override;
  void execute() override;
};

template <> class TestUnitNameTrait<ListAllTests>
{
public:
  static constexpr const char name[] = "ListAllTests";
};

} // namespace ttest

#define TEST_BASE_CORE(unit_class_name, unit_base_class, unit_name, DECL)      \
  class unit_class_name : public unit_base_class                               \
  {                                                                            \
  public:                                                                      \
    const char *name() const override;                                         \
    void throwPass(const std::string &msg) const override                      \
    {                                                                          \
      throw ::ttest::TestUnitPassed<unit_class_name>(msg);                     \
    }                                                                          \
    void throwSkip(const std::string &msg) const override                      \
    {                                                                          \
      throw ::ttest::TestUnitSkipped<unit_class_name>(msg);                    \
    }                                                                          \
    void throwFail(const std::string &msg) const override                      \
    {                                                                          \
      throw ::ttest::TestUnitFailed<unit_class_name>(msg);                     \
    }                                                                          \
                                                                               \
    DECL                                                                       \
  };                                                                           \
                                                                               \
  template <> class ttest::TestUnitNameTrait<unit_class_name>                  \
  {                                                                            \
  public:                                                                      \
    static constexpr const char name[] = unit_name;                            \
  };                                                                           \
                                                                               \
  const char *unit_class_name::name() const                                    \
  {                                                                            \
    return ttest::TestUnitNameTrait<unit_class_name>::name;                    \
  }                                                                            \
                                                                               \
  namespace ttest::tests_data                                                  \
  {                                                                            \
  static TestUnitGenerator<unit_class_name> test_##unit_class_name;            \
  }

/**
 * Define test with directly override ttest::TestUnit::execute()
 *
 * You cannot use DESCRIBE() and IT() with this macro.
 *
 * ```
 * class custom_setup_teardown : public ::ttest::TestUnit
 * {
 *   //...
 * };
 *
 * TEST_E(test_name, custom_setup_teardown, "test_name")
 * {
 *   // directly write code to test
 * }
 * ```
 */
#define TEST_E(unit_class, unit_base_class, unit_name)                         \
  TEST_BASE_CORE(unit_class, unit_base_class, unit_name,                       \
                 virtual void execute() override;)                             \
  void unit_class::execute()

#define TEST_C(unit_class, unit_base_template, unit_name)                      \
  TEST_BASE_CORE(unit_class, unit_base_template<unit_class>, unit_name, )      \
  namespace _ttest_##unit_class##_cases                                        \
  {                                                                            \
    using _ttest_unit = unit_class;                                            \
    namespace _impl                                                            \
    {                                                                          \
    using parent = void;                                                       \
    }                                                                          \
  }                                                                            \
  namespace _ttest_##unit_class##_cases

/**
 * DESCRIBE/IT test
 *
 * ```
 * TEST(test_class)
 * {
 *   // Here is not an execution block (namespace)
 *
 *   DESCRIBE(test_method)
 *   {
 *     // Again, here is not an execution block (namespace)
 *
 *     IT("works in a way") {
 *       // test code...
 *     }
 *
 *     IT("works in another way") {
 *       // test code...
 *     }
 *   }
 *
 *   DESCRIBE(test_another_method)
 *   {
 *     // ...
 *   }
 * }
 * ```
 *
 * To customize setup/teardown:
 *
 * ```
 * template <typename Unit>
 * class custom_setup_teardown : public ::ttest::CommonTestUnit<Unit>
 * {
 *   //...
 * };
 *
 * TEST_C(test_class, custom_setup_teardown, "test_class")
 * {
 *   DESCRIBE("something")
 *   {
 *     IT("works") {
 *       this->unit->...; // Access to data defined while setup
 *     }
 *   }
 * }
 * ```
 */
#define TEST(unit_class)                                                       \
  TEST_C(unit_class, ::ttest::CommonTestUnit, #unit_class)

#define DESCRIBE_X(dname, string_name)                                         \
  namespace _ttest_desc_##dname                                                \
  {                                                                            \
    class _meta_##dname                                                        \
    {                                                                          \
    public:                                                                    \
      using parent = _impl::parent;                                            \
      static constexpr const char name[] = string_name;                        \
      template <typename T> class CaseNameTrait;                               \
      template <typename T> class TestCase;                                    \
      template <typename T> class CaseGenerator;                               \
    };                                                                         \
    namespace _impl                                                            \
    {                                                                          \
    using parent = _meta_##dname;                                              \
    }                                                                          \
  }                                                                            \
  namespace _ttest_desc_##dname

#define DESCRIBE(name) DESCRIBE_X(name, #name)

#define IT_DESC(b, p, l)                                                       \
  ::ttest::ItDescription<                                                      \
      b, p, _impl::parent::CaseNameTrait<::ttest::It<_ttest_unit, l>>>

#define IT_X(description, l)                                                   \
  template <> class _impl::parent::CaseNameTrait<::ttest::It<_ttest_unit, l>>  \
  {                                                                            \
  public:                                                                      \
    static constexpr const char name[] = description;                          \
  };                                                                           \
  template <>                                                                  \
  class _impl::parent::TestCase<::ttest::It<_ttest_unit, l>>                   \
      : public _ttest_unit::TestCaseType                                       \
  {                                                                            \
  public:                                                                      \
    using NameData = IT_DESC(::ttest::TestUnitNameTrait<_ttest_unit>,          \
                             _impl::parent, l);                                \
    TestCase(_ttest_unit *unit) : _ttest_unit::TestCaseType(unit) {}           \
                                                                               \
    virtual const char *name() const override { return NameData::name; }       \
    virtual void execute() override;                                           \
                                                                               \
    void throwPass(const std::string &msg) const override                      \
    {                                                                          \
      throw ::ttest::TestCasePassed<                                           \
          _ttest_unit, _impl::parent::TestCase<::ttest::It<_ttest_unit, l>>>(  \
          msg);                                                                \
    }                                                                          \
    void throwSkip(const std::string &msg) const override                      \
    {                                                                          \
      throw ::ttest::TestCaseSkipped<                                          \
          _ttest_unit, _impl::parent::TestCase<::ttest::It<_ttest_unit, l>>>(  \
          msg);                                                                \
    }                                                                          \
    void throwFail(const std::string &msg) const override                      \
    {                                                                          \
      throw ::ttest::TestCaseFailed<                                           \
          _ttest_unit, _impl::parent::TestCase<::ttest::It<_ttest_unit, l>>>(  \
          msg);                                                                \
    }                                                                          \
  };                                                                           \
  template <>                                                                  \
  class _impl::parent::CaseGenerator<::ttest::It<_ttest_unit, l>>              \
      : public _ttest_unit::TestCaseGeneratorType                              \
  {                                                                            \
  public:                                                                      \
    ::Teuchos::RCP<_ttest_unit::TestCaseType>                                  \
    generate(_ttest_unit *unit) const override                                 \
    {                                                                          \
      return ::Teuchos::rcp(                                                   \
          new _impl::parent::TestCase<::ttest::It<_ttest_unit, l>>(unit));     \
    }                                                                          \
  };                                                                           \
  static _impl::parent::CaseGenerator<::ttest::It<_ttest_unit, l>>             \
      _ttest_unit_generator_##l;                                               \
  static ::ttest::TestUnitCaseAdder<_ttest_unit> _ttest_unit_##l(              \
      &_ttest_unit_generator_##l);                                             \
  void _impl::parent::TestCase<::ttest::It<_ttest_unit, l>>::execute()

#define IT_E(description, l) IT_X(description, l)

#ifdef __COUNTER__
#define IT_U(description) IT_E(description, __COUNTER__)
#else
#define IT_U(description) IT_E(description, __LINE__)
#endif

#define IT(description) IT_U(description)

#endif
