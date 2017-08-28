/*
 * Branch.h
 *
 *  Created on: 25.08.2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_BRANCH_H_
#define SRC_CANDY_CORE_BRANCH_H_

#include <vector>

#include "candy/core/Trail.h"

namespace Candy {

class Branch {
	struct VarOrderLt {
		const vector<double>& activity;
		bool operator()(Var x, Var y) const {
			return activity[x] > activity[y];
		}
		VarOrderLt(const vector<double>& act) : activity(act) {}
	};

public:
	Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
	std::vector<double> activity; // A heuristic measurement of the activity of a variable.
	double var_inc; // Amount to bump next variable with.
	double var_decay;
	double max_var_decay;
	std::vector<char> polarity; // The preferred polarity of each variable.
	std::vector<char> decision; // Declares if a variable is eligible for selection in the decision heuristic

	Branch(double vd, double mvd) :
		polarity(), decision(),
		order_heap(VarOrderLt(activity)),
		activity(),
		var_inc(1), var_decay(vd), max_var_decay(mvd) {

	}

	inline void grow(bool dvar, bool sign, double act) {
		decision.push_back(dvar);
		polarity.push_back(sign);
		activity.push_back(act);
		insertVarOrder(decision.size() - 1);
	}

	inline void grow(size_t size, bool dvar, bool sign, double act) {
		int prevSize = decision.size(); // can be negative during initialization
		if (size > decision.size()) {
			decision.resize(size, dvar);
			polarity.resize(size, sign);
			activity.resize(size, act);
			for (int i = prevSize; i < size; i++) {
				insertVarOrder(i);
				Statistics::getInstance().solverDecisionVariablesInc();
			}
		}
	}

	inline void initFrom(const CNFProblem& problem) {
		std::vector<double> occ = problem.getLiteralRelativeOccurrences();
		for (size_t i = 0; i < decision.size(); i++) {
			activity[i] = occ[mkLit(i, true)] + occ[mkLit(i, false)];
			polarity[i] = occ[mkLit(i, true)] > occ[mkLit(i, false)];
		}
	}

	// Declare if a variable should be eligible for selection in the decision heuristic.
	inline void setDecisionVar(Var v, bool b) {
		if (decision[v] != static_cast<char>(b)) {
			decision[v] = b;
			if (b) {
				insertVarOrder(v);
				Statistics::getInstance().solverDecisionVariablesInc();
			} else {
				Statistics::getInstance().solverDecisionVariablesDec();
			}
		}
	}
	// Insert a variable in the decision order priority queue.
	inline void insertVarOrder(Var x) {
		if (!order_heap.inHeap(x) && decision[x])
			order_heap.insert(x);
	}

	// Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
	inline void varDecayActivity() {
		var_inc *= (1 / var_decay);
	}

	// Increase a variable with the current 'bump' value.
	inline void varBumpActivity(Var v) {
		varBumpActivity(v, var_inc);
	}

	inline void varBumpActivity(Var v, double inc) {
		if ((activity[v] += inc) > 1e100) {
			varRescaleActivity();
		}
		if (order_heap.inHeap(v)) {
			order_heap.decrease(v); // update order-heap
		}
	}

	void varRescaleActivity() {
		for (size_t i = 0; i < activity.size(); i++) {
			activity[i] *= 1e-100;
		}
		var_inc *= 1e-100;
	}

	void rebuildOrderHeap(Trail& trail) {
		vector<Var> vs;
		for (size_t v = 0; v < decision.size(); v++) {
			if (decision[v] && trail.value(checked_unsignedtosigned_cast<size_t, Var>(v)) == l_Undef) {
				vs.push_back(checked_unsignedtosigned_cast<size_t, Var>(v));
			}
		}
		order_heap.build(vs);
	}

	inline Lit pickBranchLit(Trail& trail) {
		Var next = var_Undef;

		// Activity based decision:
		while (next == var_Undef || trail.value(next) != l_Undef || !decision[next]) {
			if (order_heap.empty()) {
				next = var_Undef;
				break;
			} else {
				next = order_heap.removeMin();
			}
		}

		return next == var_Undef ? lit_Undef : mkLit(next, polarity[next]);
	}
};

}
#endif /* SRC_CANDY_CORE_BRANCH_H_ */
