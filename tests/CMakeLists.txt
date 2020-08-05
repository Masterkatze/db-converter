enable_testing()

find_package(GTest REQUIRED)

macro(easy_gtest CPPFILE)
    get_filename_component(filename ${CPPFILE} NAME_WE)
    add_executable(${filename} ${CPPFILE})
    add_gtest(${filename} ${ARGN})
endmacro()

macro(add_gtest TESTNAME)
    target_link_libraries(${TESTNAME} PUBLIC gtest gtest_main ${ARGN})
    target_include_directories(${TESTNAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

    if(GOOGLE_TEST_INDIVIDUAL)
        if(CMAKE_VERSION VERSION_LESS 3.10)
            gtest_add_tests(TARGET ${TESTNAME}
                            TEST_PREFIX "${TESTNAME}."
                            TEST_LIST TmpTestList)
            set_tests_properties(${TmpTestList} PROPERTIES FOLDER "Tests")
        else()
            gtest_discover_tests(${TESTNAME}
                TEST_PREFIX "${TESTNAME}."
                PROPERTIES FOLDER "Tests")
        endif()
    else()
        add_test(${TESTNAME} ${TESTNAME})
        set_target_properties(${TESTNAME} PROPERTIES FOLDER "Tests")
    endif()

    add_dependencies(build_and_test ${TESTNAME})
endmacro()

add_custom_target(build_and_test GTEST_COLOR=1 ${CMAKE_CTEST_COMMAND} -V)

#add_subdirectory(unit)
add_subdirectory(integration)
