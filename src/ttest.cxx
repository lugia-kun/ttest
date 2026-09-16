#include "ttest.hpp"
#include "Teuchos_CommHelpers.hpp"
#include "ttest_mpi.hpp"

#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_FancyOStream.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_TypeNameTraits.hpp>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ttest
{
static const char *argv0 = nullptr;

const char *runnerExec() { return argv0; }

using test_list_type =
    std::vector<std::pair<const char *, const TestUnitGeneratorBase *>>;

static test_list_type &tests()
{
  static test_list_type testCases;
  return testCases;
}

TestUnitGeneratorBase::TestUnitGeneratorBase() {}

void TestUnitGeneratorBase::addTest() const
{
  tests().push_back(std::make_pair(name(), this));
}

static RCP<Teuchos::FancyOStream> &outputStream()
{
  static RCP<Teuchos::FancyOStream> os;
  return os;
}

void setOutputStream(std::ostream &os)
{
  outputStream() = Teuchos::getFancyOStream(Teuchos::rcp(&os, false));
}

void setOutputStream(const RCP<Teuchos::FancyOStream> &os)
{
  outputStream() = os;
}

RCP<Teuchos::FancyOStream> getOutputStream(void)
{
  if (!outputStream())
    setOutputStream(std::cout);
  return outputStream();
}

int runAllTests(int argc, char **argv)
{
  std::set<std::string> units;
  auto comm = Teuchos::DefaultComm<int>::getComm();

  if (argc > 0) {
    argv0 = argv[0];
    int i = 1;
    if (argc > 1) {
      /* disable output */
      if (std::string(argv[i]) == "-") {
        auto blackhole = Teuchos::rcp(new Teuchos::oblackholestream);
        auto fancy = Teuchos::rcp(new Teuchos::FancyOStream(blackhole));
        setOutputStream(fancy);
        ++i;
      }
    }
    for (; i < argc; ++i) {
      std::string arg = argv[i];
      units.insert(arg);
      if (std::find_if(tests().begin(), tests().end(), [&](const auto &i) {
            return arg == i.first;
          }) == tests().end()) {
        throw std::invalid_argument("No test found with: " + arg);
      }
    }
  }

  RCP<Teuchos::FancyOStream> outp = getOutputStream();
  auto &out = *outp;
  bool showProcRank = false;
  if (comm) {
    showProcRank = true;
    out.setProcRankAndSize(comm->getRank(), comm->getSize());
  } else {
    out.setProcRankAndSize(0, 1);
  }
  out.setOutputToRootOnly(0);

  bool failed = false;
  for (auto &[name, unit_gen] : tests()) {
    if (!units.empty() && units.find(name) == units.end())
      continue;
    auto unit = unit_gen->generate();
    try {
      try {
        out.setShowProcRank(false);
        out.setOutputToRootOnly(0);
        out << "[   RUN   ] Test " << name << "\n";
        if (comm)
          Teuchos::barrier(*comm);
        out.setShowProcRank(showProcRank);
        out.setOutputToRootOnly(-1);
        unit->setup();
        unit->execute();
        if (unit->isFailed())
          unit->throwFail("Soft failed");
      } catch (const TestSkipped &) {
        unit->teardown(Skip, nullptr);
        throw;
      } catch (const TestFailed &) {
        unit->teardown(Failed, nullptr);
        throw;
      } catch (const std::exception &e) {
        throw;
      } catch (...) {
        unit->teardown(Exception, nullptr);
        throw;
      }
      unit->teardown(Success, nullptr);
      unit->throwPass("");
    } catch (const TestPassed &) {
      out.setShowProcRank(false);
      out.setOutputToRootOnly(0);
      out << "[ SUCCESS ] Test " << name << "\n";
    } catch (const TestSkipped &e) {
      out.setShowProcRank(false);
      out.setOutputToRootOnly(0);
      out << "[  SKIP   ] Test " << name << "\n";
      if (comm)
        Teuchos::barrier(*comm);
      out.setShowProcRank(showProcRank);
      out.setOutputToRootOnly(-1);
      {
        Teuchos::OSTab tab(out, 2);
        out << e.what() << "\n";
      }
    } catch (const TestFailed &) {
      out.setShowProcRank(false);
      out.setOutputToRootOnly(0);
      out << "[ FAILED  ] Test " << name << "\n";
      failed = true;
    } catch (const std::exception &e) {
      out.setShowProcRank(false);
      out.setOutputToRootOnly(0);
      out << "[EXCEPTION] Test " << name << "\n";
      if (comm)
        Teuchos::barrier(*comm);
      out.setShowProcRank(showProcRank);
      out.setOutputToRootOnly(-1);
      {
        Teuchos::OSTab tab(out, 2);
        out << "Exception thrown with message:\n\n"
            << e.what() << "\n\nType: " << Teuchos::typeName(e) << "\n";
      }
      failed = true;
    } catch (...) {
      out.setShowProcRank(false);
      out.setOutputToRootOnly(0);
      if (comm)
        Teuchos::barrier(*comm);
      out.setShowProcRank(showProcRank);
      out.setOutputToRootOnly(-1);
      {
        Teuchos::OSTab tab(out, strlen("[EXCEPTION] "));
        out << "Custom exception (not in std::exception) thrown\n";
      }
      throw;
    }
  }
  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

const char *ListAllTests::name() const
{
  return TestUnitNameTrait<ListAllTests>::name;
}

void ListAllTests::execute()
{
  auto outp = getOutputStream();
  auto &out = *outp;

  out << "Available tests:\n";

  for (auto &[name, unit_gen] : tests())
    out << " * " << name << "\n";

  out << "Required number of MPI proceses:\n";

  int max = 0;
  for (auto &[name, unit_gen] : tests()) {
    auto unit = unit_gen->generate();
    int size = unit->requiredSize();
    if (max < size)
      max = size;

    out << " * " << name << " = " << size << "\n";
  }

  out << "Maximum required number of processes: " << max << "\n";
}

static TestUnitGenerator<ListAllTests> list_all_tests_generator;
} // namespace ttest
