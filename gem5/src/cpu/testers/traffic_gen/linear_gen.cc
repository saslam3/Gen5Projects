/*
 * Copyright (c) 2012-2013, 2016-2017 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed here under.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cpu/testers/traffic_gen/linear_gen.hh"

#include <algorithm>

#include "base/random.hh"
#include "base/trace.hh"
#include "debug/TrafficGen.hh"

void
LinearGen::enter()
{
    // reset the address and the data counter
    nextAddr = startAddr;
    dataManipulated = 0;
}

PacketPtr
LinearGen::getNextPacket()
{
    // choose if we generate a read or a write here
    bool isRead = readPercent != 0 &&
        (readPercent == 100 || random_mt.random(0, 100) < readPercent);

    assert((readPercent == 0 && !isRead) || (readPercent == 100 && isRead) ||
           readPercent != 100);

    DPRINTF(TrafficGen, "LinearGen::getNextPacket: %c to addr %x, size %d\n",
            isRead ? 'r' : 'w', nextAddr, blocksize);

    // Add the amount of data manipulated to the total
    dataManipulated += blocksize;

    PacketPtr pkt = getPacket(nextAddr, blocksize,
                              isRead ? MemCmd::ReadReq : MemCmd::WriteReq);

    // increment the address
    nextAddr += blocksize;

    // If we have reached the end of the address space, reset the
    // address to the start of the range
    if (nextAddr > endAddr) {
        DPRINTF(TrafficGen, "Wrapping address to the start of "
                "the range\n");
        nextAddr = startAddr;
    }

    return pkt;
}

Tick
LinearGen::nextPacketTick(bool elastic, Tick delay) const
{
    // Check to see if we have reached the data limit. If dataLimit is
    // zero we do not have a data limit and therefore we will keep
    // generating requests for the entire residency in this state.
    if (dataLimit && dataManipulated >= dataLimit) {
        DPRINTF(TrafficGen, "Data limit for LinearGen reached.\n");
        // there are no more requests, therefore return MaxTick
        return MaxTick;
    } else {
        // return the time when the next request should take place
        Tick wait = random_mt.random(minPeriod, maxPeriod);

        // compensate for the delay experienced to not be elastic, by
        // default the value we generate is from the time we are
        // asked, so the elasticity happens automatically
        if (!elastic) {
            if (wait < delay)
                wait = 0;
            else
                wait -= delay;
        }

        return curTick() + wait;
    }
}
