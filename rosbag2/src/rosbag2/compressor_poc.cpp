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

#include <chrono>
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
  ROSBAG2_LOG_INFO("----- File Compression Results ----");
  auto start = std::chrono::high_resolution_clock::now();
  std::string decompressed_buffer;
  std::string compressed_buffer;
  std::string compressed_uri = uri_to_compressed_uri(uri);
  std::ifstream infile(uri);
  ROSBAG2_LOG_INFO_STREAM("Compressing " << uri);
  if (infile.is_open()) {
    // Get size and allocate
    infile.seekg(0, std::ios::end);
    size_t decompressed_buffer_length = infile.tellg();
    decompressed_buffer.resize(decompressed_buffer_length);
    ROSBAG2_LOG_INFO_STREAM("Size Before: " << decompressed_buffer_length << " B");
    // Go back and read in contents
    infile.seekg(0, std::ios::beg);
    infile.read(&decompressed_buffer[0], decompressed_buffer_length);
    ROSBAG2_LOG_INFO("Loaded decompressed data.");
    // TODO(dabonnie) call abstract implementation method that wraps the specific API
    // TODO(dabonnie) ZSTD Streaming:
    //  https://github.com/facebook/zstd/blob/dev/examples/streaming_compression.c
//    size_t compressed_size = snappy::Compress(decompressed_buffer.c_str(), decompressed_buffer_length,
//                                              &compressed_buffer);
    size_t const compressed_buffer_length = ZSTD_compressBound(decompressed_buffer.size());
    compressed_buffer.resize(compressed_buffer_length);
    ROSBAG2_LOG_INFO_STREAM("Compressed buffer length: " << compressed_buffer_length);
    size_t const compressed_size = ZSTD_compress(&compressed_buffer, compressed_buffer.size(),
      &decompressed_buffer[0], decompressed_buffer.size(), 1);
    ROSBAG2_LOG_INFO_STREAM("Compressed size: " << compressed_size);
    if (ZSTD_isError(compressed_size)) {
      std::stringstream err;
      err << "ZSTD Error: " << ZSTD_getErrorName(compressed_size);
      throw std::runtime_error(err.str());
    }
    ROSBAG2_LOG_INFO_STREAM("Size After: " << compressed_size << " B");
    auto compression_ratio = (float)decompressed_buffer_length / (float)compressed_size;
    ROSBAG2_LOG_INFO_STREAM("Compression ratio: " << compression_ratio);
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
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    ROSBAG2_LOG_INFO_STREAM("Compression took " << duration.count() << " microseconds");
    ROSBAG2_LOG_INFO("-----------------------------------");
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
  std::shared_ptr<SerializedBagMessage> & decompressed_message)
{
  auto start = std::chrono::high_resolution_clock::now();
  size_t decompressed_buffer_length = decompressed_message->serialized_data->buffer_length;
  uint8_t * decompressed_buffer = decompressed_message->serialized_data->buffer;
  size_t compressed_length = ZSTD_compressBound(decompressed_buffer_length);
  uint8_t * compressed_buffer = new uint8_t[compressed_length];

  size_t compressed_size = ZSTD_compress(compressed_buffer, compressed_length,
                                         decompressed_buffer, decompressed_buffer_length, 1);
  if (ZSTD_isError(compressed_size)) {
    ROSBAG2_LOG_WARN("Unable to compress message. Not compressing.");
    return decompressed_message;
  }
  auto compression_ratio = (float)decompressed_buffer_length / (float)compressed_size;
  ROSBAG2_LOG_INFO_STREAM("Message size before: " << decompressed_buffer_length << " B");
  ROSBAG2_LOG_INFO_STREAM("Message size after: " << compressed_size << " B");
  ROSBAG2_LOG_INFO_STREAM("Compression ratio: " << compression_ratio);

  // TODO(piraka9011) Leaking memory :))))
  decompressed_message->serialized_data->buffer = compressed_buffer;
  decompressed_message->serialized_data->buffer_length = compressed_length;
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  ROSBAG2_LOG_INFO_STREAM("Message compression took " << duration.count() << " microseconds");
  return decompressed_message;
}

std::string CompressorPoC::get_compression_identifier() const
{
  return "TESTING_POC";
}
}
