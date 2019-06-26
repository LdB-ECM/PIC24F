#include <stdbool.h>				// C standard unit needed for bool and true/false
#include <stdint.h>					// C standard unit needed for uint8_t, uint32_t, etc
#include "memory.h"

/*--------------------------------------------------------------------------}
{			 BLOCK LINK STRUCTURE - INTERNAL USE ONLY KEEP OPAQUE			}
{--------------------------------------------------------------------------*/
typedef struct block_link32
{
	struct block_link32* next;					/*< The next block in the list.							*/
	struct block_link32* prev;					/*< The prev block in the list.							*/
	struct memlist* owner;						/*< List that owns this entry							*/
	uint32_t xBlockSize;						/*< The size of the free block.							*/
} BLOCKLINK32;

/*--------------------------------------------------------------------------}
{			Adds the memory into the memory double linked list				}
{--------------------------------------------------------------------------*/
static void AddMemToList (MEMLIST* list, BLOCKLINK32* mem)
{
	if (list->tail != 0)											// List already has at least one task
	{
		/* Insert task into double linked ready list */
		list->tail->next = mem;										// Add mem to current list tail
		mem->prev = list->tail;										// Set mem prev to current list tail
		list->tail = mem;											// Now mem becomes the list tail
		mem->next = 0;												// Zero the memk next pointer
	}
	else {															// No existing mem in list
	 /* Init ready list */
		list->tail = mem;											// Task is the tail
		list->head = mem;											// Task is also head
		mem->next = 0;												// Task has no next ready
		mem->prev = 0;												// Task has no prev ready
	}
	mem->owner = list;												// Set the list as owner of task
}

/*--------------------------------------------------------------------------}
{	  	Removes the mem from whatever double linked list it is in			}
{--------------------------------------------------------------------------*/
static bool RemoveMemFromList (BLOCKLINK32* mem)
{
	if (mem && mem->owner)											// mem pointer and owner both valid
	{
		MEMLIST* list = mem->owner;									// Grab the owner as list									
		if (mem == list->head)										// mem is the first one in the list		
		{
			if (mem == list->tail)									// If mem is also the last one on list it is only mem in list
			{
				list->tail = 0;										// Zero the tail task ptr
				list->head = 0;										// Zero the head task ptr
			}
			else {
				list->head = mem->next;								// Move next mem up to list head position
				list->head->prev = 0;								// This mem is now top its previous is then null (it would have been the mem we are removing)
			}
		}
		else {														// mem is not head then must be in list
			if (mem == list->tail)									// Is mem the tail ptr in list
			{
				list->tail = mem->prev;								// List tail will now become mem previous 
				list->tail->next = 0;								// Zero that mem next pointer (it would have been the mem we are removing)
			}
			else {
				mem->next->prev = mem->prev;						// Our next prev will point to our prev (unchains mem from next links)
				mem->prev->next = mem->next;						// Our prev next will point to our next (unchains mem from prev links)
			}
		}
		mem->next = 0;												// Zero next pointer
		mem->prev = 0;												// Zero previous pointer
		mem->owner = 0;												// Zero mem owner pointer
		return true;												// Return success
	}
	return false;													// Something fundementally wrong
}


/***************************************************************************}
{					   PUBLIC MEMORY INTERFACE ROUTINES					    }
****************************************************************************/

/*-[ mem32_sys_init ]-------------------------------------------------------}
.  Initializes a 32 bit memory address system on the area of memory from
. start address to end address and respecting the alignment requested.
. This must be called with valid entries before any use of the system.
.--------------------------------------------------------------------------*/
bool mem32_sys_init(MEM32_SYSTEM* memsys, uint32_t start_addr, uint32_t end_addr, uint16_t alignment)
{
	if (memsys && end_addr > start_addr)							// Pointer and addresses valid
	{
		BLOCKLINK32* pxStart;
		uint16_t xHeapStructSize;
		uint32_t mask = 0xFFFFFFFF - alignment + 1;					// Create alignment mask
		start_addr = (start_addr + alignment - 1) & mask;			// Make sure start_addr is aligned to upper alignment granual
		end_addr &= mask;											// Make sure end_addr is aligned to lower alignment granual				
		xHeapStructSize = (sizeof(BLOCKLINK32) + alignment - 1) & mask; // Calculate heap structure size 
		pxStart = (BLOCKLINK32*)start_addr;							// Set the memory system start block
		

		/* To start with there is a single free block that is sized to take up the
		entire heap space, minus the space taken by pxEnd. */
		pxStart->next = 0;											// We have only one block there is no next
		pxStart->prev = 0;											// We have only one block there is no prev
		pxStart->xBlockSize = end_addr - start_addr - xHeapStructSize;// Initial memory size bewteen start and end minus struct size
		pxStart->owner = 0;											// Start block starts free with no owner

		/* Set all the memsys values */
		memsys->pxFree = (MEMLIST){ 0 };							// Initialize the free list
		memsys->pxAlloc = (MEMLIST){ 0 };							// Initialze the alloc list
		AddMemToList(&memsys->pxFree, pxStart);						// The free list
		memsys->xHeapStructSize = xHeapStructSize;					// Hold heap size structure
		memsys->xAlignment = alignment;								// Hold system alignment
		memsys->xFreeBytesRemaining = pxStart->xBlockSize;			// Amount of memory avilable
		memsys->xMinimumEverFreeBytesRemaining = pxStart->xBlockSize;// Minimum ever seen starts as max
		return true;
	}
	return false;													// Memory initialize failed
}

