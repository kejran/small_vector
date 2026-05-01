cmake_minimum_required(VERSION 3.20)

project(
	small_vector
	VERSION 0.1.0
	LANGUAGES CXX
)

add_library(small_vector INTERFACE)
add_library(small_vector::small_vector ALIAS small_vector)

target_compile_features(small_vector INTERFACE cxx_std_20)

target_include_directories(
	small_vector
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

# not entirely sure about this one
if(MSVC)
	target_sources(
		small_vector
		INTERFACE
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/small_vector.natvis>
	)
endif()

include(CTest)

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()
