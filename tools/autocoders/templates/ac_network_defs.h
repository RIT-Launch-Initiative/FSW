/// THIS CODE IS GENERATED BY AN AUTOCODER (DO NOT EDIT)
// Last Generated: {{ date_time }}
// Reference Files: {% for file in files %}
// - {{ file }}
{% endfor %}

#ifndef _AUTOCODER_NETWORK_DEFS_H_
#define _AUTOCODER_NETWORK_DEFS_H_

#include <stdint.h>

#define CREATE_IP_ADDR(base, octet1, octet2) STRINGIFY(base) "." STRINGIFY(octet3) "." STRINGIFY(octet4)

namespace NNetworkDefs {
    // General
    static constexpr uint16_t GENERAL_COMMAND_PORT = {{ general.commandPort }};

    static constexpr uint16_t NOTIFICATION_PORT = {{ general.notificationPort }};
    {% for module_name, module_info in modules.items() %}
    // {{ module_name.capitalize() }} Module
    static constexpr const char* {{ module_name.upper() }}_MODULE_IP_ADDR_BASE = "10.{{ module_info.id }}";

    static constexpr uint16_t {{ module_name.upper() }}_BASE_PORT = {{ module_info.base_port }};
    {% for offset_name, offset_info in module_info.port_offsets.items() %}
    static constexpr uint16_t {{ module_name.upper() }}_MODULE_{{ offset_name.upper()|replace("_", "_") }}_PORT = {{ module_info.base_port + offset_info.port_number }};
    {% endfor %}{% endfor %}
}
#endif
