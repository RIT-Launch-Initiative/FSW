#pragma once
#include "common.hpp"

#include <cstdint>
/* 
 *  Call once to initialize the storage subsystem
 *  Intended for use in SYS_INIT
 */
extern "C" int storage_init();

namespace NStorage {
uint32_t GetBootcount();

using EraseProgressFn = void (*)(uint32_t index, uint32_t total);

void EraseParameters();
void EraseData(EraseProgressFn callback);

bool HasStoredFlight();

/**
 * Write parameters to the parameters partition
 * This has the effect of 'closing' the flash so no boots after this will 
 * @param params[in] the parameters to write
 * @return 0 if succesfful, error code if not
 */
int WriteParameters(Parameters *params);

/**
 * Read out the stored parameters
 * @param params[out] value of parameters read. May get overwritten even if non-zero exit code is returned
 * @return 0 if successful, -1 if magic doesnt match, errcode otherwise
 */
int ReadStoredParameters(Parameters *params);

int WritePreboostPacket(uint32_t index, Packet *packet);

int ReadStoredSinglePacket(size_t index, Packet *packet);
int WriteFlightPacket(uint32_t index, Packet *params);

/**
 * Return address in flash of the parameter partition
 * @return address on flash
 */
uint32_t GetParamPartitionAddress();
/**
 * Return address in flash of the data partition
 * @return address on flash
 */
uint32_t GetDataPartitionAddress();

/**
 * Return size of the parameter partition
 * @return size in bytes
 */
uint32_t GetParamPartitionSize();
/**
 * Return size of the data partition
 * @return size in bytes
 */
uint32_t GetDataPartitionSize();

} // namespace NStorage
