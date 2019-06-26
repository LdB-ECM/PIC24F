#ifndef _MEMORY_H
#define _MEMORY_H

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif

#include <stdbool.h>				// C standard unit needed for bool and true/false
#include <stdint.h>					// C standard unti for uint8_t, uint32_t, etc
#include <stddef.h>					// C standard unit needed for size_t

/*--------------------------------------------------------------------------}
{		DELIBERATELY OPAQUE DEFINITION OF A BLOCK LINK STRUCTURE			}
{--------------------------------------------------------------------------*/
	struct block_link32;							/*< Opaque structure .. you dont need to know what it is */

/*--------------------------------------------------------------------------}
{	    	DOUBLE LINKED LIST STRUCTURE OF BLOCK LINK STRUCTURES			}
{--------------------------------------------------------------------------*/
typedef struct memlist							/*< A memory list is a double linked list of those structs */
{
	struct block_link32* head;					/*< Head entry for memory list */
	struct block_link32* tail;					/*< Tail entry for memory list */
} MEMLIST;

/*--------------------------------------------------------------------------}
{					32 BIT ADDRESS MEMORY SYSTEM STRUCTURE 					}
{--------------------------------------------------------------------------*/
typedef struct mem32_sys
{
	MEMLIST pxFree;								/*< List of free blocks of the memory system			*/
	MEMLIST pxAlloc;							/*< List of allocated blocks of the memory system		*/
	uint16_t xAlignment;						/*< Alignment of this 32bit address memory system		*/
	uint16_t xHeapStructSize;					/*< Aligned size of a struct block_link32				*/
	uint32_t xFreeBytesRemaining;				/*< Free bytes remaining of this 32 bit address system	*/
	uint32_t xMinimumEverFreeBytesRemaining;	/*< Minimum number of free bytes ever seen by system	*/
} MEM32_SYSTEM;

/***************************************************************************}
{					   PUBLIC MEMORY INTERFACE ROUTINES					    }
****************************************************************************/

/*-[ mem32_sys_init ]-------------------------------------------------------}
.  Initializes a 32 bit memory address system on the area of memory from
.  start address to end address and respecting the alignment requested.
.  This must be called with valid entries before any use of the system.
.--------------------------------------------------------------------------*/
bool mem32_sys_init (MEM32_SYSTEM* memsys, uint32_t start_addr, uint32_t end_addr, uint16_t alignment);

/*-[ mem32_sys_malloc ]-----------------------------------------------------}
.  Allocates the requested size memory block from the 32 bit memory address 
.  system. It respects the original alignment the memory system was setup 
.  with. Any failure will return NULL just like normal C malloc.
.--------------------------------------------------------------------------*/
void* mem32_sys_malloc (MEM32_SYSTEM* memsys, uint32_t xWantedSize);

/*-[ mem32_sys_free ]-------------------------------------------------------}
.  Releases the memory block from the 32 bit memory address system. Checks
.  are made on the pointer being valid just like normal C free.
.--------------------------------------------------------------------------*/
void mem32_sys_free(MEM32_SYSTEM* memsys, void* pv);

/*-[ mem32_sys_avail ]------------------------------------------------------}
.  Return the available memory of the 32 bit memory address system. This
.  says nothing of the memory fragmentation and the full amount may not be
.  able to be allocated in one large block (see maxavail)
.--------------------------------------------------------------------------*/
uint32_t mem32_sys_avail(MEM32_SYSTEM* memsys);

/*-[ mem32_sys_maxavail ]---------------------------------------------------}
.  Returns the largest contiguous block of memory available from the 32 bit
.  memory address system. This is the largest single memory block that can
.  be safely allocated.
.--------------------------------------------------------------------------*/
uint32_t mem32_sys_maxavail (MEM32_SYSTEM* memsys);

/***************************************************************************}
{	 TESTBENCH CODE USED FOR TESTING ON A SYSTEM .. NORMALLY NOT COMPILED	}
****************************************************************************/
int TEST_MEMORY(uint16_t alignment);

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif



#endif
