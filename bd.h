//
// Created by Zhiqiang He on 08/11/2017.
//

#ifndef DBMS_AGAIN_BD_H
#define DBMS_AGAIN_BD_H

#include "buf.h"

#include <stdio.h>
#include <assert.h>
#include <string>

using namespace std;

#define PAGESIZE   4096     // In bytes
#define MAXPAGES   65536    // Two groups => <= 256 MB backing storage
#define MAXBUNCH   8        // Increase 8 pages in each `IncNumPages` call


// TODO
static inline int GetNRGroups(int page_id)
{
    int nr_groups = (page_id >> 15); // <=> page_id / (4096 * 8)
    assert(nr_groups >= 0);

    return nr_groups;
}

static inline int GroupId2Pos(int group_id)
{
    return group_id << 27;
}

static inline int PageId2Pos(int page_id)
{
    return PAGESIZE * (page_id + GetNRGroups(page_id));
}

// Data storage manager
class DSMgr
{
public:
    DSMgr();
    int OpenFile(string filename);
    int CloseFile();
    int ReadPage(int page_id, bFrame *frm);
    int WritePage(int page_id, bFrame *frm);
    int Seek(int offset, int pos);
    FILE *GetFile();
    void IncNumPages();
    int GetNumPages();
    void SetUse(int page_id, int use_bit);
    int GetUse(int page_id);

private:
    FILE *currFile;          // Backing storage
    int numPages;            // Number of allocated pages
    char bitmap[MAXPAGES >> 3];

    void InitGroup(int group_id);
};


#endif //DBMS_AGAIN_BD_H
