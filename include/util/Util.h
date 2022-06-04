#pragma once

#include <chrono>
#include <random>
#include <stdexcept>
#include <vector>

#define CHECKGL do { if (auto err = glGetError()) { std::cerr << "\e[31mError at " << __FILE__ << ':' << __LINE__ << ": " << gluErrorString(err) << "\e[39m\n"; } } while(0);

namespace Game3 {
	extern std::default_random_engine utilRNG;

	long parseLong(const std::string &, int base = 10);
	long parseLong(const char *, int base = 10);
	long parseLong(std::string_view, int base = 10);
	unsigned long parseUlong(const std::string &, int base = 10);
	unsigned long parseUlong(const char *, int base = 10);
	unsigned long parseUlong(std::string_view, int base = 10);

	inline std::chrono::system_clock::time_point getTime() {
		return std::chrono::system_clock::now();
	}

	template <typename T = std::chrono::nanoseconds>
	inline T timeDifference(std::chrono::system_clock::time_point old_time) {
		return std::chrono::duration_cast<T>(getTime() - old_time);
	}

	static inline std::default_random_engine::result_type getRandom(std::default_random_engine::result_type seed = 0) {
		if (seed == 0)
			return utilRNG();
		std::default_random_engine rng;
		rng.seed(seed);
		return rng();
	}

	template <typename T, typename R = std::default_random_engine>
	void shuffle(T &container, typename R::result_type seed = 0) {
		if (seed == 0) {
			std::shuffle(container.begin(), container.end(), utilRNG);
		} else {
			R rng;
			rng.seed(seed);
			std::shuffle(container.begin(), container.end(), rng);
		}
	}

	template <typename T>
	T::value_type & choose(T &container, typename std::default_random_engine::result_type seed = 0) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return container.at(getRandom(seed) % container.size());
	}

	template <typename T>
	const T::value_type & choose(const T &container, typename std::default_random_engine::result_type seed = 0) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return container.at(getRandom(seed) % container.size());
	}

	template <typename T, typename R>
	T::value_type & choose(T &container, R &rng) {
		if (container.empty())
			throw std::invalid_argument("Container is empty");
		return container.at(rng() % container.size());
	}
}