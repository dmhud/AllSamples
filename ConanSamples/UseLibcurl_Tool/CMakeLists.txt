cmake_minimum_required (VERSION 3.21.1)
project(mycurl CXX)

find_package(CURL CONFIG REQUIRED)

add_executable(mycurl main.cpp)
target_link_libraries(mycurl CURL::libcurl)

set_target_properties(mycurl PROPERTIES LINK_FLAGS "/ignore:4099")