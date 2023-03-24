#include "cpu/pred/gselect_pred.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"

GSelectBP::GSelectBP(const LocalBPParams &params)
    : BPredUnit(params),
      PredictorSize(params.localPredictorSize),
      PHTCtrBits(params.localCtrBits),
      globalHistoryBits(params.globalHistoryBits),
      localCtrs(params.localPredictorSize),
      indexMask(PredictorSize - 1),
      ghr(0)
{
    for (auto &ctr : localCtrs)
        ctr.setBits(PHTCtrBits);
}

inline bool
GSelectBP::getPrediction(uint8_t &count)
{
    return count >= (1 << (PHTCtrBits - 1));
}

inline unsigned
GSelectBP::getGSelectIndex(Addr &PC)
{
    unsigned pcIndex = PC >> instShiftAmt;
    unsigned ghrIndex = ghr & ((1 << globalHistoryBits) - 1);
    return (pcIndex ^ ghrIndex) & indexMask;
}

void
GSelectBP::uncondBranch(ThreadID tid, Addr pc, void *&bp_history)
{
    bp_history = nullptr;
}

bool
GSelectBP::lookup(ThreadID tid, Addr branch_addr, void *&bp_history)
{
    unsigned index = getGSelectIndex(branch_addr);
    SatCounter8 &ctr = localCtrs[index];
    bool taken = getPrediction(ctr);

    bp_history = nullptr;
    return taken;
}

void
GSelectBP::btbUpdate(ThreadID tid, Addr branch_addr, void *&bp_history)
{
    unsigned index = getGSelectIndex(branch_addr);
    SatCounter8 &ctr = localCtrs[index];
    ctr.reset();

    bp_history = nullptr;
}

void
GSelectBP::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                  bool squashed, const StaticInstPtr &inst, Addr corrTarget)
{
    unsigned index = getGSelectIndex(branch_addr);
    SatCounter8 &ctr = localCtrs[index];

    if (taken) {
        ctr.increment();
    } else {
        ctr.decrement();
    }

    // Update the global history register
    ghr = (ghr << 1) | (taken ? 1 : 0);
}

void
GSelectBP::squash(ThreadID tid, void *bp_history)
{
    // No state to restore or actions to perform
}
