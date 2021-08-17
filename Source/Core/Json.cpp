#include "Json.h"

#include "Scanning.h"

#include <format>

#define ASSERT(condition, text)

// Tokenizer
///////////////////////////

enum TokenType
{
	// Single characters
	LeftBracket,
	RightBracket,
	LeftBrace,
	RightBrace,
	Comma,
	Colon,

	// Identifiers and keywords
	Boolean,
	Null,
	Identifier,
	
	// Everything else
	Number,
	String
};

struct Token
{
	Token(TokenType type)
	: m_type(type) {}

	Token(TokenType type, std::string _stringOrIdentifier)
	: m_type(type), m_stringOrIdentifier(_stringOrIdentifier) {}

	Token(TokenType type, double _number)
	: m_type(type), m_number(_number) {}

	Token(TokenType type, bool _boolean)
	: m_type(type), m_boolean(_boolean) {}

	TokenType m_type;
	std::string m_stringOrIdentifier;
	double m_number;
	bool m_boolean;
};

// ***********************************************************************

std::string ParseStringSlow(Scan::ScanningState& scan, char bound)
{	
	char* start = scan.current;
	char* pos = start;
	while (*pos != bound && !Scan::IsAtEnd(scan))
	{
		pos++;
	}
	size_t count = pos - start;
	char* outputString = new char[count * 2]; // to allow for escape chars 
	pos = outputString;

	char* cursor = scan.current;

	for (size_t i = 0; i < count; i++)
	{
		char c = *(cursor++);
		
		// Disallowed characters
		switch (c)
		{
		case '\n':
			break;
		case '\r':
			if (Scan::Peek(scan) == '\n') // CRLF line endings
				cursor++;
			break;
			//Log::Crit("Unexpected end of line"); break;
		default:
			break;
		}

		if (c == '\\')
		{
			char next = *(cursor++);
			switch (next)
			{
			// Convert basic escape sequences to their actual characters
			case '\'': *pos++ = '\''; break;
			case '"': *pos++ = '"'; break;
			case '\\':*pos++ = '\\'; break;
			case 'b': *pos++ = '\b'; break;
			case 'f': *pos++ = '\f'; break;
			case 'n': *pos++ = '\n'; break;
			case 'r': *pos++ = '\r'; break;
			case 't': *pos++ = '\t'; break;
			case 'v': *pos++ = '\v'; break;
			case '0': *pos++ = '\0'; break;

			// Unicode stuff, not doing this for now
			case 'u':
				//Log::Crit("This parser does not yet support unicode escape codes"); break;

			// Line terminators, allowed but we do not include them in the final string
			case '\n':
				break;
			case '\r':
				if (Scan::Peek(scan) == '\n') // CRLF line endings
					cursor++;
				break;
			default:
				*pos++ =  next; // all other escaped characters are kept as is, without the '\' that preceeded it
			}
		}
		else
			*pos++ = c;
	}
	cursor++;

	scan.current = cursor;

	std::string result(outputString, pos);
	delete outputString;
	return result;
}

// ***********************************************************************

std::string ParseString(Scan::ScanningState& scan, char bound)
{	
	char* start = scan.current;
	while (*(scan.current) != bound && !Scan::IsAtEnd(scan))
	{
		if (*(scan.current++) == '\\')
		{
			scan.current = start;
			return ParseStringSlow(scan, bound);
		}
	}
	std::string result(start, scan.current);
	scan.current++;
	return result;
}

// ***********************************************************************

double ParseNumber(Scan::ScanningState& scan)
{	
	scan.current -= 1; // Go back to get the first digit or symbol
	char* start = scan.current;

	// Hex number
	if (Scan::Peek(scan) == '0' && (Scan::PeekNext(scan) == 'x' || Scan::PeekNext(scan) == 'X'))
	{
		Scan::Advance(scan);
		Scan::Advance(scan);
		while (Scan::IsHexDigit(Scan::Peek(scan)))
		{
			Scan::Advance(scan);
		}
	}
	// Normal number
	else
	{
		char c = Scan::Peek(scan);
		while(Scan::IsDigit(c) 
		|| c== '+'
		|| c == '-'
		|| c == '.'
		|| c == 'E'
		|| c == 'e')
		{
			Scan::Advance(scan);
			c = Scan::Peek(scan);
		}
	}

	// TODO: error report. This returns 0.0 if no conversion possible. We can look at the literal string and see
	// If it's 0.0, ".0", "0." or 0. if not there's been an error in the parsing. I know this is cheeky. I don't care.
	return strtod(start, &scan.current);
}

