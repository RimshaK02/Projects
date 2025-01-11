//How to compile and run -> gcc mms.c -o mms -> ./mms

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>

#define FALSE 0 //For easier readability
#define TRUE 1 //For easier readability

#define BUFFER_SIZE 10 // Since max length possible is 7, we do 10 to be safe
#define OFFSET_BITS 8 // Page size is 256 = 2^8, offset between 1-255
#define OFFSET_MASK 255 // max offset = 2^8 -1 = 255
#define PAGE_SIZE 256 // Same as Frame Size
#define NUM_PAGES 256 // LogicalAddrSpace/PgSize = (2^16)/(2^8)
#define NUM_FRAMES 128 // PhysicalAddrSpace/PgSize = (2^15)/(2^8)
#define BACKING_STORE_MEM_SIZE 65536
#define TLB_SIZE 16

int total_addresses = 0;
int num_page_faults = 0;
int num_tlb_hits = 0;

int page_table[NUM_PAGES];
int frame_table[NUM_FRAMES]; // Will be used to keep track of which page is currently in which frame

signed char phys_memory[NUM_FRAMES * PAGE_SIZE];
signed char *mmapfptr;

typedef struct {
    int page_num;
    int frame_num;
} TLBentry;

TLBentry TLB[TLB_SIZE];

int tlb_head = 0;  //Tracks FIFO for TLB

// Function Prototypes
int page_fault(int page_num);
int search_TLB(int page_num);
void TLB_Add(int page_num, int frame_num);
void TLB_Update(int old_page_num, int new_page_num, int frame_num);

int main(int argc, char *argv[]) {
    // Initialize Page Table
    for (int i = 0; i < NUM_PAGES; i++) {
        page_table[i] = -1;
    }

    // Initialize Frame Table
    for (int j = 0; j < NUM_FRAMES; j++) {
        frame_table[j] = -1;
    }

    // Initialize Backing Store
    int mmapfile_fd = open("BACKING_STORE.bin", O_RDONLY);
    mmapfptr = mmap(0, BACKING_STORE_MEM_SIZE, PROT_READ, MAP_PRIVATE, mmapfile_fd, 0);

    // Initialize TLB
    for (int i=0; i<TLB_SIZE; i++){
      TLB[i].page_num = -1;
      TLB[i].frame_num = -1;
    }
    
    // Read Virtual Addresses
    FILE *fptr = fopen("addresses.txt", "r");
    char buff[BUFFER_SIZE];
    while (fgets(buff, BUFFER_SIZE, fptr) != NULL) {
        total_addresses++; //Increment total addresses regardless of translation
        int vaddr = atoi(buff); //Str -> Int
        int page_num = vaddr >> OFFSET_BITS; //Cuts off offset bits, keeping page bits
        int offset = vaddr & OFFSET_MASK; //Sets page bits to 0, keeping offset bits

        int frame_num = search_TLB(page_num); //Search TLB

        if (frame_num == -1) {
	    // TLB miss
            if (page_table[page_num] != -1) {
	        //Frame number in page table
                frame_num = page_table[page_num];
            } else {
	        //Frame number not in page table
                num_page_faults++;
                frame_num = page_fault(page_num);
            }
            TLB_Add(page_num, frame_num);  // Add to TLB, maintaining FIFO
        } else {
	    //TLB hit
	    num_tlb_hits++;
        }

        int paddr = (frame_num << OFFSET_BITS) | offset; //Physical address consists of the frame n        umber bits followed by the offset bits
        int val = phys_memory[paddr]; //Access the value at that physical address
        printf("Virtual Address: %d Physical Address = %d Value = %i\n", vaddr, paddr, val);
    }

    printf("Total addresses = %d\n", total_addresses);
    printf("Page_faults = %d\n", num_page_faults);
    printf("TLB hits = %d\n", num_tlb_hits);
    fclose(fptr);
    munmap(mmapfptr, BACKING_STORE_MEM_SIZE);
    return 0;
}

int frame_about_to_be_replaced = 0; //Either a free frame about to be taken, or a taken frame about to be given to a different page

int page_fault(int page_num) {
    memcpy(phys_memory+frame_about_to_be_replaced * PAGE_SIZE, mmapfptr + PAGE_SIZE * (page_num), PAGE_SIZE); //Copy a frame size worth of bytes from the appropriate place in backing store, and put it in the appropriate frame space in physical memory

    if (frame_table[frame_about_to_be_replaced] != -1) {
        //If the frame is already being used for some other page 
        int old_page_num = frame_table[frame_about_to_be_replaced]; //Find what page that is
        page_table[old_page_num] = -1; //Set it to -1 in page table since it will be replaced 

        TLB_Update(old_page_num, page_num, frame_about_to_be_replaced); // Update the TLB for the replaced page
    }

    page_table[page_num] = frame_about_to_be_replaced; //Update page table for the replaced page
    frame_table[frame_about_to_be_replaced] = page_num; //Update frame table for the replaed page

    int frame_to_return = frame_about_to_be_replaced;
    frame_about_to_be_replaced = (frame_about_to_be_replaced + 1) % NUM_FRAMES; //Modular +1 which     loops back to frame 0 once last frame is occupied/replaced to satisfy FIFO
    return frame_to_return;
}

int search_TLB(int page_num) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i].page_num == page_num) {
            return TLB[i].frame_num; // Return frame number if found
        }
    }
    return -1; // Page not found in TLB
}

void TLB_Add(int page_num, int frame_num) {
    //tlb_head represents the next space in the TLB to add an entry to satisfy FIFO
    TLB[tlb_head].page_num = page_num;
    TLB[tlb_head].frame_num = frame_num;

    tlb_head = (tlb_head + 1) % TLB_SIZE; // Move the head in a circular way to satisfy FIFO
}

void TLB_Update(int old_page_num, int new_page_num, int frame_num) {
    //If page being replaced is in the TLB, replace it with new entry 
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i].page_num == old_page_num) {
            TLB[i].page_num = new_page_num;
            TLB[i].frame_num = frame_num;
        }
    }
}
