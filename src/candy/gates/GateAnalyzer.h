/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef GATE_ANALYZER
#define GATE_ANALYZER

#include <cstdlib>
#include <algorithm>
#include <memory>

#include <vector>
#include <set>

#include <climits>
#include <chrono>

#include "candy/core/SolverTypes.h"
#include "candy/utils/Runtime.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {
    
/**
 * \defgroup Gates
 */
    
typedef struct Gate {
    Lit out = lit_Undef;
    For fwd, bwd;
    bool notMono = false;
    std::vector<Lit> inp;

    inline bool isDefined() const { return out != lit_Undef; }
    inline Lit getOutput() const { return out; }

    
    inline const For& getForwardClauses() const { return fwd; }
    inline const For& getBackwardClauses() const { return bwd; }
    inline const std::vector<Lit>& getInputs() const  { return inp; }
    
    inline For& getForwardClauses() {
        return const_cast<For&>(static_cast<const Gate*>(this)->getForwardClauses());
    }

    inline For& getBackwardClauses() {
        return const_cast<For&>(static_cast<const Gate*>(this)->getBackwardClauses());
    }

    inline std::vector<Lit>& getInputs() {
        return const_cast<std::vector<Lit>&>(static_cast<const Gate*>(this)->getInputs());
    }
    
    inline bool hasNonMonotonousParent() const { return notMono; }
} Gate;


class GateAnalyzer {

public:
    GateAnalyzer(const CNFProblem& dimacs, 
        double timeout = GateRecognitionOptions::opt_gr_timeout, 
        int tries = GateRecognitionOptions::opt_gr_tries,
        bool patterns = GateRecognitionOptions::opt_gr_patterns, 
        bool semantic = GateRecognitionOptions::opt_gr_semantic, 
        bool holistic = GateRecognitionOptions::opt_gr_holistic,
        bool lookahead = GateRecognitionOptions::opt_gr_lookahead, 
        bool intensify = GateRecognitionOptions::opt_gr_intensify, 
        int lookahead_threshold = GateRecognitionOptions::opt_gr_lookahead_threshold,
        unsigned int conflict_budget = GateRecognitionOptions::opt_gr_semantic_budget);

    ~GateAnalyzer();

    // main analysis routine
    void analyze();

    // public getters
    int getGateCount() const { return nGates; }
    Gate& getGate(Lit output) { return gates[var(output)]; }
    const Gate& getGate(Lit output) const { return gates[var(output)]; }

    void printGates() {
        std::vector<Lit> outputs;
        std::vector<bool> done(problem.nVars());
        for (Cl* root : roots) {
            outputs.insert(outputs.end(), root->begin(), root->end());
        }
        for (size_t i = 0; i < outputs.size(); i++) {
            Gate& gate = getGate(outputs[i]);

            if (gate.isDefined() && !done[var(outputs[i])]) {
                done[var(outputs[i])] = true;
                printf("Gate with output ");
                printLiteral(gate.getOutput());
                printf("Is defined by clauses ");
                printFormula(gate.getForwardClauses(), false);
                printFormula(gate.getBackwardClauses(), true);
                outputs.insert(outputs.end(), gate.getInputs().begin(), gate.getInputs().end());
            }
        }
    }

    const std::vector<Cl*> getRoots() const {
        return roots;
    }

    // create unique list of root literals base on root clauses
    std::vector<Lit> getRootLiterals();
    
    // create unit-clause and use it as root node (brought back with cnf2aig integration)
    Lit normalizeRoots();

    // create list of clauses of satisfied branches of the gate (brought back with minimizer integration)
    For getPrunedProblem(Cl model);


    bool hasTimeout() const;

    const CNFProblem& problem;
    Runtime runtime;

private:
    // problem to analyze:
    CandySolverInterface* solver;
    std::vector<Lit> assumptions;

    // control structures:
    std::vector<For> index; // occurrence lists
    std::vector<char> inputs; // flags to check if both polarities of literal are used as input (monotonicity)

    // heuristic configuration:
    int maxTries = 0;
    bool usePatterns = false;
    bool useSemantic = false;
    bool useHolistic = false;
    bool useLookahead = false;
    bool useIntensification = false;
    int lookaheadThreshold = 10;
    unsigned int semanticConflictBudget = 0;

    // analyzer output:
    std::vector<Cl*> roots; // top-level clauses
    std::vector<Gate> gates; // stores gate-struct for every output
    int nGates = 0;
    Cl* artificialRoot; // top-level unit-clause that can be generated by normalizeRoots()

    bool hasArtificialRoot() {
        return artificialRoot != nullptr;
    }

    Cl* getArtificialRoot() {
        return artificialRoot;
    }

    void printFormula(For& f, bool nl = false) {
        for (Cl* c : f) printClause(*c);
        if (nl) printf("\n");
    }

    void printClause(Cl& c) {
        for (Lit it : c) {
          printLiteral(it);
          printf(" ");
        }
        printf("0\n");
    }

    // main analysis routines
    void analyze(std::vector<Lit>& candidates);
    std::vector<Lit> analyze(std::vector<Lit>& candidates, bool pat, bool sem, bool dec);

    // clause selection heuristic
    std::vector<Lit> getRarestLiterals(std::vector<For>& index);
    std::vector<Cl*> getBestRoots();

    // clause patterns of full encoding
    bool patternCheck(Lit o, For& fwd, For& bwd, std::set<Lit>& inputs);
    bool semanticCheck(Var o, For& fwd, For& bwd);

    // work in progress:
    bool isBlockedAfterVE(Lit o, For& f, For& g);

    // some helpers:
    bool isBlocked(Lit o, Cl& a, Cl& b) { // assert ~o \in a and o \in b
        for (Lit c : a) for (Lit d : b) if (c != ~o && c == ~d) return true;
        return false;
    }

    bool isBlocked(Lit o, For& f, For& g) { // assert ~o \in f[i] and o \in g[j]
        for (Cl* a : f) for (Cl* b : g) if (!isBlocked(o, *a, *b)) return false;
        return true;
    }

    bool isBlocked(Lit o, Cl* c, For& f) { // assert ~o \in c and o \in f[i]
        for (Cl* a : f) if (!isBlocked(o, *c, *a)) return false;
        return true;
    }

    bool fixedClauseSize(For& f, unsigned int n) {
        for (Cl* c : f) if (c->size() != n) return false;
        return true;
    }

    void removeFromIndex(std::vector<For>& index, Cl* clause) {
        for (Lit l : *clause) {
            For& h = index[l];
            h.erase(remove(h.begin(), h.end(), clause), h.end());
        }
    }

    void removeFromIndex(std::vector<For>& index, For& clauses) {
        For copy(clauses.begin(), clauses.end());
        for (Cl* c : copy) {
            removeFromIndex(index, c);
        }
    }

};

}
#endif
