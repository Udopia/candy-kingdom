#include <zlib.h>
#include <sys/resource.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "core/Dimacs.h"
#include "simp/SimpSolver.h"

using namespace Glucose;

TEST (TestTest, zeqz) {
  ASSERT_EQ (0.0, 0.0);
}

TEST (SolverTest, outputwithbudget) {
  SimpSolver S;
  //gzFile in = gzopen("/home/markus/cnf/sc14-app/vmpc_33.cnf", "rb");
  gzFile in = gzopen("vmpc_32.renamed-as.sat05-1919.cnf", "rb");
  parse_DIMACS(in, S);
  gzclose(in);

  // create test output:
  S.setConfBudget(150000);
  S.verbosity++;
  S.verbEveryConflicts = 10000;
  vec<Lit> dummy;
  testing::internal::CaptureStdout();
  S.solveLimited(dummy);
  std::string output = testing::internal::GetCapturedStdout();

  // create reference output (comment out once generated):
//  FILE* f = fopen("/home/markus/git/candy-kingdom/test/reference.txt", "wb");
//  fprintf(f, "%s", output.c_str());
//  fflush(f);
//  fclose(f);

  // compare reference and test output:
  std::ifstream t("reference.txt");
  std::string reference((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
  ASSERT_EQ (output, reference);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}