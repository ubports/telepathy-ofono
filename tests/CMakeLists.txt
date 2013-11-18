include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    )

macro(generate_test TESTNAME USE_DBUS)
    add_executable(${TESTNAME} ${ARGN} ${TESTNAME}.cpp)
    qt5_use_modules(${TESTNAME} Contacts Core DBus Qml Test)
    set(TEST_COMMAND )
    if (${USE_DBUS})
        set(TEST_COMMAND -p ${CMAKE_CURRENT_BINARY_DIR}/${TESTNAME} -p -xunitxml -p -o -p ${CMAKE_BINARY_DIR}/test_${TESTNAME}.xml)
        add_test(${TESTNAME} ${DBUS_RUNNER} --task ${CMAKE_CURRENT_BINARY_DIR}/dbus-test-wrapper.sh ${TEST_COMMAND})
    else (${USE_DBUS})
        add_test(${TESTNAME} ${CMAKE_CURRENT_BINARY_DIR}/${TESTNAME} -xunitxml -o ${CMAKE_BINARY_DIR}/test_${TESTNAME}.xml)
    endif(${USE_DBUS})
endmacro(generate_test)

configure_file(dbus-test-wrapper.sh.in ${CMAKE_CURRENT_BINARY_DIR}/dbus-test-wrapper.sh)

generate_test(PhoneUtilsTest False ${CMAKE_SOURCE_DIR}/phoneutils.cpp)

if (DBUS_RUNNER)
    generate_test(DummyTest True)
endif(DBUS_RUNNER)

add_subdirectory(mock)
