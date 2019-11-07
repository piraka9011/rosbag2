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
  ROSBAG2_LOG_INFO_STREAM("Decompressing " << uri);
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
  size_t length = to_decompress->serialized_data->buffer_length;
  size_t decompress_bound = ZSTD_compressBound(length);

  // Allocate a new buffer for the compressed data
  auto decompressed_buffer = new rcutils_uint8_array_t;
  *decompressed_buffer = rcutils_get_zero_initialized_uint8_array();
  auto allocator = rcutils_get_default_allocator();
  auto ret = rcutils_uint8_array_init(decompressed_buffer, decompress_bound, &allocator);

  // TODO(piraka9011) Figure out why we can't use ROSBAG2_LOG macro.
  if (ret != 0) {
    std::cout << "Unable to initialize an rcutils_uint8_array when decompressing message.";
  }

  ZSTD_decompress(decompressed_buffer->buffer, decompress_bound,
                  to_decompress->serialized_data->buffer, length);

  // Free the uncompressed data
  to_decompress->serialized_data.reset();

  // Create the shared pointer with a deleter
  auto compressed_data = std::shared_ptr<rcutils_uint8_array_t>(
    decompressed_buffer,
    [](rcutils_uint8_array_t *compressed_buffer)
    {
      int error = rcutils_uint8_array_fini(compressed_buffer);
      delete compressed_buffer;
      if (error != RCUTILS_RET_OK) {
        RCUTILS_LOG_ERROR_NAMED("compress_bag_message_data", "Leaking memory %i", error);
      }
    });

  // Fill message with decompressed data
  to_decompress->serialized_data = compressed_data;

  return to_decompress;
}

std::string DecompressorPoC::get_compression_identifier() const
{
  return "TESTING_POC";
}

}  // namespace rosbag2