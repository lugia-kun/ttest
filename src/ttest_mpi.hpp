#ifndef TTEST_MPI_HPP
#define TTEST_MPI_HPP

// IWYU pragma: private, include "ttest.hpp"

#include <Teuchos_Array.hpp>
#include <Teuchos_Comm.hpp>
#include <Teuchos_CommHelpers.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_DefaultMpiComm.hpp>
#include <Teuchos_EReductionType.hpp>
#include <Teuchos_OpaqueWrapper.hpp>
#include <Teuchos_RCP.hpp>
#include <stdexcept>
#include <string>

#include "ttest_base.hpp"

namespace ttest
{
template <typename Ordinal> class MPITestUnit : public TestUnitBase<Ordinal>
{
private:
  RCP<const Teuchos::Comm<Ordinal>> comm_p = Teuchos::null;

public:
  RCP<const Teuchos::Comm<Ordinal>> comm() const override { return comm_p; }

  /**
   * Overriding note: subclass should call MPITestUnit::setup() before
   * your custom work to remaining process of setup() only for running tests.
   */
  virtual void setup() override
  {
    auto global_comm = globalComm<Ordinal>();
    const int size = global_comm ? global_comm->getSize() : 1;
    const int reqsz = this->requiredSize();
    if (size < reqsz) {
      this->throwSkip(
          "Not enough slots are available to run this test (available: " +
          std::to_string(size) + ", required: " + std::to_string(reqsz) + ")");
    } else if (size > reqsz) {
      const int rank = global_comm ? global_comm->getRank() : 0;
      comm_p = global_comm->split(rank < reqsz, rank);
      if (rank >= reqsz)
        this->throwSkip("Not running test in excess rank");
    } else {
      comm_p = global_comm;
    }
  }

  virtual void teardown(TestStatus stat,
                        const std::exception *exception) override
  {
    int istat = stat;
    int ostat = stat;
    auto global_comm = globalComm<Ordinal>();
    if (comm_p && comm_p->getSize() > global_comm->getSize()) {
      Teuchos::reduceAll(*comm_p, Teuchos::REDUCE_MAX, 1, &istat, &ostat);
      istat = ostat;
    }
    if (global_comm) {
      Teuchos::reduceAll(*global_comm, Teuchos::REDUCE_MAX, 1, &istat, &ostat);
    }
    if (ostat != stat) {
      switch ((TestStatus)ostat) {
      case Skip:
        break;
      case Success:
        this->throwPass("In another rank, test has been run and passed");
      case Failed:
        this->throwFail("In another rank, test has been failed");
      case Exception:
        throw std::runtime_error(
            "An exception has been thrown in another rank");
      }
      this->throwSkip("(unreachable reached)");
    }
  }
};

template <typename Unit, typename Ordinal>
class MPITestCase : public TestCase<Unit>
{
public:
  MPITestCase(Unit *unit) : TestCase<Unit>(unit) {}
};

template <typename Unit, typename Ordinal>
class CommonMPITestUnit
    : public CommonBase<MPITestUnit<Ordinal>, Unit, MPITestCase<Unit, Ordinal>>
{
public:
  using TestCaseType = MPITestCase<Unit, Ordinal>;

protected:
  virtual void
  setupCase(const RCP<const MPITestCase<Unit, Ordinal>> &testcase) override
  {
  }

  virtual void
  teardownCase(const RCP<const MPITestCase<Unit, Ordinal>> &testcase,
               TestStatus stats, const std::exception *e) override
  {
    auto comm = this->comm();
    int istat = stats;
    int ostat = stats;
    if (comm) {
      Teuchos::reduceAll(*comm, Teuchos::REDUCE_MAX, 1, &istat, &ostat);
    }
    if (ostat != stats) {
      switch ((TestStatus)ostat) {
      case Skip:
        break;
      case Success:
        testcase->throwPass("In another rank, test has been run and passed");
      case Failed:
        testcase->throwFail("In another rank, test has been failed");
      case Exception:
        throw std::runtime_error(
            "An exception has been thrown in another rank");
      }
      testcase->throwSkip("(unreachable reached)");
    }
  }
};

template <typename Ordinal> struct CommonMPITestUnitGen
{
  template <typename Unit> using type = CommonMPITestUnit<Unit, Ordinal>;
};

} // namespace ttest

#define TEST_MPI_E(unit_class, ordinal_type, required_size, unit_name)         \
  TEST_BASE_CORE(unit_class, ::ttest::MPITestUnit<ordinal_type>, unit_name,    \
                 virtual int requiredSize() const override;                    \
                 virtual void execute() override;)                             \
  inline int unit_class::requiredSize() const { return required_size; }        \
  void unit_class::execute()

/*
 * Define same test with different size
 */
#define TEST_COPY_MPI(unit_class, base_test, required_size, unit_name)         \
  TEST_BASE_CORE(                                                              \
      unit_class, base_test, unit_name,                                        \
      virtual int requiredSize() const override { return required_size; })

#define TEST_MPI_C(unit_class, unit_base_template, required_size, unit_name)   \
  TEST_BASE_CORE(unit_class, unit_base_template<unit_class>, unit_name,        \
                 virtual int requiredSize() const override;)                   \
  inline int unit_class::requiredSize() const { return required_size; }        \
  namespace _ttest_##unit_class##_cases                                        \
  {                                                                            \
    using _ttest_unit = unit_class;                                            \
    namespace _impl                                                            \
    {                                                                          \
    using parent = void;                                                       \
    }                                                                          \
  }                                                                            \
  namespace _ttest_##unit_class##_cases

#define TEST_MPI_O(unit_class, required_size, ordinal_type)                    \
  TEST_MPI_C(unit_class, ::ttest::CommonMPITestUnitGen<ordinal_type>::type,    \
             required_size, #unit_class)

#define TEST_MPI(unit_class, required_size)                                    \
  TEST_MPI_O(unit_class, required_size, int)

#endif
