cmake_minimum_required(VERSION 3.10)
project(MFTIndexer LANGUAGES CXX)

add_library(MFTIndexer SHARED
    MFTIndexer/MFTIndexer.cpp
)
if (MINGW)
    target_link_options(MFTIndexer PRIVATE -static -static-libstdc++ -static-libgcc)
endif()

target_include_directories(MFTIndexer PUBLIC
    ${PROJECT_SOURCE_DIR}/MFTIndexer
)

add_executable(MFTIndexerCli
    MFTIndexer/main.cpp
)

target_link_libraries(MFTIndexerCli PRIVATE MFTIndexer)
if (MINGW)
    target_link_options(MFTIndexerCli PRIVATE -municode -static -static-libstdc++ -static-libgcc)
endif()

set_target_properties(MFTIndexer PROPERTIES
    OUTPUT_NAME "MFTIndexer"
)

set_target_properties(MFTIndexerCli PROPERTIES
    OUTPUT_NAME "MFTIndexer"
)

