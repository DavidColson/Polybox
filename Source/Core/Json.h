// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <vector>
#include <map>
#include <string>

struct JsonValue
{
	enum Type
	{
		Object,
		Array,
		Floating,
		Integer,
		Boolean,
		String,
		Null
	};

	JsonValue();

	JsonValue(const JsonValue& copy);
	JsonValue(JsonValue&& copy);
	JsonValue& operator=(const JsonValue& copy);
	JsonValue& operator=(JsonValue&& copy);

	JsonValue(const std::vector<JsonValue>& array);
	JsonValue(const std::map<std::string, JsonValue>& object);
	JsonValue(std::string string);
	JsonValue(const char* string);
	JsonValue(double number);
	JsonValue(long number);
	JsonValue(bool boolean);

	static JsonValue NewObject();
	static JsonValue NewArray();

	bool IsNull() const;
	bool IsArray() const;
	bool IsObject() const;
	bool HasKey(std::string identifier) const;
	int Count() const;

	std::string ToString() const;
	double ToFloat() const;
	long ToInt() const;
	bool ToBool() const;

	JsonValue& operator[](std::string identifier);
	JsonValue& operator[](size_t index);

	const JsonValue& Get(std::string identifier) const;
	const JsonValue& Get(size_t index) const;

	void Append(JsonValue& value);

	~JsonValue();

	Type m_type;
	union Data
	{
		std::vector<JsonValue>* m_pArray;
		std::map<std::string, JsonValue>* m_pObject;
		std::string* m_pString;
		double m_floatingNumber;
		long m_integerNumber;
		bool m_boolean;
	} m_internalData;
};

JsonValue ParseJsonFile(std::string& file);
std::string SerializeJsonValue(JsonValue json, std::string indentation = "");