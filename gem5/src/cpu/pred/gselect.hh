#ifndef __CPU_PRED_GSELECT_PRED_HH__
#define __CPU_PRED_GSELECT_PRED_HH__

#include "base/sat_counter.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/GSelectBP.hh"
// The GSelectBP class implements a gselect branch predictor.
// The gselect predictor uses a global history register and a pattern history table (PHT)
class GSelectBP : public BPredUnit
{
  public:
    GSelectBP(const GSelectBPParams &params);
    void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
    void squash(ThreadID tid, void *bp_history);
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget);

  private:
    void updateGlobalHistReg(ThreadID tid, bool taken);

    struct BPHistory {
        unsigned globalHistoryReg;
        // the final taken/not-taken prediction
        // true: predict taken
        // false: predict not-taken
        bool finalPred;
    };

    std::vector<unsigned> globalHistoryReg;
    unsigned globalHistoryBits;
    unsigned historyRegisterMask;

    unsigned globalPredictorSize;
    unsigned globalCtrBits;
    unsigned globalHistoryMask;

    std::vector<SatCounter8> counters;
    unsigned threshold;
};

#endif // __CPU_PRED_GSELECT_PRED_HH__