#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"


extern uns64 cycle; // You can use this as timestamp for LRU
extern uns64 SWP_CORE0_WAYS;
////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
////////////////////////////////////////////////////////////////////

Cache  *cache_new(uns64 size, uns64 assoc, uns64 linesize, uns64 repl_policy){
    
    Cache *c = (Cache *) calloc (1, sizeof (Cache));
    c->num_ways = assoc;
    c->repl_policy = repl_policy;
    
    if(c->num_ways > MAX_WAYS){
        printf("Change MAX_WAYS in cache.h to support %llu ways\n", c->num_ways);
        exit(-1);
    }
    
    // determine num sets, and init the cache
    c->num_sets = size/(linesize*assoc);
    c->sets  = (Cache_Set *) calloc (c->num_sets, sizeof(Cache_Set));
    
    return c;
}

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION -----------
////////////////////////////////////////////////////////////////////

void    cache_print_stats    (Cache *c, char *header){
    double read_mr =0;
    double write_mr =0;
    
    if(c->stat_read_access){
        read_mr=(double)(c->stat_read_miss)/(double)(c->stat_read_access);
    }
    
    if(c->stat_write_access){
        write_mr=(double)(c->stat_write_miss)/(double)(c->stat_write_access);
    }
    
    printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
    printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
    printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
    printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
    printf("\n%s_READ_MISSPERC  \t\t : %10.3f", header, 100*read_mr);
    printf("\n%s_WRITE_MISSPERC \t\t : %10.3f", header, 100*write_mr);
    printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);
    
    printf("\n");
}



////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise
// Also if is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
////////////////////////////////////////////////////////////////////

Flag cache_access(Cache *c, Addr lineaddr, uns is_write, uns core_id){
    
    // Your Code Goes Here
    
    Flag outcome=MISS;
    
    if(is_write){
        
        c->stat_write_access++;
        
        
    }
    else c->stat_read_access++;
    //if (cycle_count % 100000 == 0)
    //  printf ("%19.0x \n", lineaddr);
    Addr tag = lineaddr / c->num_sets;
    Addr index = lineaddr % c->num_sets;
    
    for(uns64 i =0; i< c->num_ways; i++){
        
        
        if(c->sets[index].line[i].tag == tag){
            if (c->sets[index].line[i].valid){
                c->sets[index].line[i].last_access_time = cycle;
                if (is_write) c->sets[index].line[i].dirty = TRUE;
                outcome = HIT;
            }
        }
        
    }
    if (!outcome){
        if(is_write) c->stat_write_miss ++;
        else c->stat_read_miss ++;
    }
    return outcome;
}

////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
////////////////////////////////////////////////////////////////////

void cache_install(Cache *c, Addr lineaddr, uns is_write, uns core_id){
    
    // Your Code Goes Here
    // Find victim using cache_find_victim
    // Initialize the evicted entry
    // Initialize the victime entry
    Addr tag = lineaddr / c->num_sets;
    Addr index = lineaddr % c->num_sets;
    
    uns j = cache_find_victim(c,index,core_id);
    
    
    
    c->last_evicted_line = c->sets[index].line[j];
    
    if (c->last_evicted_line.dirty && c->last_evicted_line.valid){
        c->stat_dirty_evicts ++;
        
    }
    assert(j < 32);
    c->sets[index].line[j].dirty   = is_write;
    c->sets[index].line[j].tag   = tag;
    c->sets[index].line[j].last_access_time   = cycle;
    c->sets[index].line[j].valid  = 1;
    c->sets[index].line[j].core_id = core_id;
    
}

////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
////////////////////////////////////////////////////////////////////


uns cache_find_victim(Cache *c, uns set_index, uns core_id){
    uns64 count_0 = 0, count_1 = 0;
    uns j = 1;
    uns core = core_id;
    uns64 SWP_CORE1_WAYS = c->num_ways - SWP_CORE0_WAYS;
    Flag found_victim=FALSE;
    uns index = set_index;
    for (uns i=0; i<c->num_ways; i++){
        Cache_Line line = c->sets[index].line[i];
        if(line.valid==FALSE){
            found_victim=TRUE;
            j = i;
            break;
        }
    }
    if(found_victim==FALSE){
        if (c->repl_policy == 0){ //LRU
            for (uns64 i = 0; i < c->num_ways; i++) {
                if(c->sets[index].line[i].last_access_time < c->sets[index].line[j].last_access_time) j = i;
            }
        }
        
        if  (c->repl_policy == 1){ //RAND
            j = rand() % c->num_ways;
        }
        if  (c->repl_policy == 2){
            for (uns i = 0; i<c->num_ways; i++) {
                if(c->sets[index].line[i].core_id == 0 && c->sets[index].line[i].valid){
                    count_0 ++;
                }
                if(c->sets[index].line[i].core_id == 1 && c->sets[index].line[i].valid){
                    count_1 ++;
                }
            }
            if ((count_0 < SWP_CORE0_WAYS) && core_id == 0) core = 1;
            if ((count_1 < SWP_CORE1_WAYS) && core_id == 1) core = 0;
            
            for (uns64 i = 0; i < c->num_ways; i++) {
                if((c->sets[index].line[i].last_access_time < c->sets[index].line[j].last_access_time) && (c->sets[index].line[i].core_id == core)) j = i;
            }
        }
    }

    // TODO: Write this using a switch case statement
    
    return j;
}

