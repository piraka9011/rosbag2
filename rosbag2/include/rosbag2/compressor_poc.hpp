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

#ifndef ROSBAG2_SRC_COMPRESSORPOC_HPP
#define ROSBAG2_SRC_COMPRESSORPOC_HPP


#include "rosbag2/compressor.hpp"

#include <iostream>
#include <string>
#include <fstream>

#include <zstd.h>
#include "snappy.h"
#include <cstdint>

#include "rosbag2/types.hpp"


namespace rosbag2
{

class CompressorPoC : Compressor
{
public:
  CompressorPoC() = default;
  virtual ~CompressorPoC() = default;

  std::string compress_uri(const std::string & uri) override;

  std::string uri_to_compressed_uri(const std::string & uri) override;

  std::shared_ptr<SerializedBagMessage> compress_bag_message_data(
    std::shared_ptr<SerializedBagMessage> & to_compress) override;

  std::string get_compression_identifier() const override;

};

}  // namespace rosbag2

#endif //ROSBAG2_SRC_COMPRESSORPOC_HPP
