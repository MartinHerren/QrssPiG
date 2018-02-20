#include "QGUtils.h"
#include "Config.h"

#include <iomanip>
#include <stdexcept>
#include <string>

void QGUtils::formatFilename(const std::string &tmpl, std::string &str, long int freq, std::chrono::milliseconds frameStart) {
	time_t t = std::chrono::duration_cast<std::chrono::seconds>(frameStart).count();
	std::tm *tm = std::gmtime(&t);
	char s[21];
	std::strftime(s, sizeof(s), "%FT%TZ", tm);

	str = tmpl;
	size_t pos = 0;
	while (std::string::npos != (pos = str.find("%", pos))) {
		std::string sub;
		switch (str[pos + 1]) {
		case 'f':
			sub = std::to_string(freq);
			break;
		case 't':
			sub = std::string(s);
			break;
		case '%':
			sub = "%";
			break;
		default:
			sub = "";
		}

		str.replace(pos, 2, sub);
		pos += sub.length();
	}
}
