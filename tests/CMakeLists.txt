include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    )

macro(generate_test TESTNAME USE_DBUS)
    add_executable(${TESTNAME} ${ARGN} ${TESTNAME}.cpp)
    qt5_use_modules(${TESTNAME} Core DBus Test)
    set(TEST_COMMAND )
    if (${USE_DBUS})
        set(TEST_COMMAND -p ${CMAKE_CURRENT_BINARY_DIR}/${TESTNAME} -p -xunitxml -p -o -p ${CMAKE_BINARY_DIR}/test_${TESTNAME}.xml)
        add_test(${TESTNAME} ${DBUS_RUNNER} --keep-env --task ${CMAKE_CURRENT_BINARY_DIR}/dbus-test-wrapper.sh ${TEST_COMMAND})
    else (${USE_DBUS})
        add_test(${TESTNAME} ${CMAKE_CURRENT_BINARY_DIR}/${TESTNAME} -xunitxml -o ${CMAKE_BINARY_DIR}/test_${TESTNAME}.xml)
    endif(${USE_DBUS})
    # force telepathy not to use system approvers when available,
    # also force usage of memory backend in history-service
    set(TMPDIR "/tmp/tpofono_test_home")
    set(TEST_ENVIRONMENT "HOME=${TMPDIR};
                          HISTORY_SQLITE_DBPATH=:memory:;
                          XDG_CONFIG_HOME=${TMPDIR};
                          XDG_DATA_HOME=${TMPDIR};
                          XDG_CACHE_DIR=${TMPDIR};
                          XDG_CACHE_HOME=${TMPDIR};
                          XDG_DATA_DIRS=${TMPDIR};
                          MC_ACCOUNT_DIR=${TMPDIR};
                          MC_MANAGER_DIR=${TMPDIR}")
    set_tests_properties(${TESTNAME} PROPERTIES
                          ENVIRONMENT "${TEST_ENVIRONMENT}"
                          TIMEOUT 30)
    target_link_libraries(${TESTNAME}
                          ${TP_QT5_LIBRARIES}
                         )
endmacro(generate_test)

configure_file(dbus-test-wrapper.sh.in ${CMAKE_CURRENT_BINARY_DIR}/dbus-test-wrapper.sh)

generate_test(PhoneUtilsTest False ${CMAKE_SOURCE_DIR}/phoneutils.cpp)

if (DBUS_RUNNER)
    generate_test(ConnectionTest True telepathyhelper.cpp ofonomockcontroller.cpp)
    generate_test(MessagesTest True telepathyhelper.cpp ofonomockcontroller.cpp handler.cpp approvertext.cpp)
    generate_test(CallTest True telepathyhelper.cpp ofonomockcontroller.cpp handler.cpp approvercall.cpp)
endif(DBUS_RUNNER)

add_subdirectory(mock)
