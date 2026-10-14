cmake_minimum_required(VERSION 3.21)
project(PressToRagdoll LANGUAGES CXX)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

add_commonlibsse_plugin(PressToRagdoll SOURCES src/ragdoll_plugin.cpp)
target_compile_features(PressToRagdoll PRIVATE cxx_std_23)
target_precompile_headers(PressToRagdoll PRIVATE src/PCH.h)