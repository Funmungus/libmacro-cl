#include "mcr/libmacro.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <thread>

#ifdef _WIN32
#else
#include "mcr/intercept/linux/p_intercept.h"
#endif

/*! Read from /proc/bus/input/devices
 *
 *  Exclude virtual bus 6, used for things like libmacro, "I: Bus=0006"
 *  By default only include kbd handlers
 *  js* not included by default, because of accelerators
 */
#ifndef DEV_FILE
	#define DEV_FILE "/proc/bus/input/devices"
#endif
#ifndef INPUT_DIR
	#define INPUT_DIR "/dev/input"
#endif

static mcr::Libmacro *libmacroPt;

static bool setInterceptList();
static bool setInterceptList(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	libmacroPt = new mcr::Libmacro(true);

	/* Set to true to see all signals (keyboard keys) blocked.  Remember press
	 * Q to exit. */
	mcr_intercept_set_blockable(libmacroPt->ptr(), false);
	/* Detect intercept devices, or set from arguments */
	setInterceptList(argc, argv);

	mcr_intercept_set_enabled(libmacroPt->ptr(), true);
	sleep(3);

	/* Will cause an error if either intercept or Libmacro is still enabled
	 * when Libmacro is deleted. */
	mcr_intercept_set_enabled(libmacroPt->ptr(), false);
	libmacroPt->setEnabled(false);
	delete libmacroPt;
	return 0;
}

static std::regex regexAssignment("^\\s*\\w+[:=]\\s*", std::regex::icase);
static std::regex regexInput("^" INPUT_DIR "/");

static bool setInterceptList(const std::vector<std::string> &list)
{
	std::vector<const char *> dataList;
	for(auto &iter : list) {
		dataList.push_back(iter.c_str());
	}
	return !mcr_intercept_set_grabs(libmacroPt->ptr(), dataList.data(), dataList.size());
}

static bool setInterceptList()
{
#ifdef _WIN32
	return true;
#else
	std::ifstream f(DEV_FILE);
	std::string line;
	std::map<char, std::string> keyMap;
	std::vector<std::string> eventSet;
	if (!f.is_open())
		return false;
	while (std::getline(f, line)) {

	}
	f.close();
	return true;
#endif
}

static bool setInterceptList(int argc, char *argv[])
{
	int i;
	if (argc <= 1)
		return setInterceptList();
#ifdef _WIN32
	return true;
#else
	std::vector<std::string> list;
	for (i = 1; i < argc; i++) {
		if (std::regex_match(argv[i], regexInput)) {
			list.push_back(argv[i]);
		} else {
			list.push_back(std::string(INPUT_DIR "/") + argv[i]);
		}
	}
	return true;
#endif
}
