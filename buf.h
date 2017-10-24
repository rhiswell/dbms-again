//
// Created by Zhiqiang He on 08/11/2017.
//

#ifndef DBMS_AGAIN_BUF_H
#define DBMS_AGAIN_BUF_H

#define FRAMESIZE   4096
#define MAXFRAMES   1024

struct bFrame
{
    char field[FRAMESIZE];
};

extern bFrame buf[MAXFRAMES];

struct BCB
{
    BCB();
    int page_id;
    int frame_id;
    int latch;
    int count;
    int dirty;
    BCB *next;
};

// Buffer manager
class BMgr
{
public:
    BMgr();
    // Interface functions
    int FixPage(int page_id, int prot);
    void NewPage();
    int UnfixPage(int page_id);
    int NumFreeFrames();

private:
    int ftop[MAXFRAMES];
    BCB *ptof[MAXFRAMES]; // Hash table

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
