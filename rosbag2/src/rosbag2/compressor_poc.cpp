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
#include "rcutils/logging_macros.h"

namespace rosbag2
{

/**
 * Compress a file on disk.
 *
 * @param uri input file to compress
 * @return
 */
std::string CompressorPoC::compress_uri(const std::string & uri, int buffer_length)
{

  std::cout << "Compressor:::" << uri << std::endl;

  std::ifstream in(uri);
  std::string compressed_uri = uri_to_compressed_uri(uri);
  std::ofstream out(compressed_uri);

  char * buffer = new char[buffer_length];  // todo could probably only allocate once if the buffer length is a constructor arg

  while (!in.eof()) {
    std::string compressed_output;
    in.read(buffer, buffer_length);

    // todo call abstract implementation method that wraps the specific API
    //todo ZSTD Streaming: https://github.com/facebook/zstd/blob/dev/examples/streaming_compression.c
    snappy::Compress(buffer, in.gcount(), &compressed_output);

    // todo end abstraction
    // if the compression above fails, should throw

    out << compressed_output;
  }

  in.close();
  out.close();

  std::cout << "Compressor::compressed_uri:" << compressed_uri << std::endl;
  delete buffer;
  return compressed_uri;
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
