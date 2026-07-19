add_executable(test_rdram test_rdram.cpp)
target_link_libraries(test_rdram PRIVATE n64core)
add_test(NAME test_rdram COMMAND test_rdram)

add_executable(test_vr4300 test_vr4300.cpp)
target_link_libraries(test_vr4300 PRIVATE n64core)
add_test(NAME test_vr4300 COMMAND test_vr4300)

add_executable(test_frame_timing test_frame_timing.cpp)
target_link_libraries(test_frame_timing PRIVATE n64core)
add_test(NAME test_frame_timing COMMAND test_frame_timing)

add_executable(test_vi test_vi.cpp)
target_link_libraries(test_vi PRIVATE n64core)
add_test(NAME test_vi COMMAND test_vi)
