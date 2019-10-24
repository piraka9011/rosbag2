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

#include <string>

namespace rosbag2
{

    enum CompressionMode {
        FILE,
        MESSAGE,
    };

    struct CompressionOptions
    {
        std::string compression_format; // does it make sense to have different input vs output?
        CompressionMode mode;
    };

}  // namespace rosbag2

#endif  // ROSBAG2__COMPRESSION_OPTIONS_HPP_
