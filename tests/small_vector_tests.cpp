#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <small_vector.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("small_vector starts empty with inline capacity") {
	small_vector<int, std::uint8_t, 4> values;

	CHECK(values.size() == 0);
	CHECK(values.capacity() == 4);
	CHECK(values.begin() == values.end());
}

TEST_CASE("small_vector appends within inline storage") {
	small_vector<int, std::uint8_t, 4> values;

	values.push_back(1);
	values.push_back(2);
	values.emplace_back(3);

	CHECK(values.size() == 3);
	CHECK(values.capacity() == 4);
	CHECK(values[0] == 1);
	CHECK(values[1] == 2);
	CHECK(values[2] == 3);
}

TEST_CASE("small_vector grows when inline capacity is exceeded") {
	small_vector<int, std::uint8_t, 2> values;

	values.push_back(10);
	values.push_back(20);
	values.push_back(30);

	CHECK(values.size() == 3);
	CHECK(values.capacity() >= 3);
	std::vector<int> expected{10, 20, 30};
	CHECK(std::vector<int>(values.begin(), values.end()) == expected);
}

TEST_CASE("small_vector preserves values when copied and moved") {
	small_vector<std::string, std::uint8_t, 2> original;
	original.emplace_back("red");
	original.emplace_back("green");
	original.emplace_back("blue");

	auto copy = original;
	small_vector<std::string, std::uint8_t, 2> moved = std::move(original);
	std::vector<std::string> expected{"red", "green", "blue"};

	CHECK(std::vector<std::string>(copy.begin(), copy.end()) == expected);
	CHECK(std::vector<std::string>(moved.begin(), moved.end()) == expected);
	CHECK(original.size() == 0);
}

TEST_CASE("small_vector supports assignment and resize") {
	int raw[] = {1, 2, 3, 4};
	small_vector<int, std::uint8_t, 2> values(std::span<int const>(raw, 4));

	values.resize(6, 9);
	std::vector<int> grown{1, 2, 3, 4, 9, 9};
	CHECK(std::vector<int>(values.begin(), values.end()) == grown);

	values.resize(2);
	std::vector<int> shrunk{1, 2};
	CHECK(std::vector<int>(values.begin(), values.end()) == shrunk);

	std::vector<int> replacement{7, 8, 9};
	values.assign(replacement.begin(), replacement.end());
	CHECK(std::vector<int>(values.begin(), values.end()) == replacement);
}
