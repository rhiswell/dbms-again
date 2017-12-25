//
// Created by Zhiqiang He on 08/11/2017.
//

#ifndef DBMS_AGAIN_BUF_H
#define DBMS_AGAIN_BUF_H

#define MAXFRAMES   1024

#include <list>
#include <utility>
#include "bd.h"
#include "types.h"

using std::list;
using std::pair;

struct BCB
{
    int page_id;
    int frame_id;
    int latch;
    int count;
    int dirty;
    int isocc;
    BCB *next;
};

// Buffer manager
class BMgr
{
public:
    BMgr();
    // Interface functions
    int FixPage(int page_id, int prot);
    pair<int, int> FixNewPage();    // (page_id, frame_id)
    int UnfixPage(int page_id);
    int NumFreeFrames();

private:
    BCB *ptof[MAXFRAMES];   // Hash table
    list<int> lru;          // Front is LRU & back is MRU

    int SelectVictim();
    int Hash(int page_id);
    void RemoveBCB(BCB *ptr, int page_id);
    void RemoveLRUEle(int frid);
    void SetDirty(int frame_id);
    void UnsetDirty(int frame_id);
    void WriteDirtys();
    void PrintFrame(int frame_id);
};


#endif //DBMS_AGAIN_BUF_H
