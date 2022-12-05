#include "mmu.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// byte addressable memory
unsigned char RAM[RAM_SIZE]; 


// OS's memory starts at the beginning of RAM.
// Store the process related info, page tables or other data structures here.
// do not use more than (OS_MEM_SIZE: 72 MB).
unsigned char* OS_MEM = RAM;  

// memory that can be used by processes.   
// 128 MB size (RAM_SIZE - OS_MEM_SIZE)
unsigned char* PS_MEM = RAM + OS_MEM_SIZE; 


// This first frame has frame number 0 and is located at start of RAM(NOT PS_MEM).
// We also include the OS_MEM even though it is not paged. This is 
// because the RAM can only be accessed through physical RAM addresses.  
// The OS should ensure that it does not map any of the frames, that correspond
// to its memory, to any process's page. 
int NUM_FRAMES = ((RAM_SIZE) / PAGE_SIZE);

// Actual number of usable frames by the processes.
int NUM_USABLE_FRAMES = ((RAM_SIZE - OS_MEM_SIZE) / PAGE_SIZE);

// To be set in case of errors. 
int error_no; 


/*
CODE=000;
ROD=001;
RWD=010;
HEAP=011;
STACK=100;

32 bit:
    1st 3 bit: part of code()
*/


int power(int i,int j){
    if(j==0) return 1;
    if(j==1) return i;
    int ans=i;
    for(int k=1;k<j;k++){
        ans*=i;
    }
    return ans;
}




// ---------------------- Helper functions for Page table entries ------------------ // 

// return the frame number from the pte
int pte_to_frame_numm(int pte) 
{
    // TODO: student
    int t=pte;
    int num=0;
    for(int k=0;k<28;k++){
        if(RAM[t+3+k]!='p'){
            int m=RAM[t+3+k]-'0';
            if(m==0 || m==1)num+=(m*power(2,k));
        }
    }
    return (num-72*1024*1024)/(4*1024);
}


// return 1 if read bit is set in the pte
// 0 otherwise
int is_readablee(int pte) {
    // TODO: student
    return 1;
}

// return 1 if write bit is set in the pte
// 0 otherwise
int is_writeablee(int pte) {
    // TODO: student
    if(RAM[pte+31]=='4' || (RAM[pte]=='0' && RAM[pte+1]=='1' && RAM[pte+2]=='0') || (RAM[pte]=='1' && RAM[pte+1]=='0' && RAM[pte+2]=='0'))return 1;
    return 0;
}

// return 1 if executable bit is set in the pte
// 0 otherwise
int is_executablee(int pte) {
    // TODO: student
    if(RAM[pte+31]=='2' || (RAM[pte]=='0' && RAM[pte+1]=='0' && RAM[pte+2]=='0')) return 1;
    return 0;
}


// return 1 if present bit is set in the pte
// 0 otherwise
int is_presentt(int pte) {
    // TODO: student
    int t=pte;
    int num=0;
    for(int k=0;k<28;k++){
        if(RAM[t+3+k]!='p'){
            int m=RAM[t+3+k]-'0';
            if(m==0 || m==1)num+=(m*power(2,k));
        }
    }
    if(num==0 || RAM[num]=='a' || RAM[num]=='u')return 0;
    return 1;
}



void os_init() {
    memset(RAM,'u',RAM_SIZE*sizeof(char));
}


// ----------------------------------- Functions for managing memory --------------------------------- //

/**
 *  Process Virtual Memory layout: 
 *  ---------------------- (virt. memory start 0x00)
 *        code
 *  ----------------------  
 *     read only data 
 *  ----------------------
 *     read / write data
 *  ----------------------
 *        heap
 *  ----------------------
 *        stack  
 *  ----------------------  (virt. memory end 0x3fffff)
 * 
 * 
 *  code            : read + execute only
 *  ro_data         : read only
 *  rw_data         : read + write only
 *  stack           : read + write only
 *  heap            : (protection bits can be different for each heap page)
 * 
 *  assume:
 *  code_size, ro_data_size, rw_data_size, max_stack_size, are all in bytes
 *  code_size, ro_data_size, rw_data_size, max_stack_size, are all multiples of PAGE_SIZE
 *  code_size + ro_data_size + rw_data_size + max_stack_size < PS_VIRTUAL_MEM_SIZE
 *  
 * 
 *  The rest of memory will be used dynamically for the heap.
 * 
 *  This function should create a new process, 
 *  allocate code_size + ro_data_size + rw_data_size + max_stack_size amount of physical memory in PS_MEM,
 *  and create the page table for this process. Then it should copy the code and read only data from the
 *  given `unsigned char* code_and_ro_data` into processes' memory.
 *   
 *  It should return the pid of the new process.  
 *  
 */
