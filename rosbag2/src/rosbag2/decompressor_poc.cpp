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

#include <iotream>
#include "rosbag2/decompressor_poc.hpp"

static void* malloc_orDie(size_t size)
{
  void* const buff = malloc(size);
  if (buff) return buff;
  /* error */
  perror("malloc");
  exit(8);  // Temp error code
}

namespace rosbag2
{

std::string DecompressorPoC::decompress_uri(const std::string & uri, int buffer_length){
  return uri;
}

std::shared_ptr<SerializedBagMessage> DecompressorPoC::decompress_bag_message_data(
  std::shared_ptr<SerializedBagMessage> & to_decompress)
{
  /// Extracted from https://github.com/facebook/zstd/blob/dev/examples/streaming_decompression.c
  ZSTD_DCtx* const d_context = ZSTD_createDCtx();  // Decompression context
  ZSTD_DStream * d_stream = ZSTD_createDStream();

  /* Guarantee to successfully flush at least one complete compressed block in all circumstances. */
  size_t const buff_out_size = ZSTD_DStreamOutSize();
  void*  const buff_out = malloc_orDie(buff_out_size);

  ZSTD_inBuffer input = { to_decompress->serialized_data->buffer,
                          to_decompress->serialized_data->buffer_length, 0 };

  size_t lastRet = 0;
  std::make_shared<SerializedBagMessage>();
  while (input.pos < input.size){
    ZSTD_outBuffer output = {buff_out, buff_out_size, 0};
    size_t const ret = ZSTD_decompressStream(d_context, &output , &input);
    if (ZSTD_isError(ret)) {
      std::cout << "Got a ZSTD Error: \n" << ZSTD_getErrorName(ret) << std::endl;
    }
    lastRet = ret;
  }

  return std::make_shared<SerializedBagMessage>(
    to_decompress->serialized_data,
    to_decompress->time_stamp,
    to_decompress->topic_name);
}

std::string DecompressorPoC::get_compression_identifier() const
{
  return "TESTING_POC";
}

}  // namespace rosbag2