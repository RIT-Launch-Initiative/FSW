#if !defined(RADIO_MODULE_RECEIVER)

#include <launch_core/dev/gnss.h>
#include "radio_module_functionality.h"


// Callbacks
GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_data_debug_cb);
GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);



int init_lora_unique() {
    return 0;
}

int init_udp_unique() {
    return 0;
}

int start_tasks() {


    return 0;
}

#endif
