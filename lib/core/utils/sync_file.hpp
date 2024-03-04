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

namespace webframe::utils {
/**
 *  @brief   Multi-thread-safe file class
 *  @details This type handle multithreading write requests to a given output
 *stream (inc. files)
 ***********************************************/
class synchronized_file {
public:
  explicit synchronized_file(std::basic_ostream<char> *path) { _path = path; }

  synchronized_file() : _path(nullptr) {}

  synchronized_file &operator=(synchronized_file &&sf) {
    _path = sf._path;
    return *this;
  }

  template <typename T>
  friend synchronized_file &operator<<(synchronized_file &, T);

protected:
  std::ostream *_path;
  mutable std::shared_mutex _writerMutex;
};

class warning_synchronized_file : public synchronized_file {
public:
  explicit warning_synchronized_file(std::basic_ostream<char> *path)
      : synchronized_file(path) {}

  warning_synchronized_file() : synchronized_file() {}

  template <typename T>
  friend warning_synchronized_file &operator<<(warning_synchronized_file &, T);
};
class info_synchronized_file : public synchronized_file {
public:
  explicit info_synchronized_file(std::basic_ostream<char> *path)
      : synchronized_file(path) {}

  info_synchronized_file() : synchronized_file() {}

  template <typename T>
  friend info_synchronized_file &operator<<(info_synchronized_file &, T);
};
class error_synchronized_file : public synchronized_file {
public:
  explicit error_synchronized_file(std::basic_ostream<char> *path)
      : synchronized_file(path) {}

  error_synchronized_file() : synchronized_file() {}

  template <typename T>
  friend error_synchronized_file &operator<<(error_synchronized_file &, T);
};

template <typename T>
synchronized_file &operator<<(synchronized_file &file, T val) {
  if (file._path == nullptr) return file;
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}

template <typename T>
info_synchronized_file &operator<<(info_synchronized_file &file, T val) {
  if (file._path == nullptr) return file;
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}

template <typename T>
warning_synchronized_file &operator<<(warning_synchronized_file &file, T val) {
  if (file._path == nullptr) return file;
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}

template <typename T>
error_synchronized_file &operator<<(error_synchronized_file &file, T val) {
  if (file._path == nullptr) return file;
  const std::lock_guard<std::shared_mutex> locker(file._writerMutex);

  (*file._path) << val;
  (*file._path).flush();

  return file;
}
}