// ***********************************************************************

std::vector<Token> TokenizeJson(std::string jsonText)
{
	Scan::ScanningState scan;
	scan.textStart = jsonText.data();
	scan.textEnd = jsonText.data() + jsonText.size();
	scan.current = (char*)scan.textStart;
	scan.line = 1;

	std::vector<Token> tokens;

	while (!Scan::IsAtEnd(scan))
	{
		char c = Scan::Advance(scan);
		int column = int(scan.current - scan.currentLineStart);
		char* loc = scan.current - 1;
		switch (c)
		{
		// Single character tokens
		case '[': 
			tokens.push_back(Token{LeftBracket}); break;
		case ']': 
			tokens.push_back(Token{RightBracket}); break;
		case '{': 
			tokens.push_back(Token{LeftBrace}); break;
		case '}': 
			tokens.push_back(Token{RightBrace}); break;
		case ':': 
			tokens.push_back(Token{Colon}); break;
		case ',': 
			tokens.push_back(Token{Comma}); break;

		// Comments!
		case '/':
			if (Scan::Match(scan, '/'))
			{
				while (Scan::Peek(scan) != '\n')
					Scan::Advance(scan);
			}
			else if (Scan::Match(scan, '*'))
			{
				while (!(Scan::Peek(scan) == '*' && Scan::PeekNext(scan) == '/'))
					Scan::Advance(scan);

				Scan::Advance(scan); // *
				Scan::Advance(scan); // /
			}
			break;
			
		// Whitespace
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			break;

		// String literals
		case '\'':
		{
			std::string string = ParseString(scan, '\'');
			tokens.push_back(Token{String, string}); break;
			break;
		}
		case '"':
		{
			std::string string = ParseString(scan, '"');
			tokens.push_back(Token{String, string}); break;
			break;		
		}

		default:
			// Numbers
			if (Scan::IsDigit(c) || c == '+' || c == '-' || c == '.')
			{
				double num = ParseNumber(scan);
				tokens.push_back(Token{Number, num});
				break;
			}
			
			// Identifiers and keywords
			if (Scan::IsAlpha(c))
			{
				while (Scan::IsAlphaNumeric(Scan::Peek(scan)))
					Scan::Advance(scan);
				
				std::string identifier = std::string(loc, scan.current);

				// Check for keywords
				if (identifier == "true")
					tokens.push_back(Token{Boolean, true});
				else if (identifier == "false")
					tokens.push_back(Token{Boolean, false});
				else if (identifier == "null")
					tokens.push_back(Token{Null});
				else
					tokens.push_back(Token{Identifier, identifier});
			}
			break;
		}

	}
	return tokens;
}




// ***********************************************************************

std::map<std::string, JsonValue> ParseObject(std::vector<Token>& tokens, int& currentToken);
std::vector<JsonValue> ParseArray(std::vector<Token>& tokens, int& currentToken);

JsonValue ParseValue(std::vector<Token>& tokens, int& currentToken)
{
	Token& token = tokens[currentToken];

	switch (token.m_type)
	{
	case LeftBrace:
		return JsonValue(ParseObject(tokens, currentToken)); break;
	case LeftBracket:
		return JsonValue(ParseArray(tokens, currentToken)); break;
	case String:
		currentToken++;
		return JsonValue(token.m_stringOrIdentifier); break;
	case Number:
	{
		currentToken++;
		double n = token.m_number;
		double intPart;
		if (modf(n, &intPart) == 0.0)
			return JsonValue((long)intPart);
		else
			return JsonValue(n);
		break;
	}
	case Boolean:
		currentToken++;
		return JsonValue(token.m_boolean); break;
	case Null:
		currentToken++;
	default:
		return JsonValue(); break;
	}
	return JsonValue();
}

