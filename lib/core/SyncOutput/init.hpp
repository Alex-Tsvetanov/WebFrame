/**
 *  @file   file.hpp
 *  @brief  Multi-thread-safe file structure
 *  @author Alex Tsvetanov
 *  @date   2022-03-07
 ***********************************************/

#pragma once

#include <cstring>
#include <iomanip>
#include <shared_mutex>
#include <string>

/**
 *  @brief   Multi-thread-safe file class
 *  @details This type handle multithreading write requests to a given output
 *stream (inc. files)
 ***********************************************/
class SynchronizedFile {
public:
  explicit SynchronizedFile(std::basic_ostream<char> *path) { _path = path; }

  SynchronizedFile() : _path(nullptr) {}

  SynchronizedFile &operator=(SynchronizedFile &&sf) {
    _path = sf._path;
    return *this;
  }

  template <typename T>
  friend SynchronizedFile &operator<<(SynchronizedFile &, T);

protected:
  std::ostream *_path;
  mutable std::shared_mutex _writerMutex;
};

class WarningSynchronizedFile : public SynchronizedFile {
public:
  explicit WarningSynchronizedFile(std::basic_ostream<char> *path)
      : SynchronizedFile(path) {}

  WarningSynchronizedFile() : SynchronizedFile() {}

  template <typename T>
  friend WarningSynchronizedFile &operator<<(WarningSynchronizedFile &, T);
};
class InfoSynchronizedFile : public SynchronizedFile {
public:
  explicit InfoSynchronizedFile(std::basic_ostream<char> *path)
      : SynchronizedFile(path) {}

  InfoSynchronizedFile() : SynchronizedFile() {}

  template <typename T>
  friend InfoSynchronizedFile &operator<<(InfoSynchronizedFile &, T);
};
class ErrorSynchronizedFile : public SynchronizedFile {
public:
  explicit ErrorSynchronizedFile(std::basic_ostream<char> *path)
      : SynchronizedFile(path) {}

  ErrorSynchronizedFile() : SynchronizedFile() {}

  template <typename T>
  friend ErrorSynchronizedFile &operator<<(ErrorSynchronizedFile &, T);
};

template <typename T>
SynchronizedFile &operator<<(SynchronizedFile &file, T val) {
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}

template <typename T>
InfoSynchronizedFile &operator<<(InfoSynchronizedFile &file, T val) {
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}

template <typename T>
WarningSynchronizedFile &operator<<(WarningSynchronizedFile &file, T val) {
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}

template <typename T>
ErrorSynchronizedFile &operator<<(ErrorSynchronizedFile &file, T val) {
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}
