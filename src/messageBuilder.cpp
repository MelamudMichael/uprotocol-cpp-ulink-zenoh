/*
 * Copyright (c) 2023 General Motors GTO LLC
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: 2023 General Motors GTO LLC
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include <uprotocol-cpp-ulink-zenoh/message/messageBuilder.h>
#include <uprotocol-cpp/tools/base64.h>
#include <uprotocol-cpp/uuid/serializer/UuidSerializer.h>
#include <uprotocol-cpp/uri/serializer/LongUriSerializer.h>
#include <spdlog/spdlog.h>

using namespace uprotocol::uri;
using namespace uprotocol::uuid;
std::vector<uint8_t> MessageBuilder::buildHeader(const UAttributes &attributes) {
    std::vector<uint8_t> header;

    // Mandatory attributes
    auto idBytes = UuidSerializer::serializeToBytes(attributes.id());
    addTag(header, Tag::ID, idBytes.data(), idBytes.size());

    auto messageType = static_cast<uint8_t>(attributes.type());
    addTag(header, Tag::TYPE, &messageType, sizeof(messageType));

    auto priority = static_cast<uint8_t>(attributes.priority());
    addTag(header, Tag::PRIORITY, &priority, sizeof(priority));

    // Optional attributes
    if (attributes.ttl().has_value()) {
        int32_t ttl = attributes.ttl().value();
        addTag(header, Tag::TTL, reinterpret_cast<uint8_t*>(&ttl), sizeof(ttl));
    }

    if (attributes.token().has_value()) {
        const auto& token = attributes.token().value();
        addTag(header, Tag::TOKEN, reinterpret_cast<const uint8_t*>(token.c_str()), token.size());
    }

    if (attributes.serializationHint().has_value()) {
        auto hint = static_cast<uint8_t>(attributes.serializationHint().value());
        addTag(header, Tag::HINT, &hint, sizeof(hint));
    }

    if (attributes.sink().has_value()) {
        auto sinkUri = LongUriSerializer::serialize(attributes.sink().value());
        addTag(header, Tag::SINK, reinterpret_cast<const uint8_t*>(sinkUri.c_str()), sinkUri.size());
    }

    if (attributes.plevel().has_value()) {
        int32_t plevel = attributes.plevel().value();
        addTag(header, Tag::PLEVEL, reinterpret_cast<uint8_t*>(&plevel), sizeof(plevel));
    }

    if (attributes.commstatus().has_value()) {
        int32_t commstatus = attributes.commstatus().value();
        addTag(header, Tag::COMMSTATUS, reinterpret_cast<uint8_t*>(&commstatus), sizeof(commstatus));
    }

    if (attributes.reqid().has_value()) {
        auto reqIdBytes = UuidSerializer::serializeToBytes(attributes.reqid().value());
        addTag(header, Tag::REQID, reqIdBytes.data(), reqIdBytes.size());
    }

    return header;
}

size_t MessageBuilder::addTag(std::vector<uint8_t>& buffer, 
                              Tag tag, 
                              const uint8_t* data, 
                              size_t size, 
                              size_t pos) {
    
    spdlog::debug("addTag : tag = {} size = {} pos = {}",static_cast<uint8_t>(tag), size, pos);
 
    buffer[pos] = static_cast<uint8_t>(tag);

    std::memcpy(&buffer[pos + sizeof(uint8_t)], &size, sizeof(size));

    std::memcpy(&buffer[pos + sizeof(size) + sizeof(uint8_t)], data, size);

    return pos + sizeof(uint8_t) + sizeof(size) + size;
}

template <typename T>
size_t MessageBuilder::addTag(std::vector<uint8_t>& buffer, 
                              Tag tag, 
                              const T& value, 
                              size_t pos) {
    
    const uint8_t* valueBytes = nullptr;
    size_t valueSize = 0;

    if constexpr (std::is_same_v<T, std::string>) {
        valueSize = value.length();
        valueBytes = reinterpret_cast<const uint8_t*>(value.c_str());
    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>){
        valueSize = value.size();
        valueBytes = reinterpret_cast<const uint8_t*>(value.data());
    } else {
        valueBytes = reinterpret_cast<const uint8_t*>(&value);
        valueSize = sizeof(T);
    }

    return addTag(buffer, tag, valueBytes, valueSize, pos);
}

template <typename T>
void MessageBuilder::updateSize(const T& value, 
                                size_t &msgSize) noexcept {
    size_t valueSize;

    if constexpr (std::is_same_v<T, std::string>) {
        valueSize = value.length();
    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>){
        valueSize = value.size();
    } else {
        valueSize = sizeof(T);
    }

    updateSize(valueSize, msgSize);
}

void MessageBuilder::updateSize(size_t size, 
                                size_t &msgSize) noexcept {
    msgSize += 1;
    msgSize += sizeof(size_t);
    msgSize += size;

    spdlog::debug("updated message size = {}", msgSize);
}

size_t MessageBuilder::calculateSize(const UAttributes &attributes, 
                                     const UPayload &payload) noexcept {

    size_t msgSize = 0;

    updateSize(UuidSerializer::serializeToBytes(attributes.id()), msgSize);
    updateSize(attributes.type(), msgSize);
    updateSize(attributes.priority(), msgSize);

    if (attributes.ttl().has_value()) {
        updateSize(attributes.ttl().value(), msgSize);
    }

    if (attributes.token().has_value()) {
        updateSize(attributes.token().value(), msgSize);
    }

    if (attributes.serializationHint().has_value()) {
        updateSize(attributes.serializationHint().value(), msgSize);
    }

    if (attributes.sink().has_value()) {    
        updateSize(uprotocol::tools::Base64::encode(LongUriSerializer::serialize(attributes.sink().value())), msgSize);
    }

    if (attributes.plevel().has_value()) {
        updateSize(attributes.plevel().value(), msgSize);
    }

    if (attributes.commstatus().has_value()) {
        updateSize(attributes.commstatus().value(), msgSize);
    }

    if (attributes.reqid().has_value()) {
        updateSize(UuidSerializer::serializeToBytes(attributes.reqid().value()), msgSize);
    }

    updateSize(payload.size(), msgSize);

    return msgSize;
}
