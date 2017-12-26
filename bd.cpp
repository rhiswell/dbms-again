//
// Created by Zhiqiang He on 08/11/2017.
//

#include "bd.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>


// Non-thread safety
int rw_counter_r = 0;
int rw_counter_w = 0;

DSMgr::DSMgr() {}

int DSMgr::InitGroup(int group_id)
{
    int rc;
    // Increase 8 pages for this group in the file.
    rc = Seek(GroupId2Pos(group_id) + (MAXBUNCH << 12) - 1, SEEK_SET);
    if (rc == -1)
        return -1;
    fprintf(currFile, "%c", 0);

    // Reset relative bitmap
    rc = Seek(GroupId2Pos(group_id), SEEK_SET);
    if (rc == -1)
        return -1;
    fprintf(currFile, "%c", 0x01); // 00000001

    return 0;
}

int DSMgr::GetNRGroups()
{
    return (numPages >> 15) + 1;    // <=> numPages / (4096 * 8) pages per group
}

// The backing file is organized in pages that each is 4KB.
// Many pages form a group that has the following layout:
// |----------------------|-----------|-----------|----
// |--page #0 for bitmap--|--page #1--|--page #2--|...
// |----------------------|-----------|-----------|----
// A 4K page can take 32768 bits, then a group has 32768 pages.
// So, we can get capacity up to 128MB a group.
int DSMgr::OpenFile(const char *pathname)
{
    // First open to check if the file exists
    currFile = fopen(pathname, "r+");
    if (currFile == NULL) {
        // The file doesn't exist => create a file & init 1st page (partly) for bitmap
        currFile = fopen(pathname, "w+");
        if (currFile == NULL || InitGroup(0) == -1)
            return -1;
    }

    // Stat the file to init numPages
    struct stat sb;
    int rc = stat(pathname, &sb);
    if (rc != 0)
        return -1;
    numPages = (sb.st_size >> 12);  // <=> sb.st_size / 4K
    assert((numPages % MAXBUNCH == 0) && (numPages >= MAXBUNCH));

    // Load bitmaps from each allocated group
    int nr_groups = GetNRGroups();
    assert(nr_groups >= 0 && nr_groups <= MAXGROUP);
    for (int gi = 0; gi < nr_groups; gi++) {
        if (Seek(GroupId2Pos(gi), SEEK_SET) == -1)
            return -1;
        fread(bitmap + (gi << 12), sizeof(char), PAGESIZE, currFile);
    }

    return 0;
}

int DSMgr::CloseFile()
{
    // Flush in-memory meta data (e.g. bitmap) into file buffer
    for (int gi = 0; gi < GetNRGroups(); gi++) {
        if (Seek(GroupId2Pos(gi), SEEK_SET) == -1)
            return -1;
        fwrite(bitmap + (gi << 12), sizeof(char), PAGESIZE, currFile);
    }

    fclose(currFile);
    return 0;
}

int DSMgr::ReadPage(int page_id, bFrame *frm)
{
    if (frm == NULL || Seek(PageId2Pos(page_id), SEEK_SET) == -1)
        return -1;
    int nread = fread(frm->field, 1, sizeof(*frm), currFile);
    rw_counter_r++;
    return (nread == PAGESIZE) ? 0 : -1;
}

int DSMgr::WritePage(int page_id, bFrame *frm)
{
    if (frm == NULL || Seek(PageId2Pos(page_id), SEEK_SET) == -1)
        return -1;
    int nwrite = fwrite(frm->field, 1, sizeof(*frm), currFile);
    rw_counter_w++;
    return (nwrite == PAGESIZE) ? 0 : -1;
}

int DSMgr::Seek(int offset, int pos)
{
    return fseek(currFile, offset, pos);
}

FILE * DSMgr::GetFile()
{
    return currFile;
}

void DSMgr::IncNumPages()
{
    // Allocate pages in bunch
    int new_nr_pages = numPages + MAXBUNCH;
    assert(new_nr_pages <= MAXPAGES);

    if (new_nr_pages % (PAGESIZE << 3) == MAXBUNCH) {
        // Routine to create a new group
        int group_id = PageId2GroupId(new_nr_pages-1);
        InitGroup(group_id);
        // Load bitmap into memory
        Seek(GroupId2Pos(group_id), SEEK_SET);
        fread(bitmap + (group_id << 12), sizeof(char), PAGESIZE, currFile);
    } else {
        // Create bunch of pages in a group
        Seek(PageId2Pos(new_nr_pages)-1, SEEK_SET);
        fprintf(currFile, "%c", 0);
        int bm_idx = numPages >> 3; // Bitmap idx of next bunch of pages
        memset(&bitmap[bm_idx], 0, MAXBUNCH);
    }

    numPages = new_nr_pages;
}

int DSMgr::GetNumPages()
{
    return numPages;
}

int DSMgr::FindOnePage()
{
    int bm_idx, page_id;
    for (bm_idx = 0, page_id = 0; page_id < numPages;
         bm_idx++, page_id += 8) {
        if ((bitmap[bm_idx] & 0xff) == 0xff)
            continue;
        for (int off = 0; off < 8; off++) {
            if ((bitmap[bm_idx] & (1 << off)) == 0) {
                page_id += off;
                SetUse(page_id, 1);
                goto out;
            }
        }
    }

    // Allocate a bunch of new pages
    IncNumPages();
    // Do find again
    FindOnePage();

out:
    return page_id;
}

void DSMgr::SetUse(int page_id, int use_bit)
{
    int idx = page_id / 8;
    int off = page_id % 8;
    if (use_bit == 1) {
        bitmap[idx] |= (1 << off);
    } else if (use_bit == 0) {
        bitmap[idx] &= ~(1 << off);
    } else {
        // `use_bit` is out of range, then do nothing
    }
}

int DSMgr::GetUse(int page_id)
{
    int idx = page_id / 8;
    int off = page_id % 8;
    return bitmap[idx] & (1 << off);
}
