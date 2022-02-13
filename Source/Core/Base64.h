// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <string>

std::string DecodeBase64(std::string const& encoded_string);

std::string EncodeBase64(size_t length, const char* bytes);
