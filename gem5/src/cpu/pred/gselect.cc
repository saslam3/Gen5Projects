#include "cpu/pred/gselect.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"

GSelectBP::GSelectBP(const GSelectBPParams &params)
    : BPredUnit(params),
      globalHistoryReg(params.numThreads, 0),
      globalHistoryBits(ceilLog2(params.PredictorSize)),
      globalPredictorSize(params.PredictorSize),
      globalCtrBits(params.PHTCtrBits),
      counters(globalPredictorSize, SatCounter8(globalCtrBits))
{
    if (!isPowerOf2(globalPredictorSize))
        fatal("Invalid global history predictor size.\n");

    historyRegisterMask = mask(globalHistoryBits);
    globalHistoryMask = globalPredictorSize - 1;

    threshold = (ULL(1) << (globalCtrBits - 1)) - 1;
}

void
GSelectBP::uncondBranch(ThreadID tid, Addr pc, void * &bpHistory)
{
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->finalPred = true;
    bpHistory = static_cast<void*>(history);
    updateGlobalHistReg(tid, true);
}

void
GSelectBP::squash(ThreadID tid, void *bpHistory)
{
    BPHistory *history = static_cast<BPHistory*>(bpHistory);
    globalHistoryReg[tid] = history->globalHistoryReg;

    delete history;
}

bool
GSelectBP::lookup(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    unsigned globalHistoryIdx = (((branchAddr >> instShiftAmt)
                                ^ (globalHistoryReg[tid] << globalHistoryBits))
                                & globalHistoryMask);

    assert(globalHistoryIdx < globalPredictorSize);

    bool finalPrediction = counters[globalHistoryIdx] > threshold;

    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->finalPred = finalPrediction;
    bpHistory = static_cast<void*>(history);
    updateGlobalHistReg(tid, finalPrediction);

    return finalPrediction;
}

void
GSelectBP::btbUpdate(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    globalHistoryReg[tid] &= (historyRegisterMask & ~ULL(1));
}

void
GSelectBP::update(ThreadID tid, Addr branchAddr, bool taken, void *bpHistory,
                 bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
    assert(bpHistory);

    BPHistory *history = static_cast<BPHistory*>(bpHistory);

    if (squashed) {
        globalHistoryReg[tid] = (history->globalHistoryReg << 1) | taken;
        return;
    }

    unsigned globalHistoryIdx = (((branchAddr >> instShiftAmt)
                                ^ (history->globalHistoryReg << globalHistoryBits))
                                & globalHistoryMask);

    assert(globalHistoryIdx < globalPredictorSize);

    if (taken) {
        counters[globalHistoryIdx]++;
    } else {
        counters[globalHistoryIdx]--;
    }

    delete history;
}

void
GSelectBP::updateGlobalHistReg(ThreadID tid, bool taken)
{
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1 :
                               (globalHistoryReg[tid] << 1);
    globalHistoryReg[tid] &= historyRegisterMask;
}