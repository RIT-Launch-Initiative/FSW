# Storage

This is a basic implementation of the [Zephyr Storage API](https://docs.zephyrproject.org/latest/services/storage/settings/index.html).

You need to enable a bunch of things in the `prj.conf` file. 

```conf
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_FLASH_PAGE_LAYOUT=y
CONFIG_SETTINGS_NVS=y
CONFIG_NVS=y

CONFIG_SETTINGS=y
CONFIG_SETTINGS_RUNTIME=y
```

This enables the settings api and backing for flash storage. 

## Enabling settings partition
The settings API needs a partition of the flash storage configured to store settings

```cpp
&w25q128 {
  partitions {
    storage_partition: partition@0 {
      label = "storage";
      reg = <0x0 DT_SIZE_K(64)>; // Storage size, needs to be at least 64KB i _think_
    };
  };
};

/ {
  chosen {
    zephyr,settings-partition = &storage_partition;
  };
};
```

## Configuring settings
The settings api is configured by creating a `settings_handler`. At this point, I'd say just go look at the code lol
If you want to enable saving everything, it needs to implement the `h_export` function. 
You need to implement `h_set` to load data from the flash (if you're not doing this what're you here for)
The helpful function `settings_runtime_get()` which could get called from anywhere, you need to implement `h_get`, as well as enable `CONFIG_SETTINGS_RUNTIME` in the `prj.conf`
To load settings after everything is done, you can implement `h_commit`. `cprio` is the priority for this function (RTOS stuff I'm guessing)

Then to init, call 
```cpp
settings_subsys_init();
settings_register(&YOUR_CONFIG);
settings_load(); // actually load the data
```

Saving settings is as simple as `settings_save()` if you implemented `h_export` or you can use `settings_save_one`.
Getting the settings is `settings_runtime_get` to copy the value in.
