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

#include "rosbag2/decompressor_poc.hpp"

namespace rosbag2
{

std::string DecompressorPoC::decompress_uri(const std::string & uri, int buffer_length){
  return uri;
}

std::shared_ptr<SerializedBagMessage> DecompressorPoC::decompress_bag_message_data(
  std::shared_ptr<SerializedBagMessage> & to_decompress)
{
  return std::make_shared<SerializedBagMessage>(
    to_decompress->serialized_data,
    to_decompress->time_stamp,
    to_decompress->topic_name);
}

std::string DecompressorPoC::get_compression_identifier() const
{
  return "";
}

}  // namespace rosbag2