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


static void* malloc_orDie(size_t size)
{
  void* const buff = malloc(size);
  if (buff) return buff;
  /* error */
  perror("malloc");
  exit(8);  // Temp error code
}

std::string remove_extension(const std::string & filename)
{
  size_t last_dot = filename.find_last_of(".");
  if (last_dot == std::string::npos) return filename;
  return filename.substr(0, last_dot);
}

namespace rosbag2
{

std::string DecompressorPoC::decompress_uri(const std::string & uri)
{
  ROSBAG2_LOG_INFO_STREAM("Decompressing " << uri);
  std::ifstream infile(uri);
  std::string compressed_contents;
  std::string decompressed_output;
  std::string new_file_uri;
  if (infile)
  {
    // Get size and allocate
    ROSBAG2_LOG_INFO_STREAM("Resizing compressed content holder.");
    infile.seekg(0, std::ios::end);
    compressed_contents.resize(infile.tellg());
    // Go back and read in contents
    ROSBAG2_LOG_INFO("Reading in contents to compressed_contents.");
    infile.seekg(0, std::ios::beg);
    infile.read(&compressed_contents[0], compressed_contents.size());
    // Decompress
    ROSBAG2_LOG_INFO("Decompressing...");
    snappy::Uncompress(compressed_contents.c_str(), infile.gcount(), &decompressed_output);
    // Remove .compress extension and write to file.
    new_file_uri = remove_extension(uri);
    ROSBAG2_LOG_INFO_STREAM("New File URI: " << new_file_uri);
    std::ofstream outfile;
    ROSBAG2_LOG_INFO("Writing to file.");
    outfile.open(new_file_uri);
    outfile << decompressed_output;
    outfile.close();
    infile.close();
    return new_file_uri;
  }
  else
  {
    ROSBAG2_LOG_ERROR_STREAM("Unable to open compressed file.");
    throw std::runtime_error("Unable to open file");
  }
}

std::shared_ptr<SerializedBagMessage> DecompressorPoC::decompress_bag_message_data(
  std::shared_ptr<SerializedBagMessage> & to_decompress)
{
  /// Extracted from https://github.com/facebook/zstd/blob/dev/examples/streaming_decompression.c
  ZSTD_DCtx* const d_context = ZSTD_createDCtx();  // Decompression context
  //ZSTD_DStream * d_stream = ZSTD_createDStream();

  /* Guarantee to successfully flush at least one complete compressed block in all circumstances. */
  size_t const buff_out_size = ZSTD_DStreamOutSize();
  void*  const buff_out = malloc_orDie(buff_out_size);

  ZSTD_inBuffer input = { to_decompress->serialized_data->buffer,
                          to_decompress->serialized_data->buffer_length, 0 };

//  size_t lastRet = 0;
  std::make_shared<SerializedBagMessage>();
  while (input.pos < input.size){
    ZSTD_outBuffer output = {buff_out, buff_out_size, 0};
    size_t const ret = ZSTD_decompressStream(d_context, &output , &input);
    if (ZSTD_isError(ret)) {
      std::cout << "Got a ZSTD Error: \n" << ZSTD_getErrorName(ret) << std::endl;
    }
//    lastRet = ret;
  }

  return std::make_shared<SerializedBagMessage>();
}

std::string DecompressorPoC::get_compression_identifier() const
{
  return "TESTING_POC";
}

}  // namespace rosbag2