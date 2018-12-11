set(SOURCE_FILES opt.c)

add_library(slopt ${SOURCE_FILES})

target_include_directories(slopt PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(slopt PRIVATE "-fexceptions")
