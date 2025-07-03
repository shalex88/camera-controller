set(GENERATED_OUT_DIR ${CMAKE_BINARY_DIR}/registers_map)
file(MAKE_DIRECTORY ${GENERATED_OUT_DIR})

set(GENERATOR ${CMAKE_CURRENT_LIST_DIR}/generate_registers_map.sh)
set(CSV ${CMAKE_CURRENT_LIST_DIR}/Registers.csv)
set(ENUMS_HEADER ${GENERATED_OUT_DIR}/Registers.h)
set(MAP_HEADER ${GENERATED_OUT_DIR}/RegistersMap.h)

# Specify the custom command to generate Registers.h and RegistersMap.h
add_custom_command(
        OUTPUT ${ENUMS_HEADER} ${MAP_HEADER}
        COMMAND bash ${GENERATOR} ${CSV} ${ENUMS_HEADER} ${MAP_HEADER}
        DEPENDS ${GENERATOR} ${CSV}
)

add_custom_target(
        generate_registers_map_headers ALL
        DEPENDS ${ENUMS_HEADER} ${MAP_HEADER}
)

add_library(registers INTERFACE)

add_dependencies(registers generate_registers_map_headers)


target_include_directories(registers
        INTERFACE
        ${GENERATED_OUT_DIR}
)