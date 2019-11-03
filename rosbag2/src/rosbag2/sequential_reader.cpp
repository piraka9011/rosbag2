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

void remove_extension(std::string & filename) {
  size_t last_dot = filename.find_last_of('.');
  if (last_dot == std::string::npos) return;
  filename.erase(last_dot, filename.size()-1);
}
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
  std::string file_name;
  std::string relative_path;
  if (storage_options.uri.back() == '/') {
    file_name = storage_options.uri.substr(0, storage_options.uri.size() - 1);
    relative_path = storage_options.uri + file_name + ".db3" + ".compressed_poc";
  }
  else {
    file_name = storage_options.uri;
    relative_path = storage_options.uri + "/" + file_name +  ".db3" + ".compressed_poc";
  }
  std::string decompressed_uri = decompress_uri(relative_path);
  remove_extension(decompressed_uri);
  ROSBAG2_LOG_INFO_STREAM("file_name: " << file_name);
  ROSBAG2_LOG_INFO_STREAM("relative_path: " << relative_path);
  ROSBAG2_LOG_INFO_STREAM("decompressed_uri: " << decompressed_uri);
  storage_ = storage_factory_->open_read_only(decompressed_uri, storage_options.storage_id);
  /// End POC
  if (!storage_) {
    throw std::runtime_error("No storage could be initialized. Abort");
  }
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
  throw std::runtime_error("Bag is not open. Call open() before reading.");
}

std::shared_ptr<SerializedBagMessage> SequentialReader::read_next()
{
  if (storage_) {
    auto message = storage_->read_next();
    if (std::istringstream(storage_->get_metadata().compression_identifier)) {
      std::shared_ptr<SerializedBagMessage> returned_message = converter_ ? converter_->convert
        (message) : message;
    }
  }
  throw std::runtime_error("Bag is not open. Call open() before reading.");
}

std::vector<TopicMetadata> SequentialReader::get_all_topics_and_types()
{
  if (storage_) {
    return storage_->get_all_topics_and_types();
  }
  throw std::runtime_error("Bag is not open. Call open() before reading.");
}

}  // namespace rosbag2
