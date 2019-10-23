// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#ifndef ROSBAG2_SRC_COMPRESSED_BAG_MESSAGE_HPP
#define ROSBAG2_SRC_COMPRESSED_BAG_MESSAGE_HPP

#include <memory>
#include <string>

#include "rcutils/types/uint8_array.h"
#include "rcutils/time.h"

namespace rosbag2_storage
{

    enum CompressionType {
        NONE,
        BZ2,
    };

    // for now assume serialized data
    struct CompressedBagMessage
    {
        std::shared_ptr<rcutils_uint8_array_t> compressed_data; // right now assume data is serialized
        rcutils_time_point_value_t time_stamp;
        std::string topic_name;
        CompressionType compression_type;
    };

}  // namespace rosbag2_storage

#endif  // ROSBAG2_SRC_COMPRESSED_BAG_MESSAGE_HPP

