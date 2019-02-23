/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


--------------- Former Glucose Copyrights

 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).


--------------- Original Minisat Copyrights

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

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
*************************************************************************************************/

#ifndef _VARIABLE_ELIMINATION_H
#define _VARIABLE_ELIMINATION_H

#include <vector> 

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/Trail.h"
#include "candy/utils/Options.h"

namespace Candy {

class VariableElimination {
private:
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<uint32_t> elimclauses;
    std::vector<char> eliminated;
    std::vector<char> frozen;

    std::vector<Lit> resolvent;

    const bool active;          // Perform variable elimination.

    const unsigned int clause_lim;     // Variables are not eliminated if it produces a resolvent with a length above this limit. 0 means no limit.

public:
    unsigned int nEliminated;

    VariableElimination(ClauseDatabase& clause_db_, Trail& trail_) : 
        clause_db(clause_db_),
        trail(trail_),
        elimclauses(), 
        eliminated(),
        frozen(),
        resolvent(),
        active(VariableEliminationOptions::opt_use_elim),
        clause_lim(VariableEliminationOptions::opt_clause_lim), 
        nEliminated(0)
    { }

    unsigned int nTouched() {
        return nEliminated; 
    }

    inline void grow(size_t size) {
        if (size > frozen.size()) {
            frozen.resize(size);
            eliminated.resize(size);
        }
    }

    inline void lock(Var v) {
        frozen[v] = true;
    }

    inline bool isEliminated(Var v) const {
        return eliminated[v];
    }

    bool eliminate() {
        nEliminated = 0;

        if (!active) {
            return true;
        }

        std::vector<Var> variables;
        for (unsigned int v = 0; v < frozen.size(); v++) {
            if (!frozen[v] && !isEliminated(v)) variables.push_back(v);
        }

        std::sort(variables.begin(), variables.end(), [this](Var v1, Var v2) { 
            return this->clause_db.numOccurences(v1) > this->clause_db.numOccurences(v2);
        });

        for (Var variable : variables) {
            if (!trail.defines(mkLit(variable))) {
                std::vector<Clause*> pos, neg; // split the occurrences into positive and negative
                for (Clause* cl : clause_db.refOccurences(variable)) {
                    if (cl->contains(mkLit(variable))) {
                        pos.push_back(cl);
                    } else {
                        neg.push_back(cl);
                    }
                }

                if (!eliminate(variable, pos, neg)) { 
                    return false;
                }
            }
        }

        return true;
    }

    void extendModel(std::vector<lbool>& model) {
        Lit x;
        for (int i = elimclauses.size()-1, j; i > 0; i -= j) {
            for (j = elimclauses[i--]; j > 1; j--, i--) {
                Lit p = toLit(elimclauses[i]); 
                if ((model[var(p)] ^ sign(p)) != l_False)
                    goto next;
            }
            x = toLit(elimclauses[i]);
            model[var(x)] = lbool(!sign(x));
        next: ;
        }
    }

private:

    bool eliminate(Var variable, std::vector<Clause*> pos, std::vector<Clause*> neg) {
        assert(!isEliminated(variable));
        
        size_t nResolvents = 0;
        for (Clause* pc : pos) for (Clause* nc : neg) {
            size_t clause_size = 0;
            if (!merge(*pc, *nc, variable, clause_size)) {
                continue; // resolvent is tautology
            }
            if (++nResolvents > pos.size() + neg.size() || (clause_lim > 0 && clause_size > clause_lim)) {
                return true;
            } 
        }
        
        if (pos.size() > neg.size()) {
            for (Clause* c : neg) mkElimClause(elimclauses, variable, *c);
            mkElimClause(elimclauses, mkLit(variable));
        } else {
            for (Clause* c : pos) mkElimClause(elimclauses, variable, *c);
            mkElimClause(elimclauses, ~mkLit(variable));
        } 
        
        for (Clause* pc : pos) for (Clause* nc : neg) {
            if (merge(*pc, *nc, variable, resolvent)) {
                uint16_t lbd = std::min({ pc->getLBD(), nc->getLBD(), (uint16_t)(resolvent.size()-1) });
                Clause* new_clause = clause_db.createClause(resolvent.begin(), resolvent.end(), lbd);
                if (new_clause->size() == 0) {
                    return false;
                }
            }
        }

        for (const Clause* c : pos) clause_db.removeClause((Clause*)c);
        for (const Clause* c : neg) clause_db.removeClause((Clause*)c);

        eliminated[variable] = true;
        nEliminated++;

        return true;
    }

    void mkElimClause(std::vector<uint32_t>& elimclauses, Lit x) {
        elimclauses.push_back(toInt(x));
        elimclauses.push_back(1);
    }

    void mkElimClause(std::vector<uint32_t>& elimclauses, Var v, const Clause& c) { 
        assert(c.contains(v));
        uint32_t first = elimclauses.size();
        
        // Copy clause to elimclauses-vector
        for (Lit lit : c) {
            if (var(lit) != v || first == elimclauses.size()) {
                elimclauses.push_back(lit);
            } else {
                // Swap such that 'v' will occur first in the clause:
                elimclauses.push_back(elimclauses[first]);
                elimclauses[first] = lit;
            }
        }
        
        // Store the length of the clause last:
        elimclauses.push_back(c.size());
    }

    // Returns FALSE if clause is always satisfied ('out_clause' should not be used).
    bool merge(const Clause& _ps, const Clause& _qs, Var v, std::vector<Lit>& out_clause) {
        assert(_ps.contains(v));
        assert(_qs.contains(v));
        out_clause.clear();
        
        bool ps_smallest = _ps.size() < _qs.size();
        const Clause& ps = ps_smallest ? _qs : _ps;
        const Clause& qs = ps_smallest ? _ps : _qs;
        
        for (Lit qlit : qs) {
            if (var(qlit) != v) {
                auto p = std::find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return var(plit) == var(qlit); });
                if (p == ps.end()) {
                    out_clause.push_back(qlit);
                }
                else if (*p == ~qlit) {
                    return false;
                }
            }
        }
        
        for (Lit plit : ps) {
            if (var(plit) != v) {
                out_clause.push_back(plit);
            }
        }
        
        return true;
    }

    // Returns FALSE if clause is always satisfied.
    bool merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size) {
        assert(_ps.contains(v));
        assert(_qs.contains(v));
        
        bool ps_smallest = _ps.size() < _qs.size();
        const Clause& ps = ps_smallest ? _qs : _ps;
        const Clause& qs = ps_smallest ? _ps : _qs;
        
        size = ps.size() - 1;
        
        for (Lit qlit : qs) {
            if (var(qlit) != v) {
                auto p = std::find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return var(plit) == var(qlit); });
                if (p == ps.end()) {
                    size++;
                }
                else if (*p == ~qlit) {
                    return false;
                }
            }
        }
        
        return true;
    }

};

}

#endif