// ***********************************************************************

std::map<std::string, JsonValue> ParseObject(std::vector<Token>& tokens, int& currentToken)
{
	currentToken++; // Advance over opening brace

	std::map<std::string, JsonValue> map;
	while (currentToken < tokens.size() && tokens[currentToken].m_type != RightBrace)
	{
		// We expect, 
		// identifier or string
		if (tokens[currentToken].m_type != Identifier && tokens[currentToken].m_type != String) {}
			//Log::Crit("Expected identifier or string");

		std::string key = tokens[currentToken].m_stringOrIdentifier;
		currentToken += 1;

		// colon
		if (tokens[currentToken].m_type != Colon) {}
			//Log::Crit("Expected colon");
		currentToken += 1;
		
		// String, Number, Boolean, Null
		// If left bracket or brace encountered, skip until closing
		map[key] = ParseValue(tokens, currentToken);

		// Comma, or right brace
		if (tokens[currentToken].m_type == RightBrace)
			break;
		if (tokens[currentToken].m_type != Comma) {}
			//Log::Crit("Expected comma or Right Curly Brace");
		currentToken += 1;
	}
	currentToken++; // Advance over closing brace
	return std::move(map);
}

// ***********************************************************************

std::vector<JsonValue> ParseArray(std::vector<Token>& tokens, int& currentToken)
{
	currentToken++; // Advance over opening bracket

	std::vector<JsonValue> array;
	while (currentToken < tokens.size() && tokens[currentToken].m_type != RightBracket)
	{
		// We expect, 
		// String, Number, Boolean, Null
		array.push_back(ParseValue(tokens, currentToken));

		// Comma, or right brace
		if (tokens[currentToken].m_type == RightBracket)
			break;
		if (tokens[currentToken].m_type != Comma) {}
			//Log::Crit("Expected comma or right bracket");
		currentToken += 1;
	}
	currentToken++; // Advance over closing bracket
	return std::move(array);
}




// JsonValue implementation
///////////////////////////

// ***********************************************************************

JsonValue::~JsonValue()
{
	if (m_type == Type::Array)
		delete m_internalData.m_pArray;	
	else if (m_type == Type::Object)
		delete m_internalData.m_pObject;
	else if (m_type == Type::String)
	{
		delete m_internalData.m_pString;
	}
	m_type = Type::Null;
}

// ***********************************************************************

JsonValue::JsonValue()
{
	m_type = Type::Null;
	m_internalData.m_pArray = nullptr;
}

// ***********************************************************************

JsonValue::JsonValue(const JsonValue& copy)
{
	// Copy data from the other value
	switch (copy.m_type)
	{
	case Type::Array:
		m_internalData.m_pArray = new std::vector<JsonValue>(*copy.m_internalData.m_pArray);
		break;
	case Type::Object:
		m_internalData.m_pObject = new std::map<std::string, JsonValue>(*copy.m_internalData.m_pObject);
		break;
	case Type::String:
		m_internalData.m_pString = new std::string(*(copy.m_internalData.m_pString));
		break;
	default:
		m_internalData = copy.m_internalData;
		break;
	}
	m_type = copy.m_type;
}

// ***********************************************************************

JsonValue::JsonValue(JsonValue&& copy)
{
	m_internalData = copy.m_internalData;
	copy.m_internalData.m_pArray = nullptr;

	m_type = copy.m_type;
	copy.m_type = Type::Null;	
}

// ***********************************************************************

