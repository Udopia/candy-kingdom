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

#include <candy/core/SolverTypes.h>
#include <candy/core/CNFProblem.h>


#include <candy/gates/GateAnalyzer.h>
#include <candy/randomsimulation/ClauseOrder.h>

#include <candy/testutils/TestGateStructure.h>
#include "TestUtils.h"

#include <unordered_set>

namespace Candy {
    
    // TODO: these tests do not cover Plaistedt-Greenbaum-optimized gate encodings yet.
    
    // TODO: test ClauseOrder with gate filter


    // utility functions
    
    /// Returns true iff the given sets of clause pointers are equal modulo item order.
    static bool equals(std::vector<Cl*>& a, const std::vector<const Cl*>& b);
    
    /// Returns true iff the given sets of T are equal modulo item order.
    template<typename T> static bool equals(const std::vector<T>& a, const std::vector<T>& b);
    
    /// Returns true iff the given iterable object contains the given thing.
    template<typename X, typename Y> static bool contains(X* iterable, Y thing);
    
    /// Returns true iff the given gate analyzer contains exactly the given clauses containing the
    /// given output literal, for the rsp. gate with the given output literal.
    static bool containsClauses(GateAnalyzer& analyzer, Lit outputLiteral, const std::vector<const Cl*>& clauses);
    
    /// Returns true iff all given clauses contain the given literal.
    static bool allClausesContain(Lit literal, const std::vector<const Cl*>& clauses);
    
    /// Returns true iff literals contains firstVar and secondVar, and firstVar appears before secondVar in literals.
    static bool appearsOrdered(Var firstVar, Var secondVar, const std::vector<Lit> &literals);
    
    
    
    static void test_noGates(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga { *formula };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 0);
        
