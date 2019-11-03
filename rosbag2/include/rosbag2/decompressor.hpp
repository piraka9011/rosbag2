/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ROSBAG2__DECOMPRESSOR_HPP
#define ROSBAG2__DECOMPRESSOR_HPP

#include <iostream>
#include <string>
#include <fstream>

#include <zstd.h>
#include "snappy.h"
#include <cstdint>

#include "rosbag2/types.hpp"
#include "rcutils/logging_macros.h"

namespace rosbag2
{

class Decompressor
{
public:
  virtual std::string decompress_uri(const std::string & uri) = 0;

  virtual std::shared_ptr<SerializedBagMessage> decompress_bag_message_data(
    std::shared_ptr<SerializedBagMessage> & to_decompress) = 0;

  virtual std::string get_compression_identifier() const = 0;

};

} // namespace rosbag2

#endif //ROSBAG2__DECOMPRESSOR_HPP