JsonValue& JsonValue::operator=(const JsonValue& copy)
{
	// Clear out internal stuff
	if (m_type == Type::Array)
		delete m_internalData.m_pArray;	
	else if (m_type == Type::Object)
		delete m_internalData.m_pObject;
	else if (m_type == Type::String)
		delete m_internalData.m_pString;

	// Copy data from the other value
	switch (copy.m_type)
	{
	case Type::Array:
		m_internalData.m_pArray = new std::vector<JsonValue>(copy.m_internalData.m_pArray->begin(), copy.m_internalData.m_pArray->end());
		break;
	case Type::Object:
		m_internalData.m_pObject = new std::map<std::string, JsonValue>(copy.m_internalData.m_pObject->begin(), copy.m_internalData.m_pObject->end());
		break;
	case Type::String:
		m_internalData.m_pString = new std::string(*(copy.m_internalData.m_pString));
	default:
		m_internalData = copy.m_internalData;
		break;
	}
	m_type = copy.m_type;

	return *this;
}

// ***********************************************************************

JsonValue& JsonValue::operator=(JsonValue&& copy)
{
	m_internalData = copy.m_internalData;
	copy.m_internalData.m_pArray = nullptr;

	m_type = copy.m_type;
	copy.m_type = Type::Null;	

	return *this;
}

// ***********************************************************************

JsonValue::JsonValue(const std::vector<JsonValue>& array)
{
	m_internalData.m_pArray = nullptr;
	m_internalData.m_pArray = new std::vector<JsonValue>(array.begin(), array.end());
	m_type = Type::Array;
}

// ***********************************************************************

JsonValue::JsonValue(const std::map<std::string, JsonValue>& object)
{
	m_internalData.m_pArray = nullptr;
	m_internalData.m_pObject = new std::map<std::string, JsonValue>(object.begin(), object.end());
	m_type = Type::Object;
}

// ***********************************************************************

JsonValue::JsonValue(std::string string)
{
	m_internalData.m_pArray = nullptr;
	m_internalData.m_pString = new std::string(string);
	m_type = Type::String;
}

// ***********************************************************************

JsonValue::JsonValue(const char* string)
{
	m_internalData.m_pArray = nullptr;
	m_internalData.m_pString = new std::string(string);
	m_type = Type::String;
}

// ***********************************************************************

JsonValue::JsonValue(double number)
{
	m_internalData.m_pArray = nullptr;
	m_internalData.m_floatingNumber = number;
	m_type = Type::Floating;
}

// ***********************************************************************

JsonValue::JsonValue(long number)
{
	m_internalData.m_pArray = nullptr;
	m_internalData.m_integerNumber = number;
	m_type = Type::Integer;
}

// ***********************************************************************

JsonValue::JsonValue(bool boolean)
{
	m_internalData.m_pArray = nullptr;
	m_internalData.m_boolean = boolean;
	m_type = Type::Boolean;
}

// ***********************************************************************

std::string JsonValue::ToString() const
{
	if (m_type == Type::String)
		return *(m_internalData.m_pString);
	return std::string();
}

// ***********************************************************************

double JsonValue::ToFloat() const
{
	if (m_type == Type::Floating)
		return m_internalData.m_floatingNumber;
	else if (m_type == Type::Integer)
		return (double)m_internalData.m_integerNumber;
	return 0.0f;
}

// ***********************************************************************

long JsonValue::ToInt() const
{
	if (m_type == Type::Integer)
		return m_internalData.m_integerNumber;
	return 0;
}

// ***********************************************************************

bool JsonValue::ToBool() const
{
	if (m_type == Type::Boolean)
		return m_internalData.m_boolean;
	return 0;
}

// ***********************************************************************

bool JsonValue::IsNull() const
{
	return m_type == Type::Null;
}

// ***********************************************************************

bool JsonValue::HasKey(std::string identifier) const
{
	ASSERT(m_type == Type::Object, "Attempting to treat this value as an object when it is not.");
	return m_internalData.m_pObject->count(identifier) >= 1;
}

// ***********************************************************************

int JsonValue::Count() const
{
	ASSERT(m_type == Type::Array || m_type == Type::Object, "Attempting to treat this value as an array or object when it is not.");
	if (m_type == Type::Array)
		return (int)m_internalData.m_pArray->size();
	else
		return (int)m_internalData.m_pObject->size();
}

// ***********************************************************************

