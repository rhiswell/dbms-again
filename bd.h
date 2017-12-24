//
// Created by Zhiqiang He on 08/11/2017.
//

#ifndef DBMS_AGAIN_BD_H
#define DBMS_AGAIN_BD_H

#include "types.h"
#include "buf.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#define PAGESIZE   4096     // In bytes
#define MAXPAGES   65536    // Two groups => <= 256 MB backing storage
#define MAXBUNCH   8        // Increase 8 pages in each `IncNumPages` call
#define MAXGROUP   2


static inline int PageId2GroupId(int page_id)
{
    int group_id = (page_id >> 15); // <=> page_id / (4096 * 8) pages per group
    assert(group_id >= 0);

    return group_id;
}

static inline int GroupId2Pos(int group_id)
{
    return group_id << 27;          // <=> group_id * (4096 * 8) * 4K
}

static inline int PageId2Pos(int page_id)
{
    return PAGESIZE * page_id;
}

// Data storage manager
class DSMgr
{
public:
    DSMgr();
    int OpenFile(const char *pathname);
    int CloseFile();
    int ReadPage(int page_id, bFrame *frm);
    int WritePage(int page_id, bFrame *frm);
    int Seek(int offset, int pos);
    FILE *GetFile();
    void IncNumPages();
    int GetNumPages();
    int FindOnePage();
    void SetUse(int page_id, int use_bit);
    int GetUse(int page_id);

private:
    FILE *currFile;             // Backing storage
    int numPages;               // Number of allocated pages
    char bitmap[MAXPAGES >> 3]; // The bitmaps of each group will be loaded into
                                // memory in time. 2 slots => 2 groups on the fly.

    int InitGroup(int group_id);
    int GetNRGroups();
};

#endif //DBMS_AGAIN_BD_H
