find_package(doctest CONFIG QUIET)

if(NOT doctest_FOUND)
	include(FetchContent)

	FetchContent_Declare(
		doctest
		URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.2.tar.gz
		URL_HASH SHA256=9189960C2BBBC4F3382CE0773B2BB5F13E3AFD8FED47F55F193E11E85A4F9854
	)

	FetchContent_MakeAvailable(doctest)
endif()

add_executable(small_vector_tests small_vector_tests.cpp)

target_link_libraries(
	small_vector_tests
	PRIVATE
		small_vector::small_vector
		doctest::doctest
)

target_compile_features(small_vector_tests PRIVATE cxx_std_20)

add_test(NAME small_vector_tests COMMAND small_vector_tests)
