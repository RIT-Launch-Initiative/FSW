# Enable strict compiler warnings for production builds
# This file adds -Wall and other recommended warning flags

# Add warning flags for C and C++ compilation
zephyr_compile_options(-Wall)

# Optionally add more warning flags
# Uncomment these if you want even stricter checks:
# zephyr_compile_options(-Wextra)
# zephyr_compile_options(-Wpedantic)
