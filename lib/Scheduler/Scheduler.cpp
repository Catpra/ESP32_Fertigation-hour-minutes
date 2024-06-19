#include <Arduino.h>
#include "Scheduler.h"

Scheduler::Scheduler(void (*cbExecuteSchedule)(uint16_t arDuration[]))
{
    m_nCount=0;
    m_nCurrentIdx = 0;
    m_nLastIndex = -1;
    onExecuteSchedule = cbExecuteSchedule;
}

void Scheduler::addTask(uint16_t nTime, uint16_t arDuration[])
{
    if (m_nCount < MAX_SCHEDULER_COUNT)
    {
        m_arTask[m_nCount].fActive = false;
        m_arTask[m_nCount].nTime = nTime;
        for (int i=0; i<MAX_DATA_COUNT; i++)
            m_arTask[m_nCount].arDuration[i] = arDuration[i];
        m_nCount++;
    }
}

int Scheduler::start(uint16_t now)
{
    m_nLastIndex = -1;
    m_nCurrentIdx=0;
    sort();
    while ((m_nCurrentIdx)<m_nCount && m_arTask[m_nCurrentIdx].nTime<=now)
        m_nCurrentIdx++;
    if (m_nCurrentIdx>0 && m_arTask[m_nCurrentIdx].nTime>now)
        m_nCurrentIdx--;
    if (m_nCurrentIdx>=m_nCount)    
        m_nCurrentIdx = 0;
    Serial.printf("Scheduler Start from index: %d\n", m_nCurrentIdx);
    return m_nCurrentIdx;    
}

void Scheduler::run(uint16_t now)
{
    if (m_nCurrentIdx<m_nCount)
    {
        // Serial.printf("Scheduler Start run index: %d, Active: %d, nTime: %d\n", 
        //     m_nCurrentIdx, m_arTask[m_nCurrentIdx].fActive, m_arTask[m_nCurrentIdx].nTime);

        if (!m_arTask[m_nCurrentIdx].fActive && m_arTask[m_nCurrentIdx].nTime==now)
        {
            m_arTask[m_nCurrentIdx].fActive = true;
            if (m_nLastIndex>=0)
                m_arTask[m_nLastIndex].fActive = false;

            onExecuteSchedule(m_arTask[m_nCurrentIdx].arDuration);

            m_nLastIndex = m_nCurrentIdx++;
            if (m_nCurrentIdx==m_nCount)    
                m_nCurrentIdx = 0;
        }
    }
}

void Scheduler::sort()
{
    for (int i = 0; i < m_nCount; i++)
    {
        for (int j = i + 1; j < m_nCount; j++)
        {
            if (m_arTask[i].nTime > m_arTask[j].nTime)
            {
                _Task pTemp = m_arTask[i];
                m_arTask[i] = m_arTask[j];
                m_arTask[j] = pTemp;
            }
        }
    }

}