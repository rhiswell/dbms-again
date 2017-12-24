//
// Created by rh on 12/24/17.
//

#include "bd.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>


const char *test_bd_dbf = "./test_bd.dbf";
bFrame demo_buf = {.field = {'x'}};
bFrame temp_buf;

void test_bd()
{
    int rc, nr_pages, fsz;
    FILE *dbf;

    DSMgr ds_mgr;

    // \begin test InitGroup(0)
    rc = ds_mgr.OpenFile(test_bd_dbf);
    assert(rc == 0);
    nr_pages = ds_mgr.GetNumPages();
    assert(nr_pages == MAXBUNCH);
    // Test bitmap of first bunch of pages
    assert(ds_mgr.GetUse(0) == 1);
    for (int i = 1; i < MAXBUNCH; ++i)
        assert(ds_mgr.GetUse(i) == 0);
    assert((dbf = ds_mgr.GetFile()) != NULL);
    assert(ds_mgr.Seek(0, SEEK_END) == 0);
    assert(ftell(dbf) == (1 << 15));
    // Test FindOnePage
    assert(ds_mgr.FindOnePage() == 1);
    rc = ds_mgr.CloseFile();
    assert(rc == 0);
    // \end test InitGroup(0)

    printf("DSMgr::InitGroup was verified.\n");

    // \begin test allocate new pages
    rc = ds_mgr.OpenFile(test_bd_dbf);
    assert(rc == 0);
    ds_mgr.IncNumPages();
    nr_pages = ds_mgr.GetNumPages();
    assert(nr_pages == 2*MAXBUNCH);
    // Check file size
    assert((dbf = ds_mgr.GetFile()) != NULL);
    assert(ds_mgr.Seek(0, SEEK_END) == 0);
    assert(ftell(dbf) == (1 << 16));
    assert(ds_mgr.GetUse(0) == 1);
    for (int i = 1; i < 2*MAXBUNCH; i++)
        assert(ds_mgr.GetUse(i) == 0);
    rc = ds_mgr.CloseFile();
    assert(rc == 0);
    // \end test allocate new pages

    printf("DSMgr::IncNumPages() was verified.\n");

    // \begin test read/write page
    rc = ds_mgr.OpenFile(test_bd_dbf);
    assert(rc == 0);
    // Get filesize
    assert((dbf = ds_mgr.GetFile()) != NULL);
    assert(ds_mgr.Seek(0, SEEK_END) == 0);
    assert(ftell(dbf) == (1 << 16));
    int ex_page_id = 8;
    ds_mgr.SetUse(ex_page_id, 1);
    assert(ds_mgr.WritePage(ex_page_id, &demo_buf) == 0);
    assert((ds_mgr.GetUse(ex_page_id) == 1) && ds_mgr.ReadPage(ex_page_id, &temp_buf) == 0);
    for (int i = 0; i < PAGESIZE; i++)
        assert(temp_buf.field[i] == demo_buf.field[i]);
    rc = ds_mgr.CloseFile();
    assert(rc == 0);
    // \end test read/write page

    printf("DSMgr::Write/ReadPage() was verified.\n");
}