/*-[ mem32_sys_malloc ]-----------------------------------------------------}
.  Allocates the requested size memory block from the 32 bit memory address
.  system. It respects the original alignment the memory system was setup
.  with. Any failure will return NULL just like normal C malloc.
.--------------------------------------------------------------------------*/
void* mem32_sys_malloc (MEM32_SYSTEM* memsys, uint32_t xWantedSize)
{
	void* pvReturn = 0;
	if (xWantedSize > 0)
	{
		xWantedSize += memsys->xHeapStructSize;						// Need to add a heap struct size to request
		uint32_t mask = memsys->xAlignment - 1;						// Create a mask to make sure allocation will be aligned
		if ((xWantedSize & mask) != 0x00)							// If wanted size is not an aligned size we need to pad
		{															// Pad the wanted size to alignment
			xWantedSize += (memsys->xAlignment - (xWantedSize & mask));
		}
	}

	if ((xWantedSize > 0) && (xWantedSize <= memsys->xFreeBytesRemaining)) // Check we at least have a chance we have a block
	{
		BLOCKLINK32* pxBlock = memsys->pxFree.head;					// Start on first free block
		while (pxBlock != 0 && pxBlock->xBlockSize < xWantedSize)
			pxBlock = pxBlock->next;								// Search all free list for one big enough 
		if (pxBlock)												// Block with enough space found
		{
			pvReturn = (void*)(((uint8_t*)pxBlock) + memsys->xHeapStructSize);
			RemoveMemFromList(pxBlock);								// Remove the memory block from list
			if ((pxBlock->xBlockSize - xWantedSize) > ((uint32_t)memsys->xHeapStructSize * 2))
			{														// We can break it an put a smaller one back
				BLOCKLINK32* pxNewBlockLink;
				pxNewBlockLink = (void*)(((uint8_t*)pxBlock) + xWantedSize);
				pxNewBlockLink->next = 0;							// New block has no next
				pxNewBlockLink->prev = 0;							// New block has no prev
				pxNewBlockLink->owner = 0;							// New block has no owner
				pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
				pxBlock->xBlockSize = xWantedSize;					// We can reduce the size of pxBlock now
				AddMemToList(&memsys->pxFree, pxNewBlockLink);		// Add the broken small block back into free list
			}
			AddMemToList(&memsys->pxAlloc, pxBlock);				// Add pxBlock to allocated list
			memsys->xFreeBytesRemaining -= pxBlock->xBlockSize;		// Reduce the memory left

			/* If this is the lowest memory the system has seen then hold the value */
			if (memsys->xFreeBytesRemaining < memsys->xMinimumEverFreeBytesRemaining)
			{
				memsys->xMinimumEverFreeBytesRemaining = memsys->xFreeBytesRemaining;
			}
		}
	}
	return pvReturn;
}

