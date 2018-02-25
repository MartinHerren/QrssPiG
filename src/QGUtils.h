#pragma once

#include <chrono>
#include <string>

class QGUtils {
public:
	static void formatFilename(const std::string &tmpl, std::string &str, long int freq, std::chrono::milliseconds frameStart);
};
