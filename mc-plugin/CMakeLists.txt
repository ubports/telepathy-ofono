set(MCP_ACCOUNT_MANAGER_NAME mcp-account-manager-ofono)

set(MC_PLUGIN_SRC
    mcp-account-manager-ofono.c
    mission-control-plugin.c
)

set(MC_PLUGIN_HDRS
    mcp-account-manager-ofono.h
)

execute_process(COMMAND pkg-config --variable=plugindir mission-control-plugins
     OUTPUT_VARIABLE MC_PLUGINS_DIR)
string(REPLACE "\n" "" MC_PLUGINS_DIR ${MC_PLUGINS_DIR})

include_directories(${MC_PLUGINS_INCLUDE_DIRS})
add_library(${MCP_ACCOUNT_MANAGER_NAME} SHARED ${MC_PLUGIN_SRC} ${MC_PLUGIN_HDRS})
set_target_properties(${MCP_ACCOUNT_MANAGER_NAME} PROPERTIES PREFIX "")
target_link_libraries(${MCP_ACCOUNT_MANAGER_NAME} ${MC_PLUGINS_LIBRARIES})

install(TARGETS ${MCP_ACCOUNT_MANAGER_NAME} DESTINATION ${MC_PLUGINS_DIR})
