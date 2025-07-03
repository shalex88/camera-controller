#!/bin/bash

input_file_csv="$1"
output_file_enums_header="$2"
output_file_map_header="$3"

# Include necessary header files
echo "#pragma once" > "$output_file_enums_header"
echo "" >> "$output_file_enums_header"
echo "#include <cstdint>" >> "$output_file_enums_header"
echo "" >> "$output_file_enums_header"

# Generate struct
echo "struct Register {" >> "$output_file_enums_header"
echo "  uint32_t address;" >> "$output_file_enums_header"
echo "  uint32_t default_value;" >> "$output_file_enums_header"
echo "};" >> "$output_file_enums_header"
echo "" >> "$output_file_enums_header"

# Generate enum definitions
echo "enum class REG : uint32_t {" >> "$output_file_enums_header"
awk -F ',' '{print "  "$1","}' "$input_file_csv" >> "$output_file_enums_header"
echo "};" >> "$output_file_enums_header"

# Include necessary header files
echo "#pragma once" > "$output_file_map_header"
echo "" >> "$output_file_map_header"
echo "#include <unordered_map>" >> "$output_file_map_header"
echo "#include \"Registers.h\"" >> "$output_file_map_header"
echo "" >> "$output_file_map_header"


# Generate map initializations
echo "inline std::unordered_map<REG, Register> g_registers_map = {" >> "$output_file_map_header"
while IFS=',' read -r name address value
do
    echo "  {REG::$name, {$address, $value}}," >> "$output_file_map_header"
done < "$input_file_csv"
echo "};" >> "$output_file_map_header"