int create_ps(int code_size, int ro_data_size, int rw_data_size,
                 int max_stack_size, unsigned char* code_and_ro_data) 
{   
    bool can_create=0;
    int new_PID=0;
    for(int i=1;i<=100;i++){
        if(RAM[OS_MEM_SIZE-i]=='u'){
            RAM[OS_MEM_SIZE-i]='1';
            can_create=1;
            new_PID=i;
            break;
        }
    }
    if(!can_create){
        return -1;
    }
    RAM[new_PID]='1';
    // Allocating code
    int num_code_pages=code_size/(4*1024);
    for(int i=0;i<num_code_pages;i++){
        for(int j=OS_MEM_SIZE;j<RAM_SIZE;j+=4*1024){
            if(RAM[j]=='u'){
                for(int t=0;t<4*1024;t++){
                    RAM[j+t]=code_and_ro_data[i*4*1024+t];
                    
                }
                for(int t=32768*new_PID;t<32768*(new_PID+1);t+=32){
                    if(RAM[t]=='u'){
                        RAM[t]='0';
                        RAM[t+1]='0';
                        RAM[t+2]='0';
                        int num=j;
                        for(int k=0;k<28;k++){
                            if(num>0){
                                RAM[t+3+k]='0'+num%2;
                                num=num/2;
                            }
                            else RAM[t+3+k]='p';
                        }
                        RAM[t+31]='p';
                        break;
                    }
                }
                break;
            }
        }
    }
    

    // Allocating ro_data
    int num_ro_data=ro_data_size/(4*1024);
    for(int i=0;i<num_ro_data;i++){
        for(int j=OS_MEM_SIZE;j<RAM_SIZE;j+=4*1024){
            if(RAM[j]=='u'){
                for(int t=0;t<4*1024;t++){
                    RAM[j+t]=code_and_ro_data[(num_code_pages+i)*4*1024+t];
                }
                for(int t=32768*new_PID;t<32768*(new_PID+1);t+=32){
                    if(RAM[t]=='u'){
                        RAM[t]='0';
                        RAM[t+1]='0';
                        RAM[t+2]='1';
                        int num=j;
                        for(int k=0;k<28;k++){
                            if(num>0){
                                RAM[t+3+k]='0'+num%2;
                                num=num/2;
                            }
                            else RAM[t+3+k]='p';
                        }
                        RAM[t+31]='p';
                        break;
                    }
                }
                break;
            }
        }
    }
    
    // Allocating rw_data
    int num_rw_data=rw_data_size/(4*1024);
    for(int i=0;i<num_rw_data;i++){
        for(int j=OS_MEM_SIZE;j<RAM_SIZE;j+=4*1024){
            if(RAM[j]=='u'){
                for(int t=0;t<4*1024;t++){
                    RAM[j+t]='w';
                }
                for(int t=32768*new_PID;t<32768*(new_PID+1);t+=32){
                    if(RAM[t]=='u'){
                        RAM[t]='0';
                        RAM[t+1]='1';
                        RAM[t+2]='0';
                        int num=j;
                        for(int k=0;k<28;k++){
                            if(num>0){
                                RAM[t+3+k]='0'+num%2;
                                num=num/2;
                            }
                            else RAM[t+3+k]='p';
                        }
                        RAM[t+31]='p';
                        break;
                    }
                }
                break;
            }
        }
    }
    
    // Allocating stack
    int num_stack=max_stack_size/(4*1024);
    for(int i=0;i<num_stack;i++){
        for(int j=OS_MEM_SIZE;j<RAM_SIZE;j+=4*1024){
            if(RAM[j]=='u'){
                for(int t=0;t<4*1024;t++){
                    RAM[j+t]='s';
                }
                for(int t=32768*(new_PID+1)-32;t>=32768*(new_PID);t-=32){
                    if(RAM[t]=='u'){
                        RAM[t]='1';
                        RAM[t+1]='0';
                        RAM[t+2]='0';
                        int num=j;
                        for(int k=0;k<28;k++){
                            if(num>0){
                                RAM[t+3+k]='0'+num%2;
                                num=num/2;
                            }
                            else RAM[t+3+k]='p';
                        }
                        RAM[t+31]='p';
                        break;
                    }
                }
                break;
            }
        }
    }
    
    return new_PID;

}

/**
 * This function should deallocate all the resources for this process. 
 * 
 */
void exit_ps(int pid) 
{
    // TODO student
    RAM[OS_MEM_SIZE-pid]='u';
    for(int t=32768*pid;t<32768*(pid+1);t+=32){
        RAM[t]='u';
        RAM[t+1]='u';
        RAM[t+2]='u';
        int num=0;
        for(int k=0;k<28;k++){
            if(RAM[t+3+k]!='p'){
                int m=RAM[t+3+k]-'0';
                if(m==0 || m==1)if(m==0 || m==1)num+=(m*power(2,k));
            }
            RAM[t+k+3]='u';
            
        }
        RAM[t+31]='u';
        bool tt=num<0 || num+4*1024>=RAM_SIZE;
        if(!tt){
            for(int i=0;i<4*1024;i++){
                RAM[num]='u';
            }
        }
    }
    return;
}



/**
 * Create a new process that is identical to the process with given pid. 
 * 
 */
int fork_ps(int pid) {

    // TODO student:
    bool can_create=0;
    int new_PID=0;
    for(int i=1;i<=100;i++){
        if(RAM[OS_MEM_SIZE-i]=='u'){
            RAM[OS_MEM_SIZE-i]='1';
            can_create=1;
            new_PID=i;
            break;
        }
    }
    if(!can_create){
        return -1;
    }
    int diff=32768*(new_PID-pid);
    for(int t=32768*pid;t<32768*(pid+1);t+=32){
        RAM[t+diff]=RAM[t];
        RAM[t+1+diff]=RAM[t+1];
        RAM[t+2+diff]=RAM[t+2];
        int num=0;
        for(int k=0;k<28;k++){
            if(RAM[t+3+k]!='p'){
                int m=RAM[t+3+k]-'0';
                if(m==0 || m==1)num+=(m*power(2,k));
            }
        }
        int new_num=0;
        for(int j=OS_MEM_SIZE;j<RAM_SIZE;j+=4*1024){
            if(RAM[j]=='u'){
                new_num=j;
                break;
            }
        }
        for(int i=0;i<4*1024;i++){
            RAM[new_num+i]=RAM[num+i];
        }
        for(int k=0;k<28;k++){
            if(new_num>0){
                RAM[t+3+diff+k]='0'+new_num%2;
                new_num=new_num/2;
            }
            else RAM[t+3+k+diff]='p';
        }
        RAM[t+31+diff]=RAM[t+31];
    }
    return 0;
}



// dynamic heap allocation
//
// Allocate num_pages amount of pages for process pid, starting at vmem_addr.
// Assume vmem_addr points to a page boundary.  
// Assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
//
//
// Use flags to set the protection bits of the pages.
// Ex: flags = O_READ | O_WRITE => page should be read & writeable.
//
// If any of the pages was already allocated then kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
void allocate_pages(int pid, int vmem_addr, int num_pages, int flags) 
{
    // TODO student
    if(RAM[pid]=='u'){
        error_no=ERR_SEG_FAULT;
        return;
    }
    int vmem_pn=vmem_addr/(4*1024);
    for(int page=0;page<num_pages;page++){
        int t=32768*pid+(vmem_pn+page)*32;
        if(RAM[t]!='u'){
            error_no=ERR_SEG_FAULT;
            exit_ps(pid);
            return;
        }  
        for(int j=OS_MEM_SIZE;j<RAM_SIZE;j+=4*1024){
            if(RAM[j]=='u'){
                for(int t=0;t<4*1024;t++){
                    RAM[j+t]='a';
                }
                RAM[t]='1';
                RAM[t+1]='0';
                RAM[t+2]='0';
                char f=flags+'0';
                RAM[t+31]=f;
                int num=j;
                for(int k=0;k<28;k++){
                    if(num>0){
                        RAM[t+3+k]='0'+num%2;
                        num=num/2;
                    }
                    else RAM[t+3+k]='p';
                }
                break;
            }
        }
    }
    return;

}



