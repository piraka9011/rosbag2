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

#include <cassert>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "snappy.h"
#include "rcpputils/filesystem_helper.hpp"
#include "rosbag2/compression_options.hpp"
#include "rosbag2/info.hpp"
#include "rosbag2/logging.hpp"
#include "rosbag2_storage/metadata_io.hpp"

namespace
{
void remove_extension(std::string & filename, int n_times = 1) {
  for (int i = 0; i < n_times; i++) {
    size_t last_dot = filename.find_last_of('.');
    if (last_dot == std::string::npos) return;
    filename.erase(last_dot, filename.size() - 1);
  }
}

void clean_uri(const std::string &uri, std::string &new_uri)
{
  if (uri.back() == '/') {
    new_uri = uri + uri.substr(0, uri.size() - 1);;
  } else {
    new_uri = uri + "/" + uri;
  }
}

}
namespace rosbag2
{

SequentialReader::SequentialReader(
  std::unique_ptr<rosbag2_storage::StorageFactoryInterface> storage_factory,
  std::shared_ptr<SerializationFormatConverterFactoryInterface> converter_factory)
: storage_factory_(std::move(storage_factory)), converter_factory_(std::move(converter_factory)),
  converter_(nullptr)
{
  decompressor_ = std::make_unique<DecompressorPoC>();
}

SequentialReader::~SequentialReader()
{
  storage_.reset();  // Necessary to ensure that the storage is destroyed before the factory
}

void
SequentialReader::open(
  const StorageOptions & storage_options, const ConverterOptions & converter_options)
{
  storage_options_ = storage_options;
  /// Hardcoded for POC.
  std::string new_uri, comp_file_relative_path;
  // Need to clean b/c someone might specify URI with trailing backslash.
  clean_uri(storage_options_.uri, new_uri);
  // New way of reading metadata
  rosbag2_storage::MetadataIo metadata_io;
  metadata_ = std::make_unique<rosbag2_storage::BagMetadata>(
    metadata_io.read_metadata(storage_options_.uri));
  // Check if we need to compress
  ROSBAG2_LOG_INFO_STREAM("compression_identifier: " << metadata_->compression_format);
  ROSBAG2_LOG_INFO_STREAM("compression_mode: " << metadata_->compression_mode);
  if (!metadata_->compression_format.empty()) {
    file_is_compressed_ = StringToCompressionModeMap.at(metadata_->compression_mode) ==
                          CompressionMode::FILE;
    message_is_compressed_ = StringToCompressionModeMap.at(metadata_->compression_mode) ==
                             CompressionMode::MESSAGE;
    if (file_is_compressed_) {
      ROSBAG2_LOG_INFO("Found compressed files.");
      decompressor_->uri_to_relative_path(new_uri, comp_file_relative_path);
      ROSBAG2_LOG_INFO_STREAM("relative_path: " << comp_file_relative_path);
      std::string decompressed_uri = decompressor_->decompress_file(comp_file_relative_path);
      ROSBAG2_LOG_INFO_STREAM("decompressed_uri: " << decompressed_uri);
    }
  }

  storage_ = storage_factory_->open_read_only(new_uri, storage_options_.storage_id);
  /// End POC

  if (!storage_) {
    throw std::runtime_error("No storage could be initialized. Abort");
  }

  file_paths_ = metadata_->relative_file_paths;
  current_file_iterator_ = file_paths_.begin();

  auto topics = metadata_->topics_with_message_count;
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

bool SequentialReader::has_next_file() const
{
  return current_file_iterator_ + 1 != file_paths_.end();
}

void SequentialReader::load_next_file()
{
  assert(current_file_iterator_ != file_paths_.end());
  current_file_iterator_++;
}

bool SequentialReader::has_next()
{
  if (storage_) {
    if (!storage_->has_next()) {
      if (has_next_file()) {
        load_next_file();
        if (file_is_compressed_) {
          remove_extension(*current_file_iterator_, 2);
          std::string relative_compressed_file_path;
          decompressor_->uri_to_relative_path(*current_file_iterator_,
            relative_compressed_file_path);
          decompressor_->decompress_file(relative_compressed_file_path);
        }
        storage_ = storage_factory_->open_read_only(*current_file_iterator_, storage_options_.storage_id);
      }
    }
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
