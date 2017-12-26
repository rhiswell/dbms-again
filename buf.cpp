//
// Created by Zhiqiang He on 08/11/2017.
//

#include "buf.h"

#include <list>
#include <iterator>
#include <utility>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bd.h"

using std::list;
using std::pair;
using std::make_pair;

bFrame buf[MAXFRAMES];  // 1024 * 4KB => 4MB
BCB bcb[MAXFRAMES];
extern DSMgr ds_mgr;


inline void reset_bcb(BCB *bcb)
{
    bcb->latch = bcb->count = bcb->dirty = bcb->isocc = 0;
    bcb->next = NULL;
}

BMgr::BMgr()
{
    lru.clear();
    memset(ptof, 0x0, MAXFRAMES * sizeof(BCB *));
}

int BMgr::Hash(int n)
{
    return n % MAXFRAMES;
}

int BMgr::FixPage(int page_id, int prot)
{
    int rv = -1;

    // Check if page is in buffer
    for (BCB *curr_bcb = ptof[Hash(page_id)];
         curr_bcb != NULL; curr_bcb = curr_bcb->next) {
        if (curr_bcb->page_id != page_id)
            continue;

        curr_bcb->count++;
        if (curr_bcb->latch == 0)
            curr_bcb->latch++;
        assert(curr_bcb->latch == 1);
        assert(curr_bcb->isocc == 1);

        // Move into MRU
        RemoveLRUEle(curr_bcb->frame_id);
        lru.push_back(curr_bcb->frame_id);

        rv = curr_bcb->frame_id;
        return rv;
    }

    // Load page and then fix it
    int frame_id = -1;
    if (NumFreeFrames() == 0) {
        // No free frames => Frame replacing using LRU
        frame_id = SelectVictim();
        assert(frame_id != -1);
    } else {
        // Find a free frame
        for (int i = 0; i < MAXFRAMES; i++) {
            if (bcb[i].isocc == 1)
                continue;
            frame_id = i;
            break;
        }
    }

    // Load page into buffer
    int rc = ds_mgr.ReadPage(page_id, &buf[frame_id]);
    assert(rc == 0);

    // Init the BCB
    BCB *curr_bcb = &bcb[frame_id];
    reset_bcb(curr_bcb);

    curr_bcb->isocc = 1;
    curr_bcb->page_id = page_id;
    curr_bcb->frame_id = frame_id;
    curr_bcb->count++;
    curr_bcb->latch++;

    // Insert the BCB into `ftop` hash table
    int htab_idx = Hash(page_id);
    BCB *bcb_ent = ptof[htab_idx];
    if (bcb_ent == NULL) {
        bcb_ent = curr_bcb;
    } else {
        curr_bcb->next = bcb_ent->next;
        bcb_ent = curr_bcb;
    }
    ptof[htab_idx] = bcb_ent;

    // Move into MRU
    RemoveLRUEle(frame_id);
    lru.push_back(frame_id);

    rv = frame_id;

out:
    return rv;
}

pair<int, int> BMgr::FixNewPage()
{
    // Allocate a new page and then fix it
    int page_id = ds_mgr.FindOnePage();
    int fram_id = FixPage(page_id, 0);
    return make_pair(page_id, fram_id);
}

int BMgr::UnfixPage(int page_id)
{
    BCB *curr_bcb = NULL;
    for (curr_bcb = ptof[Hash(page_id)]; curr_bcb != NULL;
         curr_bcb = curr_bcb->next) {
        if (curr_bcb->page_id == page_id)
            break;
    }
    assert(curr_bcb != NULL);

    curr_bcb->count--;
    if (curr_bcb->count == 0)
        curr_bcb->latch = 0;

    return curr_bcb->frame_id;
}

int BMgr::NumFreeFrames()
{
    int cnt = 0;
    for (int i = 0; i < MAXFRAMES; i++)
        if (bcb[i].isocc == 0)
            cnt++;
    return cnt;
}

void BMgr::TriggerWrite(int frame_id)
{
    SetDirty(frame_id);
}

void BMgr::TriggerRead(int frame_id)
{
    // Do nothing
}

int BMgr::SelectVictim()
{
    int rv = -1;

    for (list<int>::iterator it = lru.begin(); it != lru.end(); it++) {
        BCB *curr_bcb = &bcb[*it];
        if (curr_bcb->latch != 0)
            continue;
        // Found a victim
        if (curr_bcb->dirty == 1) {
            // #0: Flush page buffer into backing disk
            int rc = ds_mgr.WritePage(
                    curr_bcb->page_id, &buf[curr_bcb->frame_id]);
            if (rc == -1) {
                fprintf(stderr, "Error: failed to write page %d: frame %d\n",
                        curr_bcb->page_id, curr_bcb->frame_id);
                rv = -1;
                goto out;
            }
            UnsetDirty(curr_bcb->frame_id);
        }
        // #1: Remove BCB from `ptof` hash table
        RemoveBCB(curr_bcb, curr_bcb->page_id);
        rv = curr_bcb->frame_id;
        goto out;
    }

out:
    return rv;
}

void BMgr::RemoveBCB(BCB *ptr, int page_id)
{
    int htab_idx = Hash(page_id);
    for (BCB *curr_bcb = ptof[htab_idx]; curr_bcb != NULL;
         curr_bcb = curr_bcb->next) {
        if (curr_bcb != ptr)
            continue;
        curr_bcb = curr_bcb->next;
        break;
    }
}

void BMgr::RemoveLRUEle(int frid)
{
    lru.remove(frid);
}

void BMgr::SetDirty(int frame_id)
{
    bcb[frame_id].dirty = 1;
}

void BMgr::UnsetDirty(int frame_id)
{
    bcb[frame_id].dirty = 0;
}

// Called when DB shutdown
void BMgr::WriteDirtys()
{
    for (int i = 0; i < MAXFRAMES; i++) {
        BCB *curr_bcb = &bcb[i];
        if (!curr_bcb->isocc || !curr_bcb->dirty)
            continue;
        int rc = ds_mgr.WritePage(curr_bcb->page_id, &buf[curr_bcb->frame_id]);
        if (rc == -1)
            fprintf(stderr, "Error: failed to write page %d: frame %d\n",
                    curr_bcb->page_id, curr_bcb->frame_id);
        UnsetDirty(curr_bcb->frame_id);
    }
}
