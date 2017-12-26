//
// Created by rh on 12/25/17.
//

#include "bd.h"
#include <assert.h>
#include <utility>

using std::pair;

// DSMgr ds_mgr;
extern DSMgr ds_mgr;

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
    // \begin test FixPage
    // This will increase reference counter
    assert(b_mgr.NumFreeFrames() == 0);
    for (int i = 1; i < MAXFRAMES+1; i++) {
        rc = (b_mgr.FixPage(i, 0));
    }
    // \end test FixPage
    // Buffer is full => assert(SelectVictim() == 0) failure
    // So do UnFixPage firstly will eliminate this error
    // \begin test UnFixPage
    int frame_id;
    frame_id = b_mgr.UnfixPage(1);
    frame_id = b_mgr.UnfixPage(1);
    pair<int, int> ret = b_mgr.FixNewPage();
    assert(ret.second == frame_id);
    // \end test UnFixPage
    // Flush backing file
    rc = ds_mgr.CloseFile();
    assert(rc == 0);
    // \end test FixNewPage
}