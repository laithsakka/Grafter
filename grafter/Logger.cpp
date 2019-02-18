//===--- Logger.cpp -------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//

#include "Logger.h"

#include <ctime>
#include <iostream>
Logger *Logger::staticLogger = nullptr;

Logger::Logger(int mode_, string fileName_, string header_) {
  this->mode = mode_;
  this->header = header_;
  // if (mode_ != _LOGGER_OFF_MODE) {
  // //  this->file.open(fileName_.c_str(), std::ofstream::out | std::ofstream::app);
  // //  this->log("Logger Initialized");
  // }
}

Logger::~Logger() {
  // if (this->mode != _LOGGER_OFF_MODE)
  //   if (this->file.is_open())
  //     this->file.close();
}

void Logger::log(string s) {

  if (this->mode == _LOGGER_FILE_MODE || this->mode == _LOGGER_HYBRID_MODE)
    this->file << this->header + ":"
               << "\t" << s << endl;

  if (this->mode == _LOGGER_STDOUT_MODE || this->mode == _LOGGER_HYBRID_MODE)
    cout << this->header << " :\t" << s << endl;
}

bool Logger::logError(string s) {

  if (this->mode == _LOGGER_FILE_MODE || this->mode == _LOGGER_HYBRID_MODE)
    this->file << "Error :" << this->header + ":"
               << "\t" << s << endl;

  if (this->mode == _LOGGER_STDOUT_MODE || this->mode == _LOGGER_HYBRID_MODE)
    cout << "Error :" << this->header << " :\t" << s << endl;
  
  return  false; 
}
void Logger::logInfo(string s) {
  if (this->mode == _LOGGER_FILE_MODE || this->mode == _LOGGER_HYBRID_MODE)
    this->file << "Info :" << this->header + ":"
               << "\t" << s << endl;

  if (this->mode == _LOGGER_STDOUT_MODE || this->mode == _LOGGER_HYBRID_MODE)
    cout << "Info :" << this->header << " :\t" << s << endl;
}

void Logger::logDebug(string s) {

  if (this->mode == _LOGGER_FILE_MODE || this->mode == _LOGGER_HYBRID_MODE)
    this->file << "Debug :" << this->header + ":"
               << "\t" << s << endl;

  if (this->mode == _LOGGER_STDOUT_MODE || this->mode == _LOGGER_HYBRID_MODE)
    cout << this->header << "Debug :"
         << " :\t" << s << endl;
}
void Logger::logWarn(string s) {

  if (this->mode == _LOGGER_FILE_MODE || this->mode == _LOGGER_HYBRID_MODE)
    this->file << "Warn :" << this->header + ":"
               << "\t" << s << endl;

  if (this->mode == _LOGGER_STDOUT_MODE || this->mode == _LOGGER_HYBRID_MODE)
    cout << this->header << "Warn :"
         << " :\t" << s << endl;
}

Logger &Logger::getStaticLogger() {
  if (staticLogger == nullptr) {
    std::time_t result = std::time(nullptr);

    string timeSlot = std::ctime(&result);
    staticLogger =
        new Logger(_LOGGER_STDOUT_MODE, ("./T_" + timeSlot + ".txt"));
  }

  return *staticLogger;
}
