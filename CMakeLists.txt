cmake_minimum_required(VERSION 3.21)
project(Heroes5GameApi LANGUAGES CXX)
add_library(heroes5_game_api INTERFACE)
target_include_directories(heroes5_game_api INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(heroes5_game_api INTERFACE cxx_std_20)
if(WIN32)
    target_link_libraries(heroes5_game_api INTERFACE bcrypt)
endif()
if(PROJECT_IS_TOP_LEVEL)
    if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "Game API checks require MSVC x86.")
    endif()
    enable_testing()
    add_library(resident_buffer_first SHARED tests/resident_buffer_fixture.cpp)
    add_library(resident_buffer_second SHARED tests/resident_buffer_fixture.cpp)
    target_link_libraries(resident_buffer_first PRIVATE heroes5_game_api user32)
    target_link_libraries(resident_buffer_second PRIVATE heroes5_game_api user32)
    target_compile_definitions(resident_buffer_second PRIVATE RESIDENT_VALUE=202)
    add_executable(game_api_test tests/game_api_test.cpp)
    # Compile the production reader with only native address bindings supplied
    # by the fixture. Fixed game addresses may overlap this process's OS heap.
    set(actor_fixture_source "${CMAKE_CURRENT_SOURCE_DIR}/include/h5/actor_input.hpp")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${actor_fixture_source}")
    file(READ "${actor_fixture_source}" actor_fixture)
    string(REPLACE "\"hooks.hpp\"" "<h5/hooks.hpp>" actor_fixture "${actor_fixture}")
    string(REPLACE "0xe17bcc" "reinterpret_cast<uint32_t>(committedArmyMethods)" actor_fixture "${actor_fixture}")
    string(REPLACE "0xe79d3c" "reinterpret_cast<uint32_t>(committedWorldMethods)" actor_fixture "${actor_fixture}")
    string(REPLACE "0x89fbf0" "reinterpret_cast<uintptr_t>(CommittedDefinitionGetter)" actor_fixture "${actor_fixture}")
    string(REPLACE "0xa6d490" "reinterpret_cast<uintptr_t>(CommittedActiveActor)" actor_fixture "${actor_fixture}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/actor_input_fixture.hpp" "${actor_fixture}")
    # Substitute only external memory reads for deterministic mutation admission
    # tests. The production parsing, FP wrapper and operation control flow run
    # unchanged; these fixtures refuse before any fixed native call is reached.
    set(native_ui_source "${CMAKE_CURRENT_SOURCE_DIR}/include/h5/native_ui.hpp")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${native_ui_source}")
    file(READ "${native_ui_source}" native_ui_fixture)
    set(native_ui_read [=[inline bool ReadLocal(uint32_t address, void* output, size_t bytes) {
    __try { std::memcpy(output, reinterpret_cast<const void*>(address), bytes); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}]=])
    string(FIND "${native_ui_fixture}" "${native_ui_read}" native_ui_read_at)
    if(native_ui_read_at EQUAL -1)
        message(FATAL_ERROR "Native UI memory-read fixture seam changed.")
    endif()
    string(REPLACE "${native_ui_read}" [=[inline bool ReadLocal(uint32_t address, void* output, size_t bytes) {
    return ::ReadNativeUiFixture(address, output, bytes);
}]=] native_ui_fixture "${native_ui_fixture}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/native_ui_fixture.hpp" "${native_ui_fixture}")
    target_include_directories(game_api_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(game_api_test PRIVATE heroes5_game_api)
    target_compile_definitions(game_api_test PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    add_test(NAME game_api_boundaries COMMAND game_api_test
        $<TARGET_FILE:resident_buffer_first> $<TARGET_FILE:resident_buffer_second>)
endif()
