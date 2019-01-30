/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <candy/rsil/BranchingHeuristics.h>
#include <candy/testutils/TestUtils.h>
#include <candy/core/Solver.h>
#include <candy/core/Trail.h>
#include <candy/randomsimulation/RandomSimulator.h>
#include <candy/gates/GateAnalyzer.h>
#include <candy/rsar/Heuristics.h>
#include <candy/frontend/CandyBuilder.h>

namespace Candy {
    
    using TestedRSILBranchingHeuristic = RSILBranchingHeuristic3;
    using TestedRSILVanishingBranchingHeuristic = RSILVanishingBranchingHeuristic3;
    using TestedRSILBudgetBranchingHeuristic = RSILBudgetBranchingHeuristic3;
    
    template<class Heuristic>
    static void test_uninitializedHeuristicReturnsUndefForMinInput() {
        ClauseDatabase clause_db;
        Trail trail(1);

        Heuristic underTest { clause_db, trail };

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(1_L);
        auto result = underTest.getAdvice();

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_emptyInitializedHeuristicReturnsUndefForMinInput() {
        Conjectures empty{};

        ClauseDatabase clause_db;
        Trail trail(1);

        Heuristic underTest { clause_db, trail };

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(1_L);
        auto result = underTest.getAdvice();

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesAdviceForSingleEquivalence() {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});

        ClauseDatabase clause_db;
        Trail trail(5);
        
        Heuristic underTest { clause_db, trail };
        underTest.init(std::move(testData));
        
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);
        auto result = underTest.getAdvice();

