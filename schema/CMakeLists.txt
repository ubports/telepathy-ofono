file(GLOB SCHEMA_FILES ${CMAKE_CURRENT_SOURCE_DIR}/v*.sql)

set(SCHEMA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/schema.sql)
set(VERSION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/version.info)

add_custom_command(
    OUTPUT ${SCHEMA_FILE} ${VERSION_FILE}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/update_schema.sh ${CMAKE_CURRENT_SOURCE_DIR} ${SCHEMA_FILE} ${VERSION_FILE}
    WORKING DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${SCHEMA_FILES}
    )

add_custom_target(schema_update DEPENDS ${SCHEMA_FILE} ${VERSION_FILE})

# just to get the v*sql files to show on QtCreator
add_custom_target(schema_files_target ALL SOURCES ${SCHEMA_FILES})
