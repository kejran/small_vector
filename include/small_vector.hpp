#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>

template <typename T, typename I, std::size_t Size>
class small_vector {

public:

	static_assert(Size > 0, "small_vector requires a non-zero SBO size");
	static_assert(Size <= static_cast<std::size_t>((std::numeric_limits<I>::max)()), "SBO size must fit in the index type");

	static constexpr auto align = static_cast<std::align_val_t>(alignof(T));

	using iterator = T *;
	using const_iterator = T const *;

	small_vector() = default;
	small_vector(small_vector<T, I, Size> const &other) {
		copy_from_(other.data(), other.count_);
	}
	small_vector(small_vector<T, I, Size> &&other) {
		if (other.allocated_()) {
			allocation_ = other.allocation_;
			count_ = other.count_;
			capacity_ = other.capacity_;
			other.count_ = 0;
			other.capacity_ = Size;
		}
		else {
			move_from_sbo_(other);
		}
	}
	small_vector(std::initializer_list<T> list) {
		copy_from_(list.begin(), list.size());
	}
	small_vector(std::span<T const> span) {
		copy_from_(span.data(), span.size());
	}

	~small_vector() {
		clear();
		if (allocated_())
			::operator delete(allocation_, align);
	}

	small_vector &operator=(small_vector const &other) {
		if (this == &other)
			return *this;
		clear();
		copy_from_(other.data(), other.count_);
		return *this;
	}
	small_vector &operator=(small_vector &&other) {
		if (this == &other)
			return *this;

		clear();
		release_allocation_();

		if (other.allocated_()) {
			allocation_ = other.allocation_;
			count_ = other.count_;
			capacity_ = other.capacity_;
			other.count_ = 0;
			other.capacity_ = Size;
		}
		else {
			move_from_sbo_(other);
		}
		return *this;
	}

	T *data() { return allocated_() ? allocation_ : sb_(); }

	T const *data() const { return allocated_() ? allocation_ : sb_(); }

	std::size_t size() const { return count_; }
	std::size_t capacity() const { return capacity_; }

	iterator begin() { return data(); }
	const_iterator begin() const { return data(); }
	const_iterator cbegin() const { return data(); }

	iterator end() { return data() + count_; }
	const_iterator end() const { return data() + count_; }
	const_iterator cend() const { return data() + count_; }

	T &operator[](std::size_t index) { return data()[index]; }
	T const &operator[](std::size_t index) const { return data()[index]; }
	
	template <class It>
	void assign(It first, It last) {
		small_vector tmp;
		for (auto it = first; it != last; ++it)
			tmp.push_back(*it);
		*this = std::move(tmp);
	}

	void clear() {
		if (count_) {
			auto it = data();
			std::destroy(it, it + count_);
			count_ = 0;
		}
	}

	void reserve(std::size_t newSize) {
		ensure_size_(newSize);
		if (newSize <= capacity_)
			return;

		auto oldData = data();
		auto oldCount = count_;
		auto oldAllocated = allocated_();
		auto newAlloc = reinterpret_cast<T *>(::operator new(newSize * sizeof(T), align));

		auto constructed = newAlloc;
		try {
			for (std::size_t i = 0; i < oldCount; ++i, ++constructed)
				std::construct_at(constructed, std::move_if_noexcept(oldData[i]));
		}
		catch (...) {
			std::destroy(newAlloc, constructed);
			::operator delete(newAlloc, align);
			throw;
		}

		std::destroy(oldData, oldData + oldCount);
		if (oldAllocated)
			::operator delete(oldData, align);

		allocation_ = newAlloc;
		capacity_ = newSize;
	}

	void push_back(T const &other) {
		if (full_() && contains_(std::addressof(other))) {
			T tmp(other);
			reserve(next_capacity_());
			std::construct_at(data() + count_, std::move(tmp));
		}
		else {
			if (full_())
				reserve(next_capacity_());
			std::construct_at(data() + count_, other);
		}
		++count_;
	}

	void push_back(T &&other) {
		if (full_() && contains_(std::addressof(other))) {
			T tmp(std::move(other));
			reserve(next_capacity_());
			std::construct_at(data() + count_, std::move(tmp));
		}
		else {
			if (full_())
				reserve(next_capacity_());
			std::construct_at(data() + count_, std::move(other));
		}
		++count_;
	}

	template <typename... Args>
	void emplace_back(Args &&... args) {
		if (full_())
			reserve(next_capacity_());
		std::construct_at(data() + count_, std::forward<Args>(args)...);
		++count_;
	}

	void resize(std::size_t count, T const &value = T()) {
		if (count < count_) {
			auto b = data();
			auto e = b + count_;
			std::destroy(b + count, e);
			count_ = count;
		}
		if (count > count_) {
			if (contains_(std::addressof(value))) {
				T tmp(value);
				reserve(count);
				append_fill_(count, tmp);
			}
			else {
				reserve(count);
				append_fill_(count, value);
			}
		}
	}

private:

	bool full_() const { return capacity_ == count_; }
	bool allocated_() const { return capacity_ != Size; }

	T *sb_() { return reinterpret_cast<T *>(buffer_); }
	T const *sb_() const { return reinterpret_cast<T const *>(buffer_); }

	bool contains_(T const *ptr) const {
		auto first = data();
		auto last = first + count_;
		auto less = std::less<T const *>{};
		return !less(ptr, first) && less(ptr, last);
	}

	std::size_t next_capacity_() const {
		auto current = static_cast<std::size_t>(capacity_);
		auto max = max_size_();
		if (current == max)
			throw std::length_error("small_vector capacity overflow");
		if (current > max / 2)
			return max;
		return current * 2;
	}

	static constexpr std::size_t max_size_() {
		return static_cast<std::size_t>((std::numeric_limits<I>::max)());
	}

	static void ensure_size_(std::size_t count) {
		if (count > max_size_())
			throw std::length_error("small_vector capacity overflow");
	}

	void release_allocation_() {
		if (allocated_()) {
			::operator delete(allocation_, align);
			capacity_ = Size;
		}
	}

	void copy_from_(T const *src, std::size_t count) {
		reserve(count);
		try {
			std::uninitialized_copy_n(src, count, data());
			count_ = count;
		}
		catch (...) {
			release_allocation_();
			throw;
		}
	}

	void move_from_sbo_(small_vector &other) {
		auto count = other.count_;
		auto constructed = sb_();
		try {
			for (std::size_t i = 0; i < count; ++i, ++constructed)
				std::construct_at(constructed, std::move(other.sb_()[i]));
		}
		catch (...) {
			std::destroy(sb_(), constructed);
			throw;
		}
		count_ = count;
		std::destroy(other.sb_(), other.sb_() + other.count_);
		other.count_ = 0;
		other.capacity_ = Size;
	}

	void append_fill_(std::size_t count, T const &value) {
		while (count_ < count) {
			std::construct_at(data() + count_, value);
			++count_;
		}
	}

	union {
		alignas(T) std::byte buffer_[sizeof(T) * Size];
		T *allocation_;
	};
	I capacity_ = Size;
	I count_ = 0;
};
