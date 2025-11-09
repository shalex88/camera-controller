# GenTL Example Application

add_executable(gentl_example gentl_example.cpp)

target_include_directories(gentl_example
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(gentl_example
    PRIVATE
        FpgaCXPProducer
)

# Optional: Install example
install(TARGETS gentl_example
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)