/*-[ mem32_sys_free ]-------------------------------------------------------}
.  Releases the memory block from the 32 bit memory address system. Checks
.  are made on the pointer being valid just like normal C free.
.--------------------------------------------------------------------------*/
void mem32_sys_free (MEM32_SYSTEM* memsys, void* pv)
{
	uint8_t* puc = (uint8_t*)pv;
	if (pv != 0)
	{
		BLOCKLINK32* pxLink;
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= memsys->xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = (void*)puc;

		if (pxLink->owner == &memsys->pxAlloc)						// It is a valid memory pointer
		{
			// yep valid remove from allocated list
			RemoveMemFromList(pxLink);								// Remove the block from it's owner
			memsys->xFreeBytesRemaining += pxLink->xBlockSize;		// Add the memory back to pool

			BLOCKLINK32* pxBlock = memsys->pxFree.head;				// Start on first free block
			while (pxBlock != 0 && (uint32_t)pxBlock + pxBlock->xBlockSize != (uint32_t)pxLink)
				pxBlock = pxBlock->next;							// Search for prior block
			if (pxBlock)											// Prior free block exists
			{
				RemoveMemFromList(pxBlock);							// Remove the prior block	
				pxBlock->xBlockSize += pxLink->xBlockSize;			// Add free blcok size to it
				pxLink = pxBlock;									// PxLink now becomes the prior block
			}

			pxBlock = memsys->pxFree.head;							// Start on first free block
			while (pxBlock != 0 && (uint32_t)pxLink + pxLink->xBlockSize != (uint32_t)pxBlock)
				pxBlock = pxBlock->next;							// Search for prior block
			if (pxBlock)											// Prior free block exists
			{
				RemoveMemFromList(pxBlock);							// Remove that block from list
				pxLink->xBlockSize += pxBlock->xBlockSize;			// Add blcok size to it
			}

			AddMemToList(&memsys->pxFree, pxLink);					// Now simply insert the free pointer back to free list

		}
	}
}

/*-[ mem32_sys_avail ]------------------------------------------------------}
.  Return the available memory of the 32 bit memory address system. This 
.  says nothing of the memory fragmentation and the full amount may not be
.  able to be allocated in one large block (see maxavail)
.--------------------------------------------------------------------------*/
uint32_t mem32_sys_avail (MEM32_SYSTEM* memsys)
{
	if (memsys) return  memsys->xFreeBytesRemaining;				// Return free bytes available
	else return 0;													// Return zero
}

/*-[ mem32_sys_maxavail ]---------------------------------------------------}
.  Returns the largest contiguous block of memory available from the 32 bit 
.  memory address system. This is the largest single memory block that can
.  be safely allocated.
.--------------------------------------------------------------------------*/
uint32_t mem32_sys_maxavail (MEM32_SYSTEM* memsys)
{
	uint32_t retVal = 0;
	if (memsys) {
		BLOCKLINK32* pxBlock = memsys->pxFree.head;					// Start on first free block
		while (pxBlock != 0)
		{
			if (pxBlock->xBlockSize - memsys->xHeapStructSize > retVal)
			{
				retVal = pxBlock->xBlockSize - memsys->xHeapStructSize;
			}
			pxBlock = pxBlock->next;								// Move to next free block
		}
	}
	return retVal;													// Return largest block found
}


/***************************************************************************}
{	 TESTBENCH CODE USED FOR TESTING ON A SYSTEM .. NORMALLY NOT COMPILED	}
****************************************************************************/
#ifdef TESTBENCH
#include <stdio.h>					// Needed for printf
#include <stdlib.h>					// Needed for RAND
static uint8_t buffer[2000000];
typedef char* charptr;
int TEST_MEMORY (uint16_t alignment)
{
	uint32_t max;
	MEM32_SYSTEM mem;
	charptr test[100];

	mem32_sys_init(&mem, (uint32_t)& buffer[0], (uint32_t)& buffer[2000000], alignment);
	printf("mem avail %lu\n", mem32_sys_avail(&mem));

	max = mem32_sys_maxavail(&mem);
	printf("Largest allocation possible: %lu\n", max);
	test[0] = mem32_sys_malloc(&mem, max);
	if (test[0])
	{
		printf("Max allocation success\n");
		mem32_sys_free(&mem, test[0]);
		printf("Releasing allocation for next test\n");
	}
	else printf("Max allocation failed\n");

	int i;
	for (i = 0; i < 100; i++)
	{
		test[i] = mem32_sys_malloc(&mem, (rand() % 2500) + 10);
	}
	printf("100 random allocations done\n");

	printf("Random order free of those allocations\n");
	while (i != 0)
	{
		int j = (rand() % 100);
		if (test[j] != 0) {
			mem32_sys_free(&mem, test[j]);
			test[j] = 0;
			i--;
		}
	}

	printf("mem avail %lu\n", mem32_sys_avail(&mem));
	if (mem.pxAlloc.head == 0 && mem.pxAlloc.tail == 0
		&& mem.pxFree.head == mem.pxFree.tail)
	{
		printf("100 Random malloc and free in random order worked correctly\n");
		printf("Min Memory Ever achieved %lu\n", mem.xMinimumEverFreeBytesRemaining);
	}
	return (0);
}
#endif


