// Copyright 2018, Bosch Software Innovations GmbH.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rosbag2/sequential_reader.hpp"
#include "rosbag2/decompressor_poc.hpp"

#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "snappy.h"

#include "rosbag2/compression_options.hpp"
#include "rosbag2/info.hpp"
#include "rosbag2/logging.hpp"

namespace
{
std::string remove_extension(const std::string & filename) {
  size_t last_dot = filename.find_last_of('.');
  if (last_dot == std::string::npos) return filename;
  return filename.substr(0, last_dot);
}

//void remove_extension(std::string & filename) {
//  size_t last_dot = filename.find_last_of('.');
//  if (last_dot == std::string::npos) return;
//  filename.erase(last_dot, filename.size()-1);
//}

void clean_path(
  const std::string & uri,
  std::string & dir_name, // Directory name ex. rosbag2_2019_10_2
  std::string & new_uri,  // URI for files ex. rosbag2_2019_10_2/rosbag2_2019_10_2 (_1,_2,_3 etc.)
  std::string & relative_path  // Full DB path ex. rosbag2_2019_10_2/rosbag2_2019_10_2.db.compressed
  )
{
  if (uri.back() == '/') {
    dir_name = uri.substr(0, uri.size() - 1);
    new_uri = uri + dir_name;
    relative_path = new_uri + ".db3" + ".compressed_poc";
  }
  else {
    dir_name = uri;
    new_uri = dir_name + "/" + dir_name;
    relative_path = new_uri +  ".db3" + ".compressed_poc";
  }
}
}

std::shared_ptr<rosbag2::SerializedBagMessage> decompress_message(
  std::shared_ptr<rosbag2::SerializedBagMessage> & to_decompress)
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
    [](rcutils_uint8_array_t * compressed_buffer) {
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

std::string decompress_uri(const std::string & uri) {
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
  ROSBAG2_LOG_ERROR_STREAM("Unable to open compressed file.");
  throw std::runtime_error("Unable to open file");
}

namespace rosbag2
{

SequentialReader::SequentialReader(
  std::unique_ptr<rosbag2_storage::StorageFactoryInterface> storage_factory,
  std::shared_ptr<SerializationFormatConverterFactoryInterface> converter_factory)
: storage_factory_(std::move(storage_factory)), converter_factory_(std::move(converter_factory)),
  converter_(nullptr)
{}

SequentialReader::~SequentialReader()
{
  storage_.reset();  // Necessary to ensure that the storage is destroyed before the factory
}

void
SequentialReader::open(
  const StorageOptions & storage_options, const ConverterOptions & converter_options)
{
  /// Hardcoded for POC.
  std::string dir_name, new_uri, comp_file_relative_path;
  // Need to clean b/c someone might specify URI with trailing backslash.
  clean_path(storage_options.uri, dir_name, new_uri, comp_file_relative_path);
  ROSBAG2_LOG_INFO_STREAM("dir_name: " << dir_name);
  ROSBAG2_LOG_INFO_STREAM("new_uri: " << new_uri);
  ROSBAG2_LOG_INFO_STREAM("relative_path: " << comp_file_relative_path);
  storage_ = storage_factory_->open_read_only(new_uri, storage_options.storage_id);
  /// End POC

  if (!storage_) {
    throw std::runtime_error("No storage could be initialized. Abort");
  }

  /// POC
  ROSBAG2_LOG_INFO_STREAM(
    "compression_identifier: " << storage_->get_metadata().compression_identifier);
  if (!storage_->get_metadata().compression_identifier.empty()) {
    std::string decompressed_uri = decompress_uri(comp_file_relative_path);
    ROSBAG2_LOG_INFO_STREAM("decompressed_uri: " << decompressed_uri);
  }
  /// End POC

  auto topics = storage_->get_metadata().topics_with_message_count;
  if (topics.empty()) {
    return;
  }

  // Currently a bag file can only be played if all topics have the same serialization format.
  auto storage_serialization_format = topics[0].topic_metadata.serialization_format;
  for (const auto & topic : topics) {
    if (topic.topic_metadata.serialization_format != storage_serialization_format) {
      throw std::runtime_error("Topics with different rwm serialization format have been found. "
              "All topics must have the same serialization format.");
    }
  }

  if (converter_options.output_serialization_format != storage_serialization_format) {
    converter_ = std::make_unique<Converter>(
      storage_serialization_format,
      converter_options.output_serialization_format,
      converter_factory_);
    auto topics = storage_->get_all_topics_and_types();
    for (const auto & topic_with_type : topics) {
      converter_->add_topic(topic_with_type.name, topic_with_type.type);
    }
  }
}

bool SequentialReader::has_next()
{
  if (storage_) {
    return storage_->has_next();
  }
  throw std::runtime_error("Bag is not open. Call open() before checking next message.");
}

std::shared_ptr<SerializedBagMessage> SequentialReader::read_next()
{
  if (storage_) {
    auto message = storage_->read_next();
    return converter_ ? converter_->convert(message) : message;
  }
  throw std::runtime_error("Bag is not open. Call open() before reading next message.");
}

std::vector<TopicMetadata> SequentialReader::get_all_topics_and_types()
{
  if (storage_) {
    return storage_->get_all_topics_and_types();
  }
  throw std::runtime_error("Bag is not open. Call open() before getting all topics.");
}

}  // namespace rosbag2
