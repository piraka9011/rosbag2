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
#include "snappy.h"
#include <string>
#include <fstream>

namespace rosbag2 {

// default buffer length used to read a file from disk and compress
const int COMPRESSOR_BUFFER_LENGTH_DEFAULT = 4194304*2; // 4 megabytes, todo need to find a sane default

struct CompressionState {
    bool compression_success;
    std::string compressed_uri;
};

/**
 * Class used to compress bag files
 */
class Compressor
{
public:
    
    Compressor() = default;
    virtual ~Compressor() = default;

    /**
     * Compress a file on disk.
     *
     * @param uri input file to compress
     * @return
     */
    virtual CompressionState compress_uri(std::string uri, int buffer_length = COMPRESSOR_BUFFER_LENGTH_DEFAULT) {

      std::cout << "Compressor::compress_uri:" << uri << std::endl;
      CompressionState compression_state;
      try {

        std::ifstream in(uri);
        std::string compressed_uri = uri_to_compressed_uri(uri);
        std::ofstream out(compressed_uri);

        //todo check input file size
        // if less then the buffer length then use that

        std::string compressed_file = uri_to_compressed_uri(uri);
        compression_state.compressed_uri = compressed_file;

        char * buffer = new char [buffer_length];

        while(!in.eof()) {
          std::string compressed_output;
          in.read(buffer, buffer_length);

          // todo call abstract implementation method that wraps the specific API
          snappy::Compress(buffer, in.gcount(), &compressed_output);

          out << compressed_output;
        }

        in.close();// todo delete in from disk? if successfully compressed?
        out.close();
        compression_state.compression_success = true;

      } catch (...) {
        compression_state.compression_success = false;
      }

      std::cout << "Compressor::compression_success:" << compression_state.compression_success  << std::endl;
      std::cout << "Compressor::compressed_uri:" << compression_state.compressed_uri  << std::endl;

      return compression_state;
    }

    // todo should be abstract
    /**
     * Return the uri to use for the compressed file.
     *
     * @param uri original file uri
     * @return the compressed file uri
     */
    virtual std::string uri_to_compressed_uri(std::string uri) {
      return uri + ".snappy";
    }

    //todo compress individual messages
    //virtual std::string compress_bag_message_data(SerializedBagMessage to_compress){}
};

}  // namespace rosbag2

#endif //ROSBAG2_SRC_COMPRESSOR_HPP