        underTest.readGates(ga);
        EXPECT_EQ(underTest.getAmountOfVars(), 0ul);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 0ul);
        EXPECT_EQ(underTest.getInputVariables().size(), 0ul);
    }
    
    TEST(RSClauseOrderTest, noGates_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_noGates(*underTest);
    }
    
    TEST(RSClauseOrderTest, noGates_nonrecursiveImpl) {
        auto underTest = createNonrecursiveClauseOrder();
        test_noGates(*underTest);
    }
    
    
    
    static void test_singleGate(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0,1));
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga { *formula };
        ga.analyze();
        
        ASSERT_EQ(ga.getResult().getGateCount(), 1);
        
        underTest.readGates(ga);
        
        EXPECT_EQ(underTest.getAmountOfVars(), 3ul);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 1ul);
        EXPECT_EQ(underTest.getInputVariables().size(), 2ul);
        
        EXPECT_TRUE(equals(underTest.getInputVariables(), std::vector<Var>({1,2})));
        
        for (auto lit : underTest.getGateOutputsOrdered()) {
            EXPECT_TRUE(allClausesContain(lit, underTest.getClauses(0)));
            EXPECT_TRUE(containsClauses(ga, lit, underTest.getClauses(0)));
        }
    }
    
    TEST(RSClauseOrderTest, singleGate_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_singleGate(*underTest);
    }
    
    TEST(RSClauseOrderTest, singleGate_nonrecursiveImpl) {
        auto underTest = createNonrecursiveClauseOrder();
        test_singleGate(*underTest);
    }
    
    
    
    static void test_fewGates(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0,1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4, 1)}, Lit(1,1));
        gateBuilder->withAnd({Lit(5, 1), Lit(4, 1)}, Lit(3,1));
        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga { *formula };
        ga.analyze();
        
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        
        underTest.readGates(ga);
        
        EXPECT_EQ(underTest.getAmountOfVars(), 6ul);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 3ul);
        EXPECT_EQ(underTest.getInputVariables().size(), 3ul);
        
        EXPECT_TRUE(equals(underTest.getInputVariables(), std::vector<Var>({2,5,4})));

        
        EXPECT_TRUE(appearsOrdered(1, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 1, underTest.getGateOutputsOrdered()));
        
        for (auto lit : underTest.getGateOutputsOrdered()) {
            EXPECT_TRUE(allClausesContain(lit, underTest.getClauses(lit.var())));
            EXPECT_TRUE(containsClauses(ga, lit, underTest.getClauses(lit.var())));
        }
    }
    
    TEST(RSClauseOrderTest, fewGates_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_fewGates(*underTest);
    }
    
    TEST(RSClauseOrderTest, fewGates_nonrecursiveImpl) {
        auto underTest = createNonrecursiveClauseOrder();
        test_fewGates(*underTest);
    }
    
    
    
    static void test_manyGates(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0,1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4, 1)}, Lit(1,1));
        
        gateBuilder->withAnd({Lit(5, 1), Lit(4, 1), Lit(6, 1)}, Lit(3,0));
        gateBuilder->withAnd({Lit(7, 0), Lit(6, 0)}, Lit(4,1));
        
        
        gateBuilder->withOr({Lit(8, 1), Lit(9,0), Lit(5,0)}, Lit(7,0));
        gateBuilder->withAnd({Lit(10, 1), Lit(9,0), Lit(6,0)}, Lit(8,0));
        gateBuilder->withAnd({Lit(11, 1), Lit(12,0), Lit(13,0)}, Lit(5,0));
        

        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga { *formula, GateRecognitionMethod::Patterns };
        ga.analyze();
        
        ASSERT_EQ(ga.getResult().getGateCount(), 7);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1ul);
        ASSERT_EQ(ga.getResult().getRoots().front()->size(), 1ul);
        
        underTest.readGates(ga);
        
        EXPECT_EQ(underTest.getAmountOfVars(), 14ul);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 7ul);
        EXPECT_EQ(underTest.getInputVariables().size(), 7ul);
        
        EXPECT_TRUE(equals(underTest.getInputVariables(), std::vector<Var>({2,6,9,10,11,12,13})));
        
        
        EXPECT_TRUE(appearsOrdered(1, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 1, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(4, 3, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(7, 4, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(8, 7, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(5, 3, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(5, 7, underTest.getGateOutputsOrdered()));
        
        for (auto lit : underTest.getGateOutputsOrdered()) {
            EXPECT_TRUE(allClausesContain(lit, underTest.getClauses(lit.var())));
            EXPECT_TRUE(containsClauses(ga, lit, underTest.getClauses(lit.var())));
        }
    }
    
    TEST(RSClauseOrderTest, manyGates_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_manyGates(*underTest);
    }
    
    TEST(RSClauseOrderTest, manyGates_nonrecursiveImpl) {
        auto underTest = createNonrecursiveClauseOrder();
        test_manyGates(*underTest);
    }

    
    
    
    
    
    static bool equals(std::vector<Cl*>& a, const std::vector<const Cl*>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        
        std::unordered_set<const Cl*> set_a;
        std::unordered_set<const Cl*> set_b;
        
        set_a.insert(a.begin(), a.end());
        set_b.insert(b.begin(), b.end());
        
        return set_a == set_b;
    }
    
    template<typename T>
    static bool equals(const std::vector<T>& a, const std::vector<T>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        
        std::unordered_set<T> set_a;
        std::unordered_set<T> set_b;
        
        set_a.insert(a.begin(), a.end());
        set_b.insert(b.begin(), b.end());
        
        return set_a == set_b;
    }
    
    template<typename X, typename Y>
    static bool contains(X* iterable, Y thing) {
        return std::find(iterable->begin(), iterable->end(), thing) != iterable->end();
    }
    
    static bool containsClauses(GateAnalyzer& analyzer, Lit outputLiteral, const std::vector<const Cl*>& clauses) {
        auto &gate = analyzer.getResult().getGate(outputLiteral);
        return gate.isDefined() && (equals(gate.fwd, clauses) || equals(gate.bwd, clauses));
    }
    
    static bool allClausesContain(Lit literal, const std::vector<const Cl*>& clauses) {
        for (auto clause : clauses) {
            if (!contains(clause, literal)) {
                return false;
            }
        }
        return true;
    }
    
    static bool appearsOrdered(Var firstVar, Var secondVar, const std::vector<Lit> &literals) {
        bool foundFirstLit = false;
        bool foundSecondLit = false;
        for (auto lit : literals) {
            foundFirstLit |= lit.var() == firstVar;
            foundSecondLit |= lit.var() == secondVar;
            if (foundSecondLit && !foundFirstLit) {
                return false;
            }
        }
        return foundFirstLit && foundSecondLit;
    }
}
