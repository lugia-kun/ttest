#include "ttest.hpp"
#include "ttest_expect.hpp"

#include <Teuchos_GlobalMPISession.hpp>
#include <stdexcept>

int main(int argc, char **argv)
{
  Teuchos::GlobalMPISession session(&argc, &argv, nullptr);
  return ttest::runAllTests(argc, argv);
}

TEST(XX)
{
  DESCRIBE(YY)
  {
    DESCRIBE(ZZ)
    {
      IT("works") { *ttest::getOutputStream() << 1 << "\n"; }

      DESCRIBE(TT) {}

      // IT("e1") { expectThrow(throw 1, std::exception); }
      IT("e2") { expectThrow(throw 1, int); }
      IT("e3") { expectThrow(throw std::runtime_error(""), std::logic_error); }
      IT("e3") { expectThrow(throw std::runtime_error(""), std::exception); }
      IT("e4") { expectNoThrow((void)1); }
      IT("e5") { expectNoThrow(throw std::runtime_error("aa")); }
      IT("e5")
      {
        expectNoThrow(throw std::runtime_error("line1\nline2\nline3"));
      }
    }
  }
}

TEST_MPI_E(MPIP, int, 2, "MPIP")
{
  *ttest::getOutputStream() << "This is " << comm()->getRank() << "\n";
}

TEST_MPI_E(MPIQ, int, 1, "MPIQ")
{
  *ttest::getOutputStream() << "This is " << comm()->getRank() << " again\n";

  expect(1).should(EQ(3));
  expect(1).should(BE_NEAR(4, 1));

  if (comm()->getRank() == 3)
    this->markFailed();
}

TEST_COPY_MPI(MPIQ4, MPIQ, 2, "MPIQ:4")
TEST_COPY_MPI(MPIQ8, MPIQ, 3, "MPIQ:5")
TEST_COPY_MPI(MPIQ9, MPIQ, 4, "MPIQ:6")
TEST_COPY_MPI(MPIQ1, MPIQ, 5, "MPIQ:7")

TEST_MPI(MPIX, 4)
{
  DESCRIBE(T)
  {
    IT("works")
    {
      *ttest::getOutputStream() << this->comm()->getRank() << "\n";
      if (this->comm()->getRank() == 3)
        this->markFailed();
    }
  }
}

TEST_COPY_MPI(MPIX2, MPIX, 2, "MPIX:2")
