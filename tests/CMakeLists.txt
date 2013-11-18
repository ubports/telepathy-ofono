include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    )

macro(generate_tests)
    foreach(test ${ARGN})
        add_executable(${test} ${test}.cpp)
        qt5_use_modules(${test} Core DBus Test)
        add_test(${test} ${CMAKE_CURRENT_BINARY_DIR}/${test} -xunitxml -o ${CMAKE_BINARY_DIR}/test_${test}.xml)
        set(TEST_ENVIRONMENT "LD_PRELOAD=${CMAKE_CURRENT_BINARY_DIR}/libofono-qt.so")
        set_tests_properties(${test} PROPERTIES
                             ENVIRONMENT ${TEST_ENVIRONMENT})
    endforeach(test)
endmacro(generate_tests)

generate_tests(
    DummyTest
    )

add_subdirectory(mock)
