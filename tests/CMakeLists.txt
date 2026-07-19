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

add_executable(test_controller test_controller.cpp)
target_link_libraries(test_controller PRIVATE n64core)
add_test(NAME test_controller COMMAND test_controller)

add_executable(test_sprite_scene test_sprite_scene.cpp)
target_link_libraries(test_sprite_scene PRIVATE n64core)
add_test(NAME test_sprite_scene COMMAND test_sprite_scene)

add_executable(test_rsp_hle test_rsp_hle.cpp)
target_link_libraries(test_rsp_hle PRIVATE n64core)
add_test(NAME test_rsp_hle COMMAND test_rsp_hle)
