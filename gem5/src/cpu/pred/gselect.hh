#ifndef __CPU_PRED_GSELECT_PRED_HH__
#define __CPU_PRED_GSELECT_PRED_HH__

#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/LocalBP.hh"

class GSelectBP : public BPredUnit
{
  public:
    GSelectBP(const LocalBPParams &params);

    virtual void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);

    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);

    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);

    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget);

    void squash(ThreadID tid, void *bp_history)
    { assert(bp_history == NULL); }

  private:
    inline bool getPrediction(uint8_t &count);

    /** Calculates the Gselect index based on the PC and GHR. */
    inline unsigned getGSelectIndex(Addr &PC);

    // Updated variable names
    const unsigned PredictorSize;
    const unsigned PHTCtrBits;
    const unsigned globalHistoryBits;

    std::vector<SatCounter8> localCtrs;

    const unsigned indexMask;

    // Additional data member for GHR
    uint32_t ghr;
};

#endif // __CPU_PRED_GSELECT_PRED_HH__
