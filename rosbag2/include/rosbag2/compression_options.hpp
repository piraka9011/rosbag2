// Copyright 2018, Bosch Software Innovations GmbH.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ROSBAG2__COMPRESSION_OPTIONS_HPP_
#define ROSBAG2__COMPRESSION_OPTIONS_HPP_

#include <map>
#include <string>

template <class T1, class T2>
std::map<T2, T1> swap_key_value(std::map<T1, T2> m) {
  std::map<T2, T1> m1;
  for (auto&& item : m) {
    m1.emplace(item.second, item.first);
  }
  return m1;
}

namespace rosbag2
{

enum CompressionMode
{
  NONE,        // sane default
  FILE,
  MESSAGE,
};

// TODO(piraka9011) static?
const std::map<CompressionMode, std::string> CompressionModeToStringMap =
  {
    {CompressionMode::NONE, "NONE"},
    {CompressionMode::FILE, "FILE"},
    {CompressionMode::MESSAGE, "MESSAGE"}
  };

const std::map<std::string, CompressionMode> StringToCompressionModeMap =
  swap_key_value(CompressionModeToStringMap);

struct CompressionOptions
{
  std::string compression_format;
  CompressionMode mode;
};

}  // namespace rosbag2

#endif  // ROSBAG2__COMPRESSION_OPTIONS_HPP_
