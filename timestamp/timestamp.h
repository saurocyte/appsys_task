#pragma once
#pragma warning (disable : 4996)

#include <chrono>
#include <ctime>
#include <iomanip>

class Timestamp {
public:
	inline static auto timestamp() {
		time_t now = time(nullptr) ;
		return std::put_time(localtime(&now), "%T");
	}
};
