//
// Created by Zhiqiang He on 08/11/2017.
//

#include "bd.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <assert.h>
#include <string>

using namespace std;


void DSMgr::InitGroup(int group_id)
{
    // Increase 8 pages in the file
    Seek(GroupId2Pos(group_id) + (MAXBUNCH << 12) - 1, SEEK_SET);
    fprintf(currFile, "\0");

    // Reset relative bitmap
    Seek(GroupId2Pos(group_id), SEEK_SET);
    fprintf(currFile, "\0");
}

// The backing file is organized in pages that each is 4KB.
// Many pages form a group that has the following layout:
// |----------------------|-----------|-----------|----
// |--page #0 for bitmap--|--page #1--|--page #2--|...
// |----------------------|-----------|-----------|----
// A 4K page can take 32768bits, then a group has 32768 pages.
// So, we can get capacity up to 128MB a group.
int DSMgr::OpenFile(string filename)
{
    // First open to check if the file exists
    currFile = fopen(filename.c_str(), "r");
    if (currFile == NULL) {
        // The file doesn't exist => create a file & init 1st page (partly) for bitmap

        if (currFile == NULL)
            return -1;
        InitGroup(0);
    }
    // Stat the file to init numPages
    struct stat sb;
    int rc = stat(filename.c_str(), &sb);
    if (rc != 0)
        return -1;
    numPages = (sb.st_size >> 12);  // <=> sb.st_size / 4K
    assert(numPages == MAXBUNCH);

    // Load bitmaps from each group
    int nr_groups = GetNRGroups();  // TODO
    Seek(0, SEEK_SET);
    fread(pages, sizeof(char), HEADROOM, currFile);
    numPages = 0;
    for (int i = 0; i < HEADROOM; i++) {
        if (pages[i] & 0x00000001) numPages++;
        if (pages[i] & 0x00000010) numPages++;
        if (pages[i] & 0x00000100) numPages++;
        if (pages[i] & 0x00001000) numPages++;
        if (pages[i] & 0x00010000) numPages++;
        if (pages[i] & 0x00100000) numPages++;
        if (pages[i] & 0x01000000) numPages++;
        if (pages[i] & 0x10000000) numPages++;
    }

    return 0;
}

int DSMgr::CloseFile()
{
    if (currFile != NULL)
        fclose(currFile);
    return 0;
}

int DSMgr::ReadPage(int page_id, bFrame *frm)
{
    if (frm == NULL || Seek(PageId2Pos(page_id), SEEK_SET) == -1)
        return 0;
    return fread(frm, sizeof(*frm), 1, GetFile());
}

int DSMgr::WritePage(int page_id, bFrame *frm)
{
    if (frm == NULL || Seek(PageId2Pos(page_id), SEEK_SET) == -1)
        return 0;
    return fwrite(frm, sizeof(*frm), 1, GetFile());
}

int DSMgr::Seek(int offset, int pos)
{
    // TODO: check offset & pos
    return fseek(GetFile(), offset, pos);
}

FILE * DSMgr::GetFile()
{
    return currFile;
}

void DSMgr::IncNumPages()
{
    // Do nothing
}

int DSMgr::GetNumPages()
{
    return numPages;
}

void DSMgr::SetUse(int page_id, int use_bit)
{
    int idx = page_id / 8;
    int off = page_id % 8;
    if (use_bit == 1) {
        pages[idx] |= (1 << off);
    } else if (use_bit == 0) {
        pages[idx] &= ~(1 << off);
    } else {
        // `use_bit` is out of range, then do nothing
    }
}

int DSMgr::GetUse(int page_id)
{
    int idx = page_id / 8;
    int off = page_id % 8;
    return pages[idx] & (1 << off);
}
