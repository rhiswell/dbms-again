//
// Created by rh on 12/25/17.
//

#include "bd.h"
#include <assert.h>
#include <utility>

using std::pair;

DSMgr ds_mgr;

const char *test_buf_dbf= "test_buf.dbf";

void test_buf()
{
    int rc;

    // \begin test FixNewPage
    // Prepare backing file
    rc = ds_mgr.OpenFile(test_buf_dbf);
    assert(rc == 0);
    // Prepare buffer
    BMgr b_mgr;
    for (int i = 0; i < MAXFRAMES; i++) {
        pair<int, int> ret = b_mgr.FixNewPage();

        assert(ret.first == (i+1));
    }
    assert(b_mgr.NumFreeFrames() == 0);
    // Flush backing file
    rc = ds_mgr.CloseFile();
    assert(rc == 0);
    // \end test FixNewPage

    // \begin test
    rc = ds_mgr.OpenFile(test_buf_dbf);
    assert(rc == 0);
    // \end test FixNewPage
}