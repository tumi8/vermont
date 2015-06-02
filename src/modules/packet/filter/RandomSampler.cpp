/*
 * Vermont Packet Filter
 * Copyright (C) 2009  Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/*
 * PSAMP Reference Implementation
 *
 * RandomSampler.cpp
 *
 * Random n-out-of-N sampling of packets
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#include <cstdlib>
#include <ctime>

#include "RandomSampler.h"

RandomSampler::RandomSampler(int n, int N) : samplingSize(N), acceptSize(n), currentPos(0)
{
        int pos;

        if(n > N) {
                int tmp;

                msg(MSG_ERROR, "%d out-of %d makes no sense - exchanging values", n, N);

                tmp=n;
                n=N;
                N=tmp;
        }

        sampleMask.clear();
        sampleMask.insert(sampleMask.begin(), N, false);

        srand(time(0));

        /*
         setup sampling bitfield
         TODO: There might be a more elegant solution to this...
         */
        for(int i = 0; i < acceptSize; i++) {
                /*
                 find a free spot in the sampleMask (i.e. a position
                 with FALSE in it) and set it to TRUE
                 */
                do {
                        pos = rand() % samplingSize;
                } while(sampleMask[pos]);

                sampleMask[pos] = true;
        }
}

bool RandomSampler::processPacket(Packet *p)
{
        bool accepted = sampleMask[currentPos];

        currentPos = (currentPos + 1) % samplingSize;

        return accepted;
}
