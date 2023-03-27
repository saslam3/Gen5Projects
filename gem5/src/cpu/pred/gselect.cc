#include "cpu/pred/gselect.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"
// Constructor for the GSelectBP class
// Initializes the branch predictor with the specified parameters
GSelectBP::GSelectBP(const GSelectBPParams &params)
    : BPredUnit(params),
      globalHistoryReg(params.numThreads, 0),
      globalHistoryBits(ceilLog2(params.PredictorSize)),
      globalPredictorSize(params.PredictorSize),
      globalCtrBits(params.PHTCtrBits),
      counters(globalPredictorSize, SatCounter8(globalCtrBits))
{
    // Ensure that the global predictor size is a power of 2
    if (!isPowerOf2(globalPredictorSize))
        fatal("Invalid global history predictor size.\n");
    // Initialize masks and threshold for the branch predictor
    historyRegisterMask = mask(globalHistoryBits);
    globalHistoryMask = globalPredictorSize - 1;

    threshold = (ULL(1) << (globalCtrBits - 1)) - 1;
}
// Updates the global history register with "taken" and sets the final prediction to true
void
GSelectBP::uncondBranch(ThreadID tid, Addr pc, void * &bpHistory)
{
    // Create a new BPHistory object and set its fields
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->finalPred = true;
    bpHistory = static_cast<void*>(history);

    // Update the global history register with "taken"
    updateGlobalHistReg(tid, true);
}
// Squashes all outstanding updates until a given sequence number
// Restores the global history register to its previous state and deletes the history object
void
GSelectBP::squash(ThreadID tid, void *bpHistory)
{
    BPHistory *history = static_cast<BPHistory*>(bpHistory);
    globalHistoryReg[tid] = history->globalHistoryReg;
    delete history;
}

// Looks up the appropriate counter in the pattern history table (PHT) and makes a prediction
// Updates the global history register based on the prediction
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

// Updates the global history register to ensure a "not taken" prediction when there is a BTB miss
void
GSelectBP::btbUpdate(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    globalHistoryReg[tid] &= (historyRegisterMask & ~ULL(1));
}

// Updates the branch predictor based on the actual outcome of the branch
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

// Updates the global history register based on the outcome of a branch
void
GSelectBP::updateGlobalHistReg(ThreadID tid, bool taken)
{
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1 :
                               (globalHistoryReg[tid] << 1);
    globalHistoryReg[tid] &= historyRegisterMask;
}