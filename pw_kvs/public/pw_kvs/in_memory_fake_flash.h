// Copyright 2020 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.
#pragma once

#include <array>
#include <cstring>

// TODO: Push/pop log module name due to logging in header.
//       Alternately: Push implementation into .cc
#include "pw_kvs/flash_memory.h"
#include "pw_log/log.h"
#include "pw_status/status.h"

namespace pw::kvs {

// This creates a buffer which mimics the behaviour of flash (requires erase,
// before write, checks alignments, and is addressed in sectors).
template <uint32_t kSectorSize, uint16_t kSectorCount>
class InMemoryFakeFlash : public FlashMemory {
 public:
  InMemoryFakeFlash(uint8_t alignment_bytes = 1)  // default 8 bit alignment
      : FlashMemory(kSectorSize, kSectorCount, alignment_bytes) {}

  // Always enabled
  Status Enable() override { return Status::OK; }
  Status Disable() override { return Status::OK; }
  bool IsEnabled() const override { return true; }

  // Erase num_sectors starting at a given address. Blocking call.
  // Address should be on a sector boundary.
  // Returns: OK, on success.
  //          INVALID_ARGUMENT, if address or sector count is invalid.
  //          UNKNOWN, on HAL error
  Status Erase(Address address, size_t num_sectors) override {
    if (address % sector_size_bytes() != 0) {
      ERR("Attempted to erase sector at non-sector aligned boundary: %zx",
          size_t(address));
      return Status::INVALID_ARGUMENT;
    }
    size_t sector_id = address / sector_size_bytes();
    if (address / sector_size_bytes() + num_sectors > sector_count()) {
      ERR("Tried to erase a sector at an address past partition end; "
          "address: %zx, sector implied: %zu",
          size_t(address),
          sector_id);
      return Status::UNKNOWN;
    }
    if (address % alignment_bytes() != 0) {
      return Status::INVALID_ARGUMENT;
    }
    std::memset(&buffer_[address], 0xFF, sector_size_bytes() * num_sectors);
    return Status::OK;
  }

  // Reads bytes from flash into buffer. Blocking call.
  // Returns: OK, on success.
  //          INVALID_ARGUMENT, if address or length is invalid.
  //          UNKNOWN, on HAL error
  StatusWithSize Read(Address address, span<std::byte> output) override {
    if (address + output.size() >= sector_count() * size_bytes()) {
      return Status::INVALID_ARGUMENT;
    }
    std::memcpy(output.data(), &buffer_[address], output.size());
    return Status::OK;
  }

  // Writes bytes to flash. Blocking call.
  // Returns: OK, on success.
  //          INVALID_ARGUMENT, if address or length is invalid.
  //          UNKNOWN, on HAL error
  StatusWithSize Write(Address address, span<const std::byte> data) override {
    if ((address + data.size()) >= sector_count() * size_bytes() ||
        address % alignment_bytes() != 0 ||
        data.size() % alignment_bytes() != 0) {
      return Status::INVALID_ARGUMENT;
    }
    // Check in erased state
    for (unsigned i = 0; i < data.size(); i++) {
      if (buffer_[address + i] != 0xFF) {
        ERR("Writing to previously written address: %zx", size_t(address));
        return Status::UNKNOWN;
      }
    }
    std::memcpy(&buffer_[address], data.data(), data.size());
    return StatusWithSize(data.size());
  }

 private:
  std::array<uint8_t, kSectorCount * kSectorSize> buffer_;
};

}  // namespace pw::kvs