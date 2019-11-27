#include "mcr/libmacro.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <thread>

#ifdef _WIN32
#else
	#include <linux/input.h>
	#include <signal.h>
	#include <unistd.h>

	#include "mcr/intercept/linux/p_intercept.h"
#endif

/*! Read from /proc/bus/input/devices
 *
 *  Exclude virtual bus 6, used for things like libmacro, "I: Bus=0006"
 *  By default only include kbd handlers
 *  js* and mouse* not included by default.  They cause problems.
 */
#ifndef DEV_FILE
	#define DEV_FILE "/proc/bus/input/devices"
#endif
#ifndef INPUT_DIR
	#define INPUT_DIR "/dev/input"
#endif

#ifdef _WIN32
#else
	const static int endKey = KEY_Q;
#endif

static mcr::Libmacro *libmacroPt = nullptr;
/* Can be improved with a condition variable */
static bool endProgram = false;

static bool exists(const char *filename)
{
	return !std::ifstream(filename).fail();
}
static bool exists(const std::string &filename)
{
	return !std::ifstream(filename).fail();
}
static bool setInterceptList();
static bool setInterceptList(int argc, char *argv[]);

/* Return value:  If blocking is enabled and return true, the signal will not
 * be received by anything else (including X11 or Wayland). */
static bool receive(void *receiver, struct mcr_Signal * dispatchSignal,
					unsigned int mods);

#ifndef _WIN32
static void sig_handler(int)
{
	endProgram = true;
}
#endif

int main(int argc, char *argv[])
{
#ifndef _WIN32
	int i;
	for (i = SIGHUP; i <= SIGTERM; i++) {
		signal(i, sig_handler);
	}
#endif

	std::cout <<
			  "Libmacro intercept sample.  To end test, press Q from intercepted keyboard, or kill with Ctrl+C."
			  << std::endl;

	libmacroPt = new mcr::Libmacro(true);

	/* Listen for signals. There will be a C++ wrapper for this. */
	mcr_Dispatcher_add_generic(libmacroPt->ptr(), nullptr, nullptr, receive);
	/* The following to only listen to key signals. */
//	mcr_Signal siggy;
//	mcr_Signal_init(&siggy);
//	siggy.isignal = libmacroPt->iKey();
//	mcr_Dispatcher_add(libmacroPt->ptr(), &siggy, nullptr, receive);

	/* Set to true and all signals (keyboard keys) will be blocked.
	 * Remember press Q to exit. */
	mcr_intercept_set_blockable(libmacroPt->ptr(), false);
	/* Detect intercept devices, or set from arguments */
	setInterceptList(argc, argv);

	mcr_intercept_set_enabled(libmacroPt->ptr(), true);

	while (!endProgram) {
		sleep(2);
	}

	/* Will cause an error if either intercept or Libmacro is still enabled
	 * when Libmacro is deleted. */
	mcr_intercept_set_enabled(libmacroPt->ptr(), false);
	libmacroPt->setEnabled(false);
	delete libmacroPt;
	return 0;
}

static std::regex regexMouseJoy("^js\\|^mouse", std::regex::icase);
static std::regex regexEvent("^event", std::regex::icase);
static std::regex regexHandler("^\\s*H:\\s*HANDLER[A-Z]*=", std::regex::icase);
static std::regex regexBusVirtual("^\\s*I:\\s*BUS=0*6\\s", std::regex::icase);

static bool setInterceptList(const std::set<std::string> &list)
{
	std::cout << "Set intercept list: ";
	std::vector<const char *> dataList;
	for(auto &iter : list) {
		std::cout << iter << ", ";
		dataList.push_back(iter.c_str());
	}
	std::cout << std::endl;
	return !mcr_intercept_set_grabs(libmacroPt->ptr(), dataList.data(),
									dataList.size());
}

