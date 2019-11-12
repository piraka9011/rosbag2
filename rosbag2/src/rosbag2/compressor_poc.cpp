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

#include "rosbag2/compressor_poc.hpp"

#include <iostream>
#include <string>
#include <fstream>

#include <zstd.h>
#include "snappy.h"
#include <cstdint>

#include "rosbag2/types.hpp"
#include "rosbag2/logging.hpp"

namespace rosbag2
{

/**
 * Compress a file on disk.
 *
 * @param uri Relative path to the input file to compress
 * @return The compressed file relative path.
 */
std::string CompressorPoC::compress_uri(const std::string & uri)
{
  std::string decompressed_buffer;
  std::string compressed_buffer;
  std::string compressed_uri = uri_to_compressed_uri(uri);
  std::ifstream infile(uri);
  ROSBAG2_LOG_INFO_STREAM("Compressing " << uri << " to " << compressed_uri);
  if (infile.is_open()) {
    // Get size and allocate
    infile.seekg(0, std::ios::end);
    size_t decompressed_buffer_length = infile.tellg();
    decompressed_buffer.resize(decompressed_buffer_length);
    // Go back and read in contents
    infile.seekg(0, std::ios::beg);
    infile.read(&decompressed_buffer[0], decompressed_buffer.size());
    // TODO(dabonnie) call abstract implementation method that wraps the specific API
    // TODO(dabonnie) ZSTD Streaming:
    //  https://github.com/facebook/zstd/blob/dev/examples/streaming_compression.c
    size_t result = snappy::Compress(decompressed_buffer.c_str(), decompressed_buffer_length,
                                     &compressed_buffer);
    ROSBAG2_LOG_INFO_STREAM("Size Before: " << decompressed_buffer_length << " B" <<
                                            "\n\t\t   Size After: " << result << " B");
    // If the compression above fails, should throw
    /// End abstraction
    infile.close();
    std::ofstream outfile(compressed_uri);
    if (!outfile.is_open()) {
      std::stringstream err;
      err << "Unable to open " << compressed_uri;
      throw std::runtime_error(err.str());
    }
    outfile << compressed_buffer;
    outfile.close();
    return compressed_uri;
  }
  std::stringstream err;
  err << "Unable to open " << uri;
  throw std::runtime_error(err.str());
}

/**
 * Return the uri to use for the compressed file.
 *
 * @param uri original file uri
 * @return the compressed file uri
 */
std::string CompressorPoC::uri_to_compressed_uri(const std::string & uri)
{
  return uri + ".compressed_poc";
}

std::shared_ptr<SerializedBagMessage> CompressorPoC::compress_bag_message_data(
  std::shared_ptr<SerializedBagMessage> & to_compress)
{

  size_t length = to_compress->serialized_data->buffer_length;
  size_t compress_bound = ZSTD_compressBound(length);
  uint8_t * compressed_data = new uint8_t[compress_bound];

  size_t compressed_size = ZSTD_compress(compressed_data, compress_bound,
    to_compress->serialized_data->buffer, length, 1);

  std::cout << "compress_bag_message_data: original length=" << length <<
    ", compressed length=" << compressed_size << ", ratio=" <<
  (float)length / (float)compressed_size << std::endl;

  // TODO(piraka9011) Leaking memory :))))
  to_compress->serialized_data->buffer = compressed_data;
  to_compress->serialized_data->buffer_length = compress_bound;

  return to_compress;
}

std::string CompressorPoC::get_compression_identifier() const
{
  return "TESTING_POC";
}
}
