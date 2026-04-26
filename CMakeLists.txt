cmake_minimum_required(VERSION 4.2)
project(remote_shell)

set(CMAKE_CXX_STANDARD 26)

add_executable(rs src/main.cpp)

enable_testing()

# Integration Test
# We pass the path to the built 'rs' binary so the script knows where it is
add_test(NAME IntegrationTest
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/integration_test.sh ${CMAKE_CURRENT_SOURCE_DIR}/test $<TARGET_FILE:rs>)

#add_test(NAME DockerPull
#        COMMAND docker compose -f ${CMAKE_CURRENT_SOURCE_DIR}/test/compose.test.yml pull)
#set_tests_properties(IntegrationTest PROPERTIES DEPENDS DockerPull)