        ASSERT_NE(result, lit_Undef);
        EXPECT_EQ(result, mkLit(2, 0));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfAssigned(std::unique_ptr<Conjectures> testData) {
        ClauseDatabase clause_db;
        Trail trail(5);

        Heuristic underTest { clause_db, trail };
        underTest.init(std::move(testData));

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(1_L);
        trail.uncheckedEnqueue(2_L);
        trail.uncheckedEnqueue(3_L);
        underTest.grow(5);
        auto result = underTest.getAdvice();

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILVanishingBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILBudgetBranchingHeuristic>(std::move(testData));
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision(std::unique_ptr<Conjectures> testData) {
        ClauseDatabase clause_db;
        Trail trail(5);
        
        Heuristic underTest { clause_db, trail };
        underTest.init(std::move(testData));

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);
        underTest.setDecisionVar(3_V, false);
        auto result = underTest.getAdvice();

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILVanishingBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILBudgetBranchingHeuristic>(std::move(testData));
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfIrrelevant(std::unique_ptr<Conjectures> testData) {
        ClauseDatabase clause_db;
        Trail trail(5);
        
        Heuristic underTest { clause_db, trail };
        underTest.init(std::move(testData));

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(5_L);
        underTest.grow(5);
        auto result = underTest.getAdvice();

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILVanishingBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILBudgetBranchingHeuristic>(std::move(testData));
    }
    
    template<class Heuristic>
    static void test_givesAdviceForSingleEquivalenceSize3(std::unique_ptr<Conjectures> testData) {
        ClauseDatabase clause_db;
        Trail trail(5);
        
        Heuristic underTest { clause_db, trail };
        underTest.init(std::move(testData));

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(4_L);
        underTest.grow(5);
        auto result = underTest.getAdvice();

        ASSERT_NE(result, lit_Undef);        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILVanishingBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILBudgetBranchingHeuristic>(std::move(testData));
    }
    
    template<class Heuristic>
    static void test_travelsUpTrail(std::unique_ptr<Conjectures> testData) {
        ClauseDatabase clause_db;
        Trail trail(5);
        
        Heuristic underTest { clause_db, trail };
        underTest.init(std::move(testData));

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(4_L);
        trail.uncheckedEnqueue(5_L);
        underTest.grow(5);
        auto result = underTest.getAdvice();

        ASSERT_NE(result, lit_Undef);        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, travelsUpTrail) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        test_travelsUpTrail<TestedRSILBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, travelsUpTrail) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        test_travelsUpTrail<TestedRSILVanishingBranchingHeuristic>(std::move(testData));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, travelsUpTrail) {
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        test_travelsUpTrail<TestedRSILBudgetBranchingHeuristic>(std::move(testData));
    }
    
    namespace {
        class MockRSARHeuristic : public RefinementHeuristic {
        public:
            void beginRefinementStep() {} ;
            void markRemovals(EquivalenceImplications&) {};
            void markRemovals(Backbones&) {};
            MOCK_METHOD2(probe, bool(Var, bool));
        };
    }
    
    // TEST(RSILBranchingHeuristicsTests, givesNoAdviceForFilteredVariable) {
    //     ClauseDatabase clause_db;
    //     Trail trail(6);

    //     MockRSARHeuristic* mockRSARHeuristic = new MockRSARHeuristic;
        
    //     EXPECT_CALL(*mockRSARHeuristic, probe(testing::Ge(4), testing::_)).WillRepeatedly(testing::Return(false));
    //     EXPECT_CALL(*mockRSARHeuristic, probe(testing::Le(3), testing::_)).WillRepeatedly(testing::Return(true));
        
    //     std::unique_ptr<Conjectures> testData (new Conjectures());
    //     testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
    //     testData->addEquivalence(EquivalenceConjecture{{mkLit(4, 0), mkLit(5,1)}});

    //     TestedRSILBranchingHeuristic underTest { clause_db, trail };
    //     underTest.init(std::move(testData), false);

    //     trail.newDecisionLevel();
    //     trail.uncheckedEnqueue(4_L);
    //     underTest.grow(6);
    //     auto result = underTest.getAdvice();
        
    //     ASSERT_EQ(result, lit_Undef);
        
    //     trail.cancelUntil(0);
    //     trail.newDecisionLevel();
    //     trail.uncheckedEnqueue(5_L);
    //     result = underTest.getAdvice();
        
    //     ASSERT_EQ(result, mkLit(5,0));

    //     delete mockRSARHeuristic;
    // }
    
    TEST(RSILBranchingHeuristicsTests, givesNoBackboneAdviceWhenBackbonesDeactivated) {
        ClauseDatabase clause_db;
        Trail trail(5);

        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(5,1), mkLit(2, 0)}});
        testData->addBackbone(BackboneConjecture {mkLit(3,0)});
        testData->addBackbone(BackboneConjecture {mkLit(4,1)});
        
        TestedRSILBranchingHeuristic underTest { clause_db, trail };
        underTest.init(std::move(testData), false);
        
        // backbone used here if backbone-usage would be activated:
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,0)), mkLit(3,0));
        
        EXPECT_EQ(underTest.getSignAdvice(mkLit(1,0)), mkLit(1,0));
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,1)), mkLit(3,1));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesBackboneAdviceWhenBackbonesActivated) {
        ClauseDatabase clause_db;
        Trail trail(5);

        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(5,1), mkLit(2, 0)}});
        testData->addBackbone(BackboneConjecture {mkLit(3,0)});
        testData->addBackbone(BackboneConjecture {mkLit(4,1)});

        TestedRSILBranchingHeuristic underTest { clause_db, trail };
        underTest.init(std::move(testData), true);
        
        // backbone used here:
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,0)), mkLit(3,1));
        
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,1)), mkLit(3,1));
        EXPECT_EQ(underTest.getSignAdvice(mkLit(1,0)), mkLit(1,0));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, isFullyActiveInFirstPeriod) {
        ClauseDatabase clause_db;
        Trail trail(5);

        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});

        TestedRSILBranchingHeuristic underTest { clause_db, trail };
        underTest.init(std::move(testData), false);

        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);
        
        int calls = 0;
        for (int i = 0; i < 100; ++i) {
            auto result = underTest.getAdvice();
            ++calls;
            EXPECT_NE(result, lit_Undef) << "Unexpected undef at call " << calls;
        }
    }
    
    // TEST(RSILVanishingBranchingHeuristicsTests, activityMatchesExpectedDistribution) {
    //     ClauseDatabase clause_db;
    //     Trail trail(5);

    //     std::unique_ptr<Conjectures> testData (new Conjectures());

    //     const unsigned long long halfLife = 1000ull;
    //     const int stages = 20;

    //     testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});

    //     TestedRSILVanishingBranchingHeuristic underTest { clause_db, trail };
    //     underTest.init(std::move(testData), false, halfLife);

    //     trail.newDecisionLevel();
    //     trail.uncheckedEnqueue(1_L);
    //     trail.uncheckedEnqueue(~2_L);
    //     underTest.grow(5);
        
    //     std::unordered_map<uint8_t, double> distribution;
    //     std::unordered_map<uint8_t, double> referenceDistribution;
        
    //     for (int stage = 0; stage < stages; ++stage) {
    //         uint32_t definedResultCounter = 0;
    //         for (auto i = decltype(halfLife){0}; i < halfLife; ++i) {
    //             Lit result = underTest.getAdvice();
    //             definedResultCounter += (result == lit_Undef) ? 0 : 1;
    //         }
    //         distribution[stage] = static_cast<double>(definedResultCounter) / static_cast<double>(halfLife);
    //         referenceDistribution[stage] = 1.0f/static_cast<double>(1 << stage);
    //     }
        
    //     // simple Kolomogorov-Smirnov test
    //     double maxAbsDiff = getMaxAbsDifference(distribution, referenceDistribution);
    //     EXPECT_LE(maxAbsDiff, 0.05f);
    // }
    
    TEST(RSILBudgetBranchingHeuristicsTests, producesUndefWhenBudgetIsDepleted) {
        ClauseDatabase clause_db;
        Trail trail(5);

        const uint64_t budget = 4ull;
        
        std::unique_ptr<Conjectures> testData (new Conjectures());
        testData->addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});

        TestedRSILBudgetBranchingHeuristic underTest { clause_db, trail };
        underTest.init(std::move(testData), false, budget);
        
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);

        for (uint64_t i = 0; i <= budget+2; ++i) {
            Lit result = underTest.getAdvice();
            if (i < budget) {
                ASSERT_NE(result, lit_Undef) << "Budgets should not have been depleted in round " << i+1;
            }
            else {
                ASSERT_EQ(result, lit_Undef) << "Budgets should have been depleted in round " << i+1;
            }
        }
        
        trail.cancelUntil(0);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(3_L);

        Lit result = underTest.getAdvice();
        ASSERT_NE(result, lit_Undef) << "The back implication's budget should not be depleted in this test";
    }

    static void test_acceptanceTest(std::string filename, bool expectedResult, std::string mode, unsigned int size = 3) {
        CNFProblem problem;
        problem.readDimacsFromFile(filename.c_str());
        ASSERT_FALSE(problem.getProblem().empty()) << "Could not read test problem file.";
        
        RSILOptions::opt_rsil_mode.set(mode.c_str());
        RSILOptions::opt_rsil_advice_size.set(size);

        CandySolverInterface* solver = createSolver(false, false, true, size);
        solver->init(problem);
        EXPECT_TRUE((expectedResult ? l_True : l_False) == solver->solve());
    }
    
    TEST(RSILBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest("problems/6s33.cnf", false, "unrestricted", 3);
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest("problems/6s33.cnf", false, "vanishing", 3);
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest("problems/6s33.cnf", false, "implicationbudgeted", 3);
    }

    TEST(RSILBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest("problems/6s33.cnf", false, "unrestricted", 2);
    }

    TEST(RSILVanishingBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest("problems/6s33.cnf", false, "vanishing", 2);
    }

    TEST(RSILBudgetBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest("problems/6s33.cnf", false, "implicationbudgeted", 2);
    }
}
