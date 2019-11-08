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

#include <iostream>

#include "rosbag2/decompressor_poc.hpp"
#include "rosbag2/logging.hpp"


void remove_extension(std::string & filename, int n_times = 1) {
  for (int i = 0; i < n_times; i++) {
    size_t last_dot = filename.find_last_of('.');
    if (last_dot == std::string::npos) return;
    filename.erase(last_dot, filename.size() - 1);
  }
}

namespace rosbag2
{

void DecompressorPoC::uri_to_relative_path(
  const std::string & uri, std::string & new_uri)
{
  // TODO(piraka9011) database extension hardcoded for PoC
  new_uri = uri + ".db3" + ".compressed_poc";
}

std::string DecompressorPoC::decompress_file(const std::string & uri)
{
  ROSBAG2_LOG_DEBUG_STREAM("Decompressing file: " << uri);
  std::ifstream infile(uri);
  std::string compressed_contents;
  std::string decompressed_output;
  std::string new_file_uri = uri;
  if (infile) {
    // Get size and allocate
    infile.seekg(0, std::ios::end);
    compressed_contents.resize(infile.tellg());
    // Go back and read in contents
    infile.seekg(0, std::ios::beg);
    infile.read(&compressed_contents[0], compressed_contents.size());
    // Decompress
    snappy::Uncompress(compressed_contents.c_str(), infile.gcount(), &decompressed_output);
    // Remove .compress extension and write to file.
    remove_extension(new_file_uri);
    std::ofstream outfile;
    outfile.open(new_file_uri);
    outfile << decompressed_output;
    outfile.close();
    infile.close();
    return new_file_uri;
  }
  ROSBAG2_LOG_ERROR_STREAM("Unable to open compressed file.");
  throw std::runtime_error("Unable to open file");
}

std::shared_ptr<SerializedBagMessage> DecompressorPoC::decompress_bag_message_data(
  std::shared_ptr<SerializedBagMessage> & to_decompress)
{
  ROSBAG2_LOG_DEBUG("Decompressing message");
  size_t length = to_decompress->serialized_data->buffer_length;
  uint8_t * buffer = to_decompress->serialized_data->buffer;

  unsigned long long const content_size = ZSTD_getFrameContentSize(buffer, length);
  if (content_size == ZSTD_CONTENTSIZE_ERROR) {
    ROSBAG2_LOG_WARN("Message not compressed with ZSTD.");
  }
  if (content_size == ZSTD_CONTENTSIZE_UNKNOWN) {
    ROSBAG2_LOG_WARN("Original message size unknown.");
  }

  // TODO(piraka9011) Safer cast b/c zstd only returns unsigned long long...
  // (val < 0) ? __SIZE_MAX__ : (size_t)((unsigned)val);
  auto decompress_bound = (size_t)((unsigned)content_size);
  uint8_t * decompressed_buffer = new uint8_t[decompress_bound];

  ZSTD_decompress(decompressed_buffer, decompress_bound,
                  buffer, length);

  // Fill message with decompressed data
  // TODO(piraka9011) Leaking memory :)))) std::copy, memcpy, etc.
  to_decompress->serialized_data->buffer = decompressed_buffer;
  to_decompress->serialized_data->buffer_length = decompress_bound;
  return to_decompress;
}

std::string DecompressorPoC::get_compression_identifier() const
{
  return "TESTING_POC";
}

}  // namespace rosbag2