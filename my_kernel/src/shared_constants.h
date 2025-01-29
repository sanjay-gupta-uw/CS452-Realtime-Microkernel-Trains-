#ifndef _shared_constants_h_
#define _shared_constants_h_

// priority levels
typedef enum
{
   HIGH,
   MEDIUM,
   LOW,
   IDLE,
} Priority;

typedef enum
{
   SVC_CREATE,
   SVC_MYTID,
   SVC_MYPARENTID,
   SVC_YIELD,
   SVC_EXIT,
} Syscall;

#endif