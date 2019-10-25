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

#include "rcutils/logging_macros.h"


namespace rosbag2 {

// default buffer length used to read a file from disk and compress
    const int COMPRESSOR_BUFFER_LENGTH_DEFAULT = 4194304 * 2; // 4 megabytes, todo need to find a sane default

/**
 * Class used to compress bag files
 */

    class Compressor {
    public:

        Compressor() = default;
        virtual ~Compressor() = default;

        /**
         * Compress a file on disk.
         *
         * @param uri input file to compress
         * @return
         */
        virtual std::string compress_uri(std::string uri, int buffer_length = COMPRESSOR_BUFFER_LENGTH_DEFAULT) {

          std::cout << "Compressor::compress_uri:" << uri << std::endl;

          std::ifstream in(uri);
          std::string compressed_uri = uri_to_compressed_uri(uri);
          std::ofstream out(compressed_uri);

          char *buffer = new char[buffer_length];

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

          return compressed_uri;
        }

        // todo should be abstract
        /**
         * Return the uri to use for the compressed file.
         *
         * @param uri original file uri
         * @return the compressed file uri
         */
        virtual std::string uri_to_compressed_uri(std::string uri) {
          return uri + ".compressed_poc";
        }

        //todo interface
        virtual std::shared_ptr<SerializedBagMessage> compress_bag_message_data(std::shared_ptr<SerializedBagMessage> to_compress) {

          size_t length = to_compress->serialized_data->buffer_length;
          size_t compress_bound = ZSTD_compressBound(length);

          // allocate a new buffer for the compressed data
          auto compressed_buffer = new rcutils_uint8_array_t;
          *compressed_buffer = rcutils_get_zero_initialized_uint8_array();
          auto allocator = rcutils_get_default_allocator();
          auto ret = rcutils_uint8_array_init(compressed_buffer, compress_bound, &allocator);

          if(ret !=0 ) {
            std::cout << "compress_bag_message_data: rcutils_uint8_array_init error:" << ret << std::endl;
          }

          size_t compressed_size = ZSTD_compress(compressed_buffer->buffer,
                  compress_bound, to_compress->serialized_data->buffer, length, 1);

          std::cout << "compress_bag_message_data: original length=" << length << ", compressed length=" << compressed_size << ", ratio="
                    << (float)length / (float)compressed_size << std::endl;

          // free the uncompressed data
          to_compress->serialized_data.reset();

          // create the shared pointer with a deleter
          auto compressed_data = std::shared_ptr<rcutils_uint8_array_t>(compressed_buffer,
              [](rcutils_uint8_array_t * compressed_buffer) {
                int error = rcutils_uint8_array_fini(compressed_buffer);
                delete compressed_buffer;
                if (error != RCUTILS_RET_OK) {
                  RCUTILS_LOG_ERROR_NAMED(
                    "compress_bag_message_data", "Leaking memory %i", error);
                }
              });

          // update the bag with the compressed data
          to_compress->serialized_data = compressed_data;
          return to_compress;
        }

        //todo interface
        virtual std::string get_compression_identifier() {
          return "TESTING_POC";
        }
    };

}  // namespace rosbag2

#endif //ROSBAG2_SRC_COMPRESSOR_HPP
