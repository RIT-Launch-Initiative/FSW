#include "n_storage.hpp"

extern "C" int storage_init(){
    return 0;
}

int handle_bootcount(){
    // read bootcount sectors, calculate bootcount, writeback bootcount
    return 0;
}

// read the first block of each partition and check if they have been filled in
int find_unused_partition(){
    return 0;
}