static void lineHandler(const std::string &line,
						std::set<std::string> &eventSet)
{
	static bool useDeviceFlag = false;
	/* New device line, we might use this device. */
	if (line.empty()) {
		useDeviceFlag = true;
		return;
	}
	/* I: BUS=0006 is a virtual device.  Ignore virtual devices. */
	if (std::regex_search(line, regexBusVirtual)) {
		useDeviceFlag = false;
		return;
	}
	/* H: HANDLERS=, this is the goods. */
	if (std::regex_search(line, regexHandler)) {
		std::set<std::string> handlerSet;
		/* Remove handlers=, everything else are the events */
		std::string eventLine = std::regex_replace(line, regexHandler, "");
		std::string str;
		std::istringstream ss(eventLine);
		while(ss) {
			ss >> str;
			/* Auto-detected mice and joysticks may cause instability
			 * with your whole computer. */
			if (std::regex_search(str, regexMouseJoy))
				return;
			if (handlerSet.find(str) == handlerSet.end())
				handlerSet.insert(str);
		}
		/* Has a keyboard, push all event files to our list. */
		if (handlerSet.find("kbd") != handlerSet.end()) {
			for (auto &iter : handlerSet) {
				if (eventSet.find(iter) == eventSet.end())
					eventSet.insert(iter);
			}
		}
	}
}

static bool setInterceptList()
{
#ifdef _WIN32
	return true;
#else
	std::ifstream f(DEV_FILE);
	std::string line;
	std::set<std::string> eventSet, fileSet;
	if (!f.is_open() || f.fail())
		return false;
	/* DEV_FILE file reports empty file and getline may not work? */
	while (f.peek() != EOF) {
		std::getline(f, line);
		lineHandler(line, eventSet);
	}
	if (eventSet.empty())
		return false;
	for (auto &iter : eventSet) {
		if (std::regex_search(iter, regexEvent)) {
			fileSet.insert(std::string(INPUT_DIR "/") + iter);
		}
	}
	setInterceptList(fileSet);
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
	std::set<std::string> list;
	for (i = 1; i < argc; i++) {
		if (exists(argv[i])) {
			list.insert(argv[i]);
		} else if (exists(std::string(INPUT_DIR "/") + argv[i])) {
			list.insert(std::string(INPUT_DIR "/") + argv[i]);
		}
	}
	setInterceptList(list);
	return true;
#endif
}

static bool receive(void *, struct mcr_Signal * dispatchSignal,
					unsigned int mods)
{
	if (!dispatchSignal) {
		std::cerr << "Oddly we have dispatched a null signal." << std::endl;
		return false;
	}
	std::cout << "Receiving signal of type: " << mcr_ISignal_name(libmacroPt->ptr(),
			  dispatchSignal->isignal) << std::endl;
	std::cout << "Current modifiers are " << mods << '.' << std::endl;
	/* Found a key, print values */
	if (dispatchSignal->isignal == libmacroPt->iKey()) {
		mcr_Key *keyPt = mcr_Key_data(dispatchSignal);
		if (keyPt) {
			std::cout << "Key: " << keyPt->key << ":" << mcr_Key_name(libmacroPt->ptr(),
					  keyPt->key) << std::endl;
			std::cout << "Apply: " << keyPt->apply << ":";
			switch (keyPt->apply) {
			case MCR_SET:
				std::cout << "MCR_SET" << std::endl;
				break;
			case MCR_UNSET:
				std::cout << "MCR_UNSET" << std::endl;
				break;
			case MCR_BOTH:
				std::cout << "MCR_BOTH" << std::endl;
				break;
			case MCR_TOGGLE:
				std::cout << "MCR_TOGGLE" << std::endl;
				break;
			}
			if (keyPt->key == endKey) {
				std::cout << "Ending key has been found.  Closing program." << std::endl;
				endProgram = true;
			}
		} else {
			std::cout << "Oops, key instance has no data." << std::endl;
		}
	}
	return true;
}
