#pragma once
#include <cstdint>
/* 
 *  Call once to initialize the storage subsystem
 *  Intended for use in SYS_INIT
 */
extern "C" int storage_init();

namespace NStorage {
enum StorageCommand {
    // normal flight packet, write it to storage ASAP
    FlightPacket,

    // mark that main is done modifying preflight data (flight has begun). storage handler can begin accessing it with the assumption that it doesn't change
    PreflightDataDone,

    // erase specified partitions
    ErasePartitionA,
    ErasePartitionB,
    ErasePartitionC,
    ErasePartitionD,

};

uint32_t GetBootcount();


}