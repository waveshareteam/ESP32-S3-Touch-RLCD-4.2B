/*
 * Imported from 78/xiaozhi-esp32.
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>

#include "ogg_demuxer.h"
#include "esp_log.h"

#define TAG "OggDemuxer"

/// @brief Reset the demuxer.
void OggDemuxer::Reset()
{
    opus_info_ = {
        .head_seen = false,
        .tags_seen = false,
        .sample_rate = 48000
    };

    state_ = ParseState::FIND_PAGE;
    ctx_.packet_len = 0;
    ctx_.seg_count = 0;
    ctx_.seg_index = 0;
    ctx_.data_offset = 0;
    ctx_.bytes_needed = 4;          // Read the four-byte "OggS" capture pattern.
    ctx_.seg_remaining = 0;
    ctx_.body_size = 0;
    ctx_.body_offset = 0;
    ctx_.packet_continued = false;

    // Clear the buffers.
    memset(ctx_.header, 0, sizeof(ctx_.header));
    memset(ctx_.seg_table, 0, sizeof(ctx_.seg_table));
    memset(ctx_.packet_buf, 0, sizeof(ctx_.packet_buf));
}

/// @brief Process a data block.
/// @param data Input data.
/// @param size Size of the input data in bytes.
/// @return Number of bytes processed.
size_t OggDemuxer::Process(const uint8_t* data, size_t size)
{
    size_t processed = 0;  // Number of bytes processed.

    while (processed < size) {
        switch (state_) {
          case ParseState::FIND_PAGE: {
            // Find the "OggS" page capture pattern.
            if (ctx_.bytes_needed < 4) {
                // Complete a partial "OggS" match spanning data blocks.
                size_t to_copy = std::min(size - processed, ctx_.bytes_needed);
                memcpy(ctx_.header + (4 - ctx_.bytes_needed), data + processed, to_copy);

                processed += to_copy;
                ctx_.bytes_needed -= to_copy;

                if (ctx_.bytes_needed == 0) {
                    // Check for an "OggS" match.
                    if (memcmp(ctx_.header, "OggS", 4) == 0) {
                        state_ = ParseState::PARSE_HEADER;
                        ctx_.data_offset = 4;
                        ctx_.bytes_needed = 27 - 4;  // Read the remaining 23 bytes of the page header.
                    } else {
                        // On a mismatch, advance by one byte and keep searching.
                        memmove(ctx_.header, ctx_.header + 1, 3);
                        ctx_.bytes_needed = 1;
                    }
                } else {
                    // Not enough data; wait for the next data block.
                    return processed;
                }
            } else if (ctx_.bytes_needed == 4) {
                // Find a complete "OggS" capture pattern in the data block.
                bool found = false;
                size_t i = 0;
                size_t remaining = size - processed;

                // Search for "OggS".
                for (; i + 4 <= remaining; i++) {
                    if (memcmp(data + processed + i, "OggS", 4) == 0) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    // Found "OggS"; skip the bytes already searched.
                    processed += i;

                    // The matched "OggS" bytes do not need to be copied.
                    // memcpy(ctx_.header, data + processed, 4);
                    processed += 4;

                    state_ = ParseState::PARSE_HEADER;
                    ctx_.data_offset = 4;
                    ctx_.bytes_needed = 27 - 4;  // Read the remaining 23 bytes.
                } else {
                    // No complete "OggS" match; preserve a possible partial match.
                    size_t partial_len = remaining - i;
                    if (partial_len > 0) {
                        memcpy(ctx_.header, data + processed + i, partial_len);
                        ctx_.bytes_needed = 4 - partial_len;
                        processed += i + partial_len;
                    } else {
                        processed += i;  // All available bytes have been searched.
                    }
                    return processed;  // Return the number of bytes processed.
                }
            } else {
                ESP_LOGE(TAG, "OggDemuxer run in error state: bytes_needed=%zu", ctx_.bytes_needed);
                Reset();
                return processed;
            }
            break;
          }

          case ParseState::PARSE_HEADER: {
            size_t available = size - processed;

            if (available < ctx_.bytes_needed) {
                // Not enough data; copy the available portion.
                memcpy(ctx_.header + ctx_.data_offset,
                        data + processed, available);

                ctx_.data_offset += available;
                ctx_.bytes_needed -= available;
                processed += available;
                return processed;  // Wait for more data.
            } else {
                // Enough data is available to complete the page header.
                size_t to_copy = ctx_.bytes_needed;
                memcpy(ctx_.header + ctx_.data_offset,
                        data + processed, to_copy);

                processed += to_copy;
                ctx_.data_offset += to_copy;
                ctx_.bytes_needed = 0;

                // Validate the page header.
                if (ctx_.header[4] != 0) {
                    ESP_LOGE(TAG, "无效的Ogg版本: %d", ctx_.header[4]);
                    state_ = ParseState::FIND_PAGE;
                    ctx_.bytes_needed = 4;
                    ctx_.data_offset = 0;
                    break;
                }

                ctx_.seg_count = ctx_.header[26];
                if (ctx_.seg_count > 0 && ctx_.seg_count <= 255) {
                    state_ = ParseState::PARSE_SEGMENTS;
                    ctx_.bytes_needed = ctx_.seg_count;
                    ctx_.data_offset = 0;
                } else if (ctx_.seg_count == 0) {
                    // The page has no segments; advance to the next page.
                    state_ = ParseState::FIND_PAGE;
                    ctx_.bytes_needed = 4;
                    ctx_.data_offset = 0;
                } else {
                    ESP_LOGE(TAG, "无效的段数: %u", ctx_.seg_count);
                    state_ = ParseState::FIND_PAGE;
                    ctx_.bytes_needed = 4;
                    ctx_.data_offset = 0;
                }
            }
            break;
        }

          case ParseState::PARSE_SEGMENTS: {
            size_t available = size - processed;

            if (available < ctx_.bytes_needed) {
                memcpy(ctx_.seg_table + ctx_.data_offset,
                        data + processed, available);

                ctx_.data_offset += available;
                ctx_.bytes_needed -= available;
                processed += available;
                return processed;  // Wait for more data.
            } else {
                size_t to_copy = ctx_.bytes_needed;
                memcpy(ctx_.seg_table + ctx_.data_offset,
                        data + processed, to_copy);

                processed += to_copy;
                ctx_.data_offset += to_copy;
                ctx_.bytes_needed = 0;

                state_ = ParseState::PARSE_DATA;
                ctx_.seg_index = 0;
                ctx_.data_offset = 0;

                // Calculate the total page body size.
                ctx_.body_size = 0;
                for (size_t i = 0; i < ctx_.seg_count; ++i) {
                    ctx_.body_size += ctx_.seg_table[i];
                }
                ctx_.body_offset = 0;
                ctx_.seg_remaining = 0;
            }
            break;
        }

          case ParseState::PARSE_DATA: {
            while (ctx_.seg_index < ctx_.seg_count && processed < size) {
                uint8_t seg_len = ctx_.seg_table[ctx_.seg_index];

                // Check whether the segment has already been partially read.
                if (ctx_.seg_remaining > 0) {
                    seg_len = ctx_.seg_remaining;
                } else {
                    ctx_.seg_remaining = seg_len;
                }

                // Check the remaining packet buffer capacity.
                if (ctx_.packet_len + seg_len > sizeof(ctx_.packet_buf)) {
                    ESP_LOGE(TAG, "包缓冲区溢出: %zu + %u > %zu", ctx_.packet_len, seg_len, sizeof(ctx_.packet_buf));
                    state_ = ParseState::FIND_PAGE;
                    ctx_.packet_len = 0;
                    ctx_.packet_continued = false;
                    ctx_.seg_remaining = 0;
                    ctx_.bytes_needed = 4;
                    return processed;
                }

                // Copy the data.
                size_t to_copy = std::min(size - processed, (size_t)seg_len);
                memcpy(ctx_.packet_buf + ctx_.packet_len, data + processed, to_copy);

                processed += to_copy;
                ctx_.packet_len += to_copy;
                ctx_.body_offset += to_copy;
                ctx_.seg_remaining -= to_copy;

                // Check whether the segment is complete.
                if (ctx_.seg_remaining > 0) {
                    // The segment is incomplete; wait for more data.
                    return processed;
                }

                // The segment is complete.
                bool seg_continued = (ctx_.seg_table[ctx_.seg_index] == 255);

                if (!seg_continued) {
                    // End of the packet.
                    if (ctx_.packet_len) {
                        if (!opus_info_.head_seen) {
                            if (ctx_.packet_len >=8 && memcmp(ctx_.packet_buf, "OpusHead", 8) == 0) {
                                opus_info_.head_seen = true;
                                if (ctx_.packet_len >= 19) {
                                    opus_info_.sample_rate = ctx_.packet_buf[12] |
                                                            (ctx_.packet_buf[13] << 8) |
                                                            (ctx_.packet_buf[14] << 16) |
                                                            (ctx_.packet_buf[15] << 24);
                                    ESP_LOGD(TAG, "OpusHead found, sample_rate=%d", opus_info_.sample_rate);
                                }
                                ctx_.packet_len = 0;
                                ctx_.packet_continued = false;
                                ctx_.seg_index++;
                                ctx_.seg_remaining = 0;
                                continue;
                            }
                        }
                        if (!opus_info_.tags_seen) {
                            if (ctx_.packet_len >= 8 && memcmp(ctx_.packet_buf, "OpusTags", 8) == 0) {
                                opus_info_.tags_seen = true;
                                ESP_LOGD(TAG, "OpusTags found.");
                                ctx_.packet_len = 0;
                                ctx_.packet_continued = false;
                                ctx_.seg_index++;
                                ctx_.seg_remaining = 0;
                                continue;
                            }
                        }
                        if (opus_info_.head_seen && opus_info_.tags_seen) {
                            if (on_demuxer_finished_) {
                                on_demuxer_finished_(ctx_.packet_buf, opus_info_.sample_rate, ctx_.packet_len);
                            }
                        } else {
                            ESP_LOGW(TAG, "当前Ogg容器未解析到OpusHead/OpusTags，丢弃");
                        }
                    }
                    ctx_.packet_len = 0;
                    ctx_.packet_continued = false;
                } else {
                    ctx_.packet_continued = true;
                }

                ctx_.seg_index++;
                ctx_.seg_remaining = 0;
            }

            if (ctx_.seg_index == ctx_.seg_count) {
                // Check whether the entire page body has been read.
                if (ctx_.body_offset < ctx_.body_size) {
                    ESP_LOGW(TAG, "数据体不完整: %zu/%zu",
                            ctx_.body_offset, ctx_.body_size);
                }

                // Preserve packet_len and packet_continued for packets spanning pages.
                if (!ctx_.packet_continued) {
                    ctx_.packet_len = 0;
                }

                // Advance to the next page.
                state_ = ParseState::FIND_PAGE;
                ctx_.bytes_needed = 4;
                ctx_.data_offset = 0;
            }
            break;
        }
        }
    }

    return processed;
}
