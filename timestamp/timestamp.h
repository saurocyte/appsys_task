#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>

class Timestamp {
public:
	inline auto timestamp() {
		time_t now = time(nullptr) ;
		return std::put_time(localtime(&now), "%T");
	}
};