JsonValue& JsonValue::operator[](std::string identifier)
{
	ASSERT(m_type == Type::Object, "Attempting to treat this value as an object when it is not.");
	return m_internalData.m_pObject->operator[](identifier);
}

// ***********************************************************************

JsonValue& JsonValue::operator[](size_t index)
{
	ASSERT(m_type == Type::Array, "Attempting to treat this value as an array when it is not.");
	ASSERT(m_internalData.m_pArray->size() > index, "Accessing an element that does not exist in this array, you probably need to append");
	return m_internalData.m_pArray->operator[](index);
}

// ***********************************************************************

const JsonValue& JsonValue::Get(std::string identifier) const
{
	ASSERT(m_type == Type::Object, "Attempting to treat this value as an object when it is not.");
	return m_internalData.m_pObject->operator[](identifier);
}

// ***********************************************************************

const JsonValue& JsonValue::Get(size_t index) const
{
	ASSERT(m_type == Type::Array, "Attempting to treat this value as an array when it is not.");
	ASSERT(m_internalData.m_pArray->size() > index, "Accessing an element that does not exist in this array, you probably need to append");
	return m_internalData.m_pArray->operator[](index);
}

// ***********************************************************************

void JsonValue::Append(JsonValue& value)
{
	ASSERT(m_type == Type::Array, "Attempting to treat this value as an array when it is not.");
	m_internalData.m_pArray->push_back(value);
}

// ***********************************************************************

JsonValue JsonValue::NewObject()
{
	return JsonValue(std::map<std::string, JsonValue>());
}

// ***********************************************************************

JsonValue JsonValue::NewArray()
{
	return JsonValue(std::vector<JsonValue>());
}

// ***********************************************************************

JsonValue ParseJsonFile(std::string& file)
{
	std::vector<Token> tokens = TokenizeJson(file);

	int firstToken = 0;
	JsonValue json5 = ParseValue(tokens, firstToken);
	return json5;
}

// ***********************************************************************

std::string SerializeJsonValue(JsonValue json, std::string indentation)
{
	std::string result = "";
	switch (json.m_type)
	{
	case JsonValue::Type::Array:
		result.append("[");
		if (json.Count() > 0)
			result.append("\n");

		for (const JsonValue& val : *json.m_internalData.m_pArray)
		{
			result += std::format("    {}{}, \n", indentation.c_str(), SerializeJsonValue(val, indentation + "    "));
		}

		if (json.Count() > 0)
			result += std::format("{}", indentation);
		result.append("]");
		break;
	case JsonValue::Type::Object:
	{
		result.append("{");
		if (json.Count() > 0)
			result.append("\n");

		for (const std::pair<std::string, JsonValue>& val : *json.m_internalData.m_pObject)
		{
			result += std::format("    {}{}: {}, \n", indentation, val.first, SerializeJsonValue(val.second, indentation + "    "));
		}

		if (json.Count() > 0)
			result += std::format("{}", indentation);
		result.append("}");
	}
	break;
	case JsonValue::Type::Floating:
		result += std::format("{:.17g}", json.ToFloat());
		break;
		// TODO: Serialize with exponentials like we do with floats
	case JsonValue::Type::Integer:
		result += std::format("{}", json.ToInt());
		break;
	case JsonValue::Type::Boolean:
		result += std::format("{}", json.ToBool() ? "true" : "false");
		break;
	case JsonValue::Type::String:
		result += std::format("\"{}\"", json.ToString().c_str());
		break;
	case JsonValue::Type::Null:
		result.append("null");
		break;
	default:
		result.append("CANT SERIALIZE YET");
		break;
	}

	// Kinda hacky thing that makes small jsonValues serialize as collapsed, easier to read files imo
	if (result.size() < 100)
	{
		size_t index = 0;
		while (true) {
			index = result.find("\n", index);

			int count = 1;
			while((index+count) != std::string::npos && result[index+count] == ' ')
				count++;

			if (index == std::string::npos) break;
			result.replace(index, count, "");
			index += 1;
		}
	}
	return result;
}
