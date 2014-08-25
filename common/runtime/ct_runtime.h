#ifndef CT_RUNTIME_H
#define CT_RUNTIME_H

#include "../eventLib/ct_event_st.h"
//#include "../taskLib/ct_file.h"
#include <pthread.h>
#include <zlib.h>
#include <stdint.h>

#define SERIAL_BUFFER_SIZE (1024 * 1024)

// Used to store serial data
typedef struct _ct_serial_buffer
{
    unsigned int pos, length, id;
    struct _ct_serial_buffer* next; // can order buffers 
    //char pad[24];
    char data[0];
} ct_serial_buffer, *pct_serial_buffer;

typedef struct _contech_thread_create {
    void* (*func)(void*);
    void* arg;
    unsigned int parent_ctid;
    unsigned int child_ctid;
    ct_tsc_t volatile child_skew;
    char pad[64];
    ct_tsc_t volatile parent_skew;
} contech_thread_create, *pcontech_thread_create;

typedef struct _contech_thread_info {
    pthread_t pt_info;
    unsigned int ctid;
    struct _contech_thread_info* next;
} contech_thread_info, *pcontech_thread_info;

typedef struct _contech_id_stack {
    unsigned int id;
    struct _contech_id_stack* next;
} contech_id_stack, *pcontech_id_stack;

typedef struct _contech_join_stack {
    ct_tsc_t start;
    unsigned int id;
    struct _contech_join_stack* next;
} contech_join_stack, *pcontech_join_stack;

void __ctCleanupThread(void* v);
void __ctAllocateLocalBuffer();
unsigned int __ctAllocateCTid();

int __ctThreadCreateActual(pthread_t*, const pthread_attr_t*, void * (*start_routine)(void *), void*);

// Create event for thread and parent
//   Puts event for parent ctid into a buffer
//   Allocates a new ctid for thread and assigns it
//   And thread ctid to thread ctid stack
void __ctOMPThreadCreate(unsigned int parent);
// create event for thread and task
//   if int == 0, pop thread ctid from stack
//   else create events with task and thread ids
void __ctOMPTaskCreate(int);
// join event for thread and task
//   if thread and local ids differ, then we are in task context
//   else ignore
void __ctOMPTaskJoin();
void __ctOMPThreadJoin(unsigned int parent);

// Push current ctid onto parent stack
void __ctOMPPushParent();
// Pop current ctid off of parent stack
//   N.B. This assumes that the returning context is the same as the caller
void __ctOMPPopParent();
void __ctPushIdStack(pcontech_id_stack*, unsigned int);
unsigned int __ctPopIdStack(pcontech_id_stack*);
unsigned int __ctPeekIdStack(pcontech_id_stack*);

pct_serial_buffer ctInternalAllocateBuffer();

void __ctQueueBuffer(bool);
// (contech_id, basic block id, num of ops)
char* __ctStoreBasicBlock(unsigned int bbid);
// (basic block id, size of string, string)
void __ctStoreBasicBlockInfo (unsigned int, unsigned int, char*);
void __ctStoreMemOp(void*, unsigned int, char*);
unsigned int __ctStoreBasicBlockComplete(unsigned int);
void __ctStoreThreadCreate(unsigned int, long long, ct_tsc_t);
void __ctStoreThreadJoin(pthread_t, ct_tsc_t);
void __ctStoreSync(void*, int, int, ct_tsc_t);
void __ctStoreBarrier(bool, void*, ct_tsc_t);
void __ctStoreMemoryEvent(bool, size_t, void*);
void* __ctInitThread(void*);//pcontech_thread_create ptc
void __ctCheckBufferSize(unsigned int);
void __ctStoreDelay(ct_tsc_t start_t);

void __ctAddThreadInfo(pthread_t *pt, unsigned int);
unsigned int __ctLookupThreadInfo(pthread_t pt);

typedef struct _ct_serial_buffer_sized
{
    unsigned int pos, length, id;
    struct _ct_serial_buffer* next; // can order buffers 
    char data[SERIAL_BUFFER_SIZE];
} ct_serial_buffer_sized;

extern ct_serial_buffer_sized initBuffer;

extern __thread pct_serial_buffer __ctThreadLocalBuffer;
extern __thread unsigned int __ctThreadLocalNumber; // no static
extern __thread pcontech_thread_info __ctThreadInfoList;
extern __thread pcontech_id_stack __ctParentIdStack;
extern __thread pcontech_id_stack __ctThreadIdStack;
extern __thread pcontech_join_stack __ctJoinStack;

extern unsigned long long __ctGlobalOrderNumber;
extern unsigned int __ctThreadGlobalNumber;
extern unsigned int __ctThreadExitNumber;
extern unsigned int __ctMaxBuffers;
extern unsigned int __ctCurrentBuffers;
extern pct_serial_buffer __ctQueuedBuffers;
extern pct_serial_buffer __ctQueuedBufferTail;
extern pct_serial_buffer __ctFreeBuffers;
// Setting the size in a variable, so that future code can tune / change this value
const extern size_t serialBufferSize;

extern pthread_mutex_t __ctQueueBufferLock;
extern pthread_cond_t __ctQueueSignal;
extern pthread_mutex_t __ctFreeBufferLock;
extern pthread_cond_t __ctFreeSignal;

extern pthread_mutex_t ctAllocLock;

extern uint8_t _binary_contech_bin_start[];// asm("_binary_contech_bin_start");
extern uint8_t _binary_contech_bin_size[];// asm("_binary_contech_bin_size");
extern uint8_t _binary_contech_bin_end[];//  asm("_binary_contech_bin_end");

#endif
