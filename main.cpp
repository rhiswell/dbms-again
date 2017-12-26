//
// Created by Zhiqiang He on 08/11/2017.
//

#include <stdio.h>
#include <assert.h>
#include "buf.h"
#include "bd.h"

//extern void test_bd();
//extern void test_buf();

// Non-thread safety
// Trace test only
extern int rw_counter_r;
extern int rw_counter_w;

const char *data_dbf = "./data.dbf";
DSMgr ds_mgr;

int main()
{
    //test_bd();
    //test_buf();
    int rc;

    // Prepare backing file
    rc = ds_mgr.OpenFile(data_dbf);
    assert(rc == 0);

    // Prepare buffer
    BMgr b_mgr;

    // Materialize pages
    for (int i = 0; i < 50000; i++)
        ds_mgr.FindOnePage();
    printf("Info: %d pages were allocated.\n", ds_mgr.GetNumPages());
    assert(ds_mgr.GetNumPages() == (50000+8));

    // \begin test
    FILE *fin = fopen("./data-5w-50w-zipf.txt", "r");
    assert(fin != NULL);
    int rw, page_id, fram_id;
    for (int i = 0; i < 500000; i++) {
        fscanf(fin, "%d,%d", &rw, &page_id);
        if (page_id == 0 || page_id == 32768)
            continue;
        if (rw == 0) {
            // Read
            fram_id = b_mgr.FixPage(page_id, 0);
            // Do data-reading
            b_mgr.TriggerRead(fram_id);
            b_mgr.UnfixPage(page_id);
        } else if (rw == 1){
            // Write
            fram_id = b_mgr.FixPage(page_id, 0);
            // Do data-writing
            b_mgr.TriggerWrite(fram_id);
            b_mgr.UnfixPage(page_id);
        }
    }
    // \end test

    // To flush in-buffer data
    rc = ds_mgr.CloseFile();
    assert(rc == 0);

    // \begin print statistics
    int rw_counter = rw_counter_r + rw_counter_w;
    printf("Info: 500000 test traces => %d IO\n", rw_counter);
    printf("Info: R: %d, W: %d\n", rw_counter_r, rw_counter_w);
    // \end print statistics

    return 0;
}
