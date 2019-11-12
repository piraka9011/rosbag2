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

#ifndef ROSBAG2_SRC_COMPRESSOR_HPP
#define ROSBAG2_SRC_COMPRESSOR_HPP


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

// default buffer length used to read a file from disk and compress
constexpr const static int COMPRESSOR_BUFFER_LENGTH_DEFAULT = 4194304 * 2;     // 8 megabytes, todo need to find a sane default

/**
 * Interface used to compress bag files
 */
class Compressor
{
public:
  /**
   * Compress a file on disk.
   *
   * @param uri input file to compress
   * @return
   */
  virtual std::string compress_uri(const std::string & uri) = 0;

  /**
   * Return the uri to use for the compressed file.
   *
   * @param uri original file uri
   * @return the compressed file uri
   */
  virtual std::string uri_to_compressed_uri(const std::string & uri) = 0;

  virtual std::shared_ptr<SerializedBagMessage> compress_bag_message_data(
    std::shared_ptr<SerializedBagMessage> & to_compress) = 0;

  virtual std::string get_compression_identifier() const = 0;
};

}  // namespace rosbag2

#endif //ROSBAG2_SRC_COMPRESSOR_HPP
