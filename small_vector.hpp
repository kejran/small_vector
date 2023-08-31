#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>

template <typename T, typename I, std::size_t Size>
class small_vector {

public:

	static constexpr auto align = static_cast<std::align_val_t>(alignof(T));

	using iterator = T *;
	using const_iterator = T const *;

	small_vector() = default;
	small_vector(small_vector<T, I, Size> const &other) {
		reserve(other.count_);
		count_ = other.count_;
		std::uninitialized_copy_n(other.data(), count_, data());
	}
	small_vector(small_vector<T, I, Size> &&other) {
		if (other.allocated_())
			allocation_ = other.allocation_;
		else
			std::uninitialized_move_n(other.sb_(), other.count_, sb_());

		std::swap(count_, other.count_);
		std::swap(capacity_, other.capacity_);
	}
	small_vector(std::initializer_list<T> list) {
		reserve(list.size());
		count_ = list.size();
		std::uninitialized_copy(list.begin(), list.end(), data());
	}
	small_vector(std::span<T const> span) {
		reserve(span.size());
		count_ = span.size();
		std::uninitialized_copy(span.begin(), span.end(), data());
	}

	~small_vector() {
		clear();
		if (allocated_())
			::operator delete(allocation_, align);
	}

	small_vector &operator=(small_vector const &other) {
		reserve(other.count_);
		count_ = other.count_;
		std::uninitialized_copy_n(other.data(), count_, data());
		return *this;
	}
	small_vector &operator=(small_vector &&other) {
		if (other.allocated_()) { // we can just move the pointer
			if (!allocated_()) // we have some sbo data, remove it
				clear();

			std::swap(allocation_, other.allocation_);
			std::swap(count_, other.count_);
			std::swap(capacity_, other.capacity_);
			return *this;
		}
		else { // need to move sbo 
			if (allocated_()) // we have some allocated data, clear it
				clear();
			std::uninitialized_move_n(other.sb_(), other.count_, sb_());
			count_ = other.count_;
			capacity_ = other.capacity_;
			return *this;
		}
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
		clear();
		for (auto it = first; it != last; ++it)
			push_back(*it);
	}

	void clear() {
		if (count_) {
			auto it = data();
			std::destroy(it, it + count_);
			count_ = 0;
		}
	}

	void reserve(std::size_t newSize) {
		if (newSize < capacity_)
			return;
		// this only happens when allocating over the sbo size
		auto newAlloc = reinterpret_cast<T *>(::operator new(newSize * sizeof(T), align));
		if (count_) {
			auto it = data();
			std::uninitialized_move_n(it, count_, newAlloc);
			std::destroy(it, it + count_);
		}
		if (allocated_())
			::operator delete(allocation_, align);
		allocation_ = newAlloc;
		capacity_ = newSize;
	}

	void push_back(T const &other) {
		if (full_())
			reserve(capacity_ * 2);
		new(data() + count_++) T(other);
	}

	void push_back(T &&other) {
		if (full_())
			reserve(capacity_ * 2);
		new(data() + count_++) T(std::move(other)); // consider: no move, o is already an rvalue?
	}

	template <typename... Args>
	void emplace_back(Args &&... args) {
		if (full_())
			reserve(capacity_ * 2);
		new(data() + count_++) T(std::forward<Args>(args)...);
	}

	void resize(std::size_t count, T const &value = T()) {
		if (count < count_) {
			auto b = data();
			auto e = b + count_;
			std::destroy(b + count, e);
		}
		if (count > count_) {
			reserve(count);
			auto beg = data();
			for (auto i = count_; i < count; ++i)
				new(beg + i) T(value);
		}
		count_ = count;
	}

private:

	bool full_() const { return capacity_ == count_; }
	bool allocated_() const { return capacity_ != Size; }

	T *sb_() { return reinterpret_cast<T *>(buffer_); }
	T const *sb_() const { return reinterpret_cast<T const *>(buffer_); }

	union {
		alignas(T) std::byte buffer_[sizeof(T) * Size];
		T *allocation_;
	};
	I capacity_ = Size;
	I count_ = 0;
};