// dynamic heap deallocation
//
// Deallocate num_pages amount of pages for process pid, starting at vmem_addr.
// Assume vmem_addr points to a page boundary
// Assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE

// If any of the pages was not already allocated then kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
void deallocate_pages(int pid, int vmem_addr, int num_pages) 
{
   // TODO student
    
    if(RAM[pid]=='u'){
        error_no=ERR_SEG_FAULT;
        return;
    }
    int vmem_pn=vmem_addr/(4*1024);
    for(int page=0;page<num_pages;page++){
        int t=32768*pid+(vmem_pn+page)*32;
        RAM[t]='u';
        RAM[t+1]='u';
        RAM[t+2]='u';
        int num=0;
        for(int k=0;k<28;k++){
            if(RAM[t+3+k]!='p'){
                int m=RAM[t+3+k]-'0';
                if(m==0 || m==1)num+=(m*power(2,k));
            }
            RAM[t+k+3]='u';
            
        }
        RAM[t+31]='u';
        for(int i=0;i<4*1024;i++){
            RAM[num+i]='u';
        }
    }
}

// Read the byte at `vmem_addr` virtual address of the process
// In case of illegal memory access kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
// 
// assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
unsigned char read_mem(int pid, int vmem_addr) 
{
    // TODO: student

    if(RAM[pid]=='u'){
        error_no=ERR_SEG_FAULT;
        return 'u';
    }
    int vmem_pn=vmem_addr/(4*1024);
    int t=32768*pid+vmem_pn*32;
    int num=0;
    for(int k=0;k<28;k++){
        if(RAM[t+3+k]!='p'){
            int m=RAM[t+3+k]-'0';
            if(m==0 || m==1)num+=(m*power(2,k));
        }   
    }
    if(!is_readablee(t)){
        error_no=ERR_SEG_FAULT;
        return 'u';
    }
    return RAM[num+vmem_addr%(4*1024)];
}

// Write the given `byte` at `vmem_addr` virtual address of the process
// In case of illegal memory access kill the process, deallocate all its resources(ps_exit) 
// and set error_no to ERR_SEG_FAULT.
// 
// assume 0 <= vmem_addr < PS_VIRTUAL_MEM_SIZE
void write_mem(int pid, int vmem_addr, unsigned char byte) 
{
    // TODO: student
    if(RAM[pid]=='u'){
        error_no=ERR_SEG_FAULT;
        return;
    }
    int vmem_pn=vmem_addr/(4*1024);
    int t=32768*pid+vmem_pn*32;

    if(!is_writeablee(t)){
        error_no=ERR_SEG_FAULT;
        return;
    }
    int num=0;
    for(int k=0;k<28;k++){
        if(RAM[t+3+k]!='p'){
            int m=RAM[t+3+k]-'0';
            if(m==0 || m==1)num+=(m*power(2,k));
        }   
    }
    RAM[num+vmem_addr%(4*1024)]=byte;
    return;
}



// -------------------  functions to print the state  --------------------------------------------- //

void print_page_table(int pid) 
{
    
    int page_table_start = 32768*pid; // TODO student: start of page table of process pid
    int num_page_table_entries = 0;           // TODO student: num of page table entries
    int i=0;
    puts("------ Printing page table-------");
    for(int t=32768*pid;t<32768*(pid+1);t+=32){
        if(RAM[t]!='u'){
            num_page_table_entries+=1;
            int pte=t;
            printf("Page num: %d, frame num: %d, R:%d, W:%d, X:%d, P%d\n", 
                i, 
                pte_to_frame_numm(pte),
                is_readablee(pte),
                is_writeablee(pte),
                is_executablee(pte),
                is_presentt(pte)
                );
            i++;
        }
    }

}


