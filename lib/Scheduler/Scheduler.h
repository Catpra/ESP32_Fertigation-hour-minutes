// create scheduler task
#pragma once
#include <stdint.h>
#define MAX_DATA_COUNT 3
#define MAX_SCHEDULER_COUNT 100

class Scheduler
{
private:
    struct _Task{
        uint16_t nTime; // hh:mm = hh*100 + mm
        bool fActive=false;
        uint16_t arDuration[MAX_DATA_COUNT];
    } m_arTask[MAX_SCHEDULER_COUNT];
    int m_nCount=0;
    int m_nCurrentIdx = 0;
    int m_nLastIndex = -1;
    void (*onExecuteSchedule)(uint16_t arDuration[]) = nullptr;

public:
    Scheduler(void (*cbExecuteSchedule)(uint16_t arDuration[]));
    void addTask(uint16_t nTime, uint16_t arDuration[]);
    int  start(uint16_t now);
    void run(uint16_t now);
    int currentIdx() { return m_nCurrentIdx; }
    int count() { return m_nCount; }
private:    
    void sort();    
};


