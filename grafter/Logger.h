//===--- Logger.h ---------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#ifndef _LOGGER
#define _LOGGER

#include <fstream>
#include <iostream>
#include <string>

#define _LOGGER_FILE_MODE 1
#define _LOGGER_STDOUT_MODE 2
#define _LOGGER_HYBRID_MODE 3
#define _LOGGER_OFF_MODE 4

using namespace std;
/* This class is logger!
 */
class Logger {
private:
  string header;
  int mode;
  ofstream file;
  static Logger *staticLogger;

public:
  Logger(int mode_, string fileName_, string header = "");
  void log(string s);
  bool logError(string s);
  void logInfo(string s);
  void logDebug(string s);
  void logWarn(string s);

  static Logger &getStaticLogger();
  ~Logger();
};

#endif
