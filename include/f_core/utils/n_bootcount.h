#ifndef N_BOOTCOUNT_H
#define N_BOOTCOUNT_H

namespace NBootCount {
    /**
     * Get the current boot count from persistent storage
     * @return The current boot count, or -1 on error
     */
    int GetBootCount();

    /**
     * Increment the boot count in persistent storage
     * @return 0 on success, negative error code on failure
     */
    int IncrementBootCount();
} // namespace NBootCount

#endif //N_BOOTCOUNT_H