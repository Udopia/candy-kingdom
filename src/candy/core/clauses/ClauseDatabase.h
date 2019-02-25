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

#include <vector>

#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/ClauseAllocator.h"
#include "candy/core/clauses/Certificate.h"
#include "candy/core/Trail.h"
#include "candy/frontend/CLIOptions.h"

#ifndef CANDY_CLAUSE_DATABASE
#define CANDY_CLAUSE_DATABASE

namespace Candy {

struct AnalysisResult {

	AnalysisResult() : 
		nConflicts(0), learnt_clause(), involved_clauses(), lbd(0), backtrack_level(0)
	{ }

	uint64_t nConflicts;
	std::vector<Lit> learnt_clause;
	std::vector<Clause*> involved_clauses;
	unsigned int lbd;
    unsigned int backtrack_level;

    void setLearntClause(std::vector<Lit>& learnt_clause_, std::vector<Clause*>& involved_clauses_, unsigned int lbd_, unsigned int backtrack_level_) {
        assert(lbd_ <= learnt_clause_.size());
        nConflicts++;
        learnt_clause.swap(learnt_clause_);
        involved_clauses.swap(involved_clauses_); 
        lbd = lbd_;
        backtrack_level = backtrack_level_;
    }

};

struct BinaryWatcher {
    Clause* clause;
    Lit other;

    BinaryWatcher(Clause* _clause, Lit _other)
        : clause(_clause), other(_other) { }

};

class ClauseDatabase {
private:
    ClauseAllocator allocator;
 
    std::vector<Clause*> clauses; // Working set of problem clauses

    const unsigned int persistentLBD;
    const bool keepMedianLBD;
    const bool recalculateLBD;

    std::vector<std::vector<BinaryWatcher>> binaryWatchers;

    Certificate certificate;

public:

    size_t nReduced, nReduceCalls;
    
    /* analysis result is stored here */
	AnalysisResult result;

    ClauseDatabase() : 
        allocator(), clauses(), 
        persistentLBD(ClauseDatabaseOptions::opt_persistent_lbd),
        keepMedianLBD(ClauseDatabaseOptions::opt_keep_median_lbd), 
        recalculateLBD(ClauseDatabaseOptions::opt_recalculate_lbd), 
        binaryWatchers(), 
        certificate(SolverOptions::opt_certified_file), 
        nReduced(0), nReduceCalls(0),
        result() 
    { }

    ~ClauseDatabase() { }

    void initBinaryWatchers() {
        for (std::vector<BinaryWatcher>& watcher : binaryWatchers) {
            watcher.clear();
        }
        for (Clause* clause : clauses) {
            if (clause->size() == 2) {
                binaryWatchers[toInt(~clause->first())].emplace_back(clause, clause->second());
                binaryWatchers[toInt(~clause->second())].emplace_back(clause, clause->first());
            }
        }
    }

    void reestimateClauseWeights(Trail& trail, std::vector<Clause*>& involved_clauses) {
        if (recalculateLBD) {
            for (Clause* clause : involved_clauses) {
                if (clause->isLearnt()) {
                    uint_fast16_t lbd = trail.computeLBD(clause->begin(), clause->end());
                    clause->setLBD(lbd);
                }
            }
        }
    }

    ClauseAllocator* createGlobalClauseAllocator() {
        return allocator.create_global_allocator();
    }

    void setGlobalClauseAllocator(ClauseAllocator* global_allocator) {
        allocator.set_global_allocator(global_allocator);
        this->clauses = allocator.collect();
        initBinaryWatchers();
    }

    typedef std::vector<Clause*>::const_iterator const_iterator;

    inline const_iterator begin() const {
        return clauses.begin();
    }

    inline const_iterator end() const {
        return clauses.end();
    }

    inline unsigned int size() const {
        return clauses.size();
    }

    inline const Clause* operator [](int i) const {
        return clauses[i];
    }

    inline void grow(size_t nVars) {
        if (binaryWatchers.size() < 2*nVars) {
            binaryWatchers.resize(nVars*2);
        }
    }

    void setLearntClause(std::vector<Lit>& learnt_clause_, std::vector<Clause*>& involved_clauses_, unsigned int lbd_, unsigned int backtrack_level_) {
        result.setLearntClause(learnt_clause_, involved_clauses_, lbd_, backtrack_level_);
    }

    void emptyClause() {
        certificate.proof();
    }

    template<typename Iterator>
    inline Clause* createClause(Iterator begin, Iterator end, unsigned int lbd = 0) {
        Clause* clause = new (allocator.allocate(std::distance(begin, end))) Clause(begin, end, lbd);
        clauses.push_back(clause);

        certificate.added(clause->begin(), clause->end());

        if (clause->size() == 2) {
            binaryWatchers[toInt(~clause->first())].emplace_back(clause, clause->second());
            binaryWatchers[toInt(~clause->second())].emplace_back(clause, clause->first());
        }
        
        return clause;
    }

    inline void removeClause(Clause* clause) {
        allocator.deallocate(clause);

        certificate.removed(clause->begin(), clause->end());

        if (clause->size() == 2) {
            std::vector<BinaryWatcher>& list0 = binaryWatchers[toInt(~clause->first())];
            std::vector<BinaryWatcher>& list1 = binaryWatchers[toInt(~clause->second())];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list1.end());
        }
    }

    Clause* strengthenClause(Clause* clause, Lit lit) {
        assert(clause->size() > 1);
        std::vector<Lit> literals;
        for (Lit literal : *clause) if (literal != lit) literals.push_back(literal);
        Clause* new_clause = createClause(literals.begin(), literals.end(), std::min(clause->getLBD(), (uint16_t)(literals.size()-1)));
        removeClause(clause);
        return new_clause;
    }

    Clause* persistClause(Clause* clause) {
        Clause* new_clause = createClause(clause->begin(), clause->end(), 0);
        removeClause(clause);
        return new_clause;
    }

    inline const std::vector<BinaryWatcher>& getBinaryWatchers(Lit lit) {
        return binaryWatchers[toInt(lit)];
    }

    std::vector<Clause*> getUnitClauses() { 
        return allocator.collect_unit_clauses();
    }

    /**
     * only call this method at decision level 0
     **/
    void reduce() { 
        std::vector<Clause*> learnts;

        copy_if(clauses.begin(), clauses.end(), std::back_inserter(learnts), [this](Clause* clause) { 
            return clause->getLBD() > persistentLBD; 
        });

        std::sort(learnts.begin(), learnts.end(), [](Clause* c1, Clause* c2) { 
            return c1->getLBD() < c2->getLBD(); 
        });

        if (learnts.size() > 1) {
            auto begin = learnts.begin() + learnts.size()/2;

            if (keepMedianLBD) {
                unsigned int median_lbd = (*begin)->getLBD();
                while (begin != learnts.end() && (*begin)->getLBD() == median_lbd) {
                    begin++;
                }
            }

            for_each(begin, learnts.end(), [this] (Clause* clause) { 
                removeClause(clause); 
            });

            nReduced += std::distance(begin, learnts.end());
        }

        nReduceCalls++;
    }

    /**
     * Make sure all references are updated after all clauses reside in a new adress space
     */
    void reorganize() {
        allocator.synchronize();
        allocator.reorganize();
        clauses = allocator.collect();
        initBinaryWatchers();
    }

};

}

#endif