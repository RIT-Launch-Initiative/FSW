// entry point for storage thread (will in the future be setup to handle more data)
int storage_thread_entry(void *, void *, void *);

struct Partitions {
    const struct device *superfast_dev;
    const struct device *superslow_dev;
    const struct device *superyev_dev;
};