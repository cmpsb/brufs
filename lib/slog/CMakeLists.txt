set(SOURCE_FILES Logger.cpp)

add_library(slog ${SOURCE_FILES})

target_include_directories(slog PUBLIC ${CMAKE_CURRENT_LIST_DIR})
