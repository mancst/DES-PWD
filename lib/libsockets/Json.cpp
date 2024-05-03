/**
 **	\file Json.cpp
 **	\date  2011-08-16
 **	\author grymse@alhem.net
**/
/*
Copyright (C) 2011  Anders Hedstrom

This library is made available under the terms of the GNU GPL.

If you would like to use this library in a closed-source application,
a separate license agreement is available. For information about 
the closed-source license agreement for the C++ sockets library,
please visit http://www.alhem.net/Sockets/license.html and/or
email license@alhem.net.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "Json.h"
#include <sstream>
#include "Utility.h"
#include <cstdio>

#ifdef _WIN32
#ifdef GetObject
#undef GetObject
#endif
#endif

#define C buffer[index]

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

static Json s_js_inner;

// --------------------------------------------------------------------------------
Json::Json() : m_type(TYPE_UNKNOWN)
{
	// by itc
	m_d_value = (double)(0.0);
}

// --------------------------------------------------------------------------------
Json::Json(char value) : m_type(TYPE_INTEGER)
{
	// by itc
	m_d_value = (double)(value);
}

// --------------------------------------------------------------------------------
Json::Json(short value) : m_type(TYPE_SIGN_INTEGER)
{
	m_d_value = (double)(value);
}

Json::Json(unsigned short value) : m_type(TYPE_INTEGER)
{
	m_d_value = (double)(value);
}

// --------------------------------------------------------------------------------
Json::Json(int value) : m_type(TYPE_SIGN_INTEGER)
{
	m_d_value = (double)(value);
}

// --------------------------------------------------------------------------------
Json::Json(unsigned int value) : m_type(TYPE_INTEGER)
{
	m_d_value = (double)(value);
}

// --------------------------------------------------------------------------------
Json::Json(uint64_t value) : m_type(TYPE_INTEGER)
{
	m_d_value = (double)(value);
}

Json::Json(int64_t value) : m_type(TYPE_SIGN_INTEGER)
{
	m_d_value = (double)(value);
}


// --------------------------------------------------------------------------------
Json::Json(double value) : m_type(TYPE_REAL), m_d_value(value)
{
}

Json::Json(float value) : m_type(TYPE_REAL), m_d_value(value)
{
}


// --------------------------------------------------------------------------------
Json::Json(const char *value) : m_type(TYPE_STRING), m_str_value(value)
{
	// by itc
    m_d_value = 0.0;
}

// --------------------------------------------------------------------------------
Json::Json(const std::string& value) : m_type(TYPE_STRING), m_str_value(value)
{
	// by itc
    m_d_value = 0.0;
}


// --------------------------------------------------------------------------------
Json::Json(bool value) : m_type(TYPE_BOOLEAN)
{
	m_d_value = (double)(value);
}

// --------------------------------------------------------------------------------
Json::Json(json_type_t t) : m_type(t)
{
	if (t != TYPE_ARRAY && t != TYPE_OBJECT)
		throw Exception("Must be type: Array or type: Object");

	// by itc
	m_d_value = (double)(0.0);
	m_str_value = "";
}

// --------------------------------------------------------------------------------
Json::~Json()
{
}


// --------------------------------------------------------------------------------
Json::json_type_t Json::Type() const { return m_type; }


// --------------------------------------------------------------------------------
bool Json::HasValue(const std::string& name) const
{
	if (m_type != TYPE_OBJECT)
		return false;
	return m_object.find(name) != m_object.end();
}

void Json::RemoveValue(const std::string& name)
{
	json_map_t::iterator it = m_object.find(name);
	if(it != m_object.end())
		m_object.erase(it);
}

// --------------------------------------------------------------------------------
Json::operator uint64_t() const
{
	return (uint64_t)m_d_value;
}
Json::operator int64_t() const
{
    return (int64_t)m_d_value;
}

// --------------------------------------------------------------------------------
Json::operator char() const
{
	return (char)m_d_value;
}

// --------------------------------------------------------------------------------
Json::operator unsigned short() const
{
	return (unsigned short)m_d_value;
}
Json::operator short() const
{
	return (short)m_d_value;
}

Json::operator uint32_t() const
{
	return (uint32_t)m_d_value;
}
Json::operator int32_t() const
{
	return (int32_t)m_d_value;
}

// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
Json::operator double() const
{
	return m_d_value;
}
Json::operator float() const
{
	return m_d_value;
}

// --------------------------------------------------------------------------------
// by itc add
Json::operator const char *() const
{
	return m_str_value.c_str();
}

// --------------------------------------------------------------------------------
Json::operator std::string() const
{
	return m_str_value;
}

// --------------------------------------------------------------------------------
Json::operator bool() const
{
	return (bool)m_d_value;
}

// --------------------------------------------------------------------------------
const Json& Json::operator[](const char *name) const
{
	if (m_type != TYPE_OBJECT)
		//throw Exception("Must be type: Object");
		return s_js_inner;
	json_map_t::const_iterator it = m_object.find(name);
	if (it != m_object.end())
		return it -> second;
	//throw Exception("Key not found: " + std::string(name));
	return s_js_inner;
}

// --------------------------------------------------------------------------------
Json& Json::operator[](const char *name)
{
	if (m_type == TYPE_UNKNOWN)
		m_type = TYPE_OBJECT;
	if (m_type != TYPE_OBJECT)
	//	throw Exception("Must be type: Object");
		return s_js_inner;
	return m_object[name];
}


// --------------------------------------------------------------------------------
const Json& Json::operator[](const std::string& name) const
{
	if (m_type != TYPE_OBJECT)
		//throw Exception("Must be type: Object");
		return s_js_inner;
	json_map_t::const_iterator it = m_object.find(name);
	if (it != m_object.end())
		return it -> second;
	//throw Exception("Key not found: " + name);
	return s_js_inner;
}

// --------------------------------------------------------------------------------
Json& Json::operator[](const std::string& name)
{
	if (m_type == TYPE_UNKNOWN)
		m_type = TYPE_OBJECT;
	if (m_type != TYPE_OBJECT)
		//throw Exception("Must be type: Object");
		return s_js_inner;
	return m_object[name];
}

// --------------------------------------------------------------------------------
void Json::Add(const Json & data)
{
	if (m_type == TYPE_UNKNOWN)
		m_type = TYPE_ARRAY;
	if (m_type != TYPE_ARRAY)
		throw Exception("trying to add array data in non-array");
	m_array.push_back(data);
}

// --------------------------------------------------------------------------------
const std::string& Json::GetString() const
{
	return m_str_value;
}

// --------------------------------------------------------------------------------
Json::json_list_t& Json::GetArray()
{
	if (m_type == TYPE_UNKNOWN)
		m_type = TYPE_ARRAY;
	if (m_type != TYPE_ARRAY)
		throw Exception("Json instance not of type: Array");
	return m_array;
}


// --------------------------------------------------------------------------------
Json::json_map_t& Json::GetObject()
{
	if (m_type == TYPE_UNKNOWN)
		m_type = TYPE_OBJECT;
	if (m_type != TYPE_OBJECT)
		throw Exception("Json instance not of type: Array");
	return m_object;
}

// itc add -------------------------------------------------------------------
Json & Json::operator = (const Json & j)
{
	if(this != &j) {
		m_type = j.m_type;
		m_d_value = j.m_d_value;
		m_str_value = j.m_str_value;
		m_array = j.m_array;
		m_object = j.m_object;
	}
	return *this;
}

Json & Json::operator++()
{
	if(m_type != TYPE_SIGN_INTEGER && m_type != TYPE_INTEGER && m_type != TYPE_REAL)
		return *this;
	m_d_value++;
	return *this;
}

// --------------------------------------------------------------------------------
const Json::json_list_t& Json::GetArray() const
{
	if (m_type != TYPE_ARRAY)
		throw Exception("Json instance not of type: Array");
	return m_array;
}


// --------------------------------------------------------------------------------
const Json::json_map_t& Json::GetObject() const
{
	if (m_type != TYPE_OBJECT)
		throw Exception("Json instance not of type: Array");
	return m_object;
}


// --------------------------------------------------------------------------------
Json Json::Parse(const std::string& data)
{
	size_t i = 0;
	Json obj;
	obj.Parse(data.c_str(), i);
	return obj;
}

// --------------------------------------------------------------------------------
int Json::Token(const char *buffer, size_t& index, std::string& ord)
{
	while (isspace(C))
		++index;

	size_t x = index; // origin
	if (C == '-' || isdigit(C)) // Number
	{
		bool dot = false;
		bool bSign = false;
		if (C == '-')
		{
			++index;
			bSign = true;
		}
		while (isdigit(C) || C == '.')
		{
			if (C == '.')
				dot = true;
			++index;
		}
		size_t sz = index - x;
		ord = std::string(buffer + x, sz);
		if (dot)
		{
			m_type = TYPE_REAL;
		}
		else if(bSign)
		{
			m_type = TYPE_SIGN_INTEGER;
		}
		else
		{
			m_type = TYPE_INTEGER;
		}
		return -m_type;
	}
	else 
	if (C == 34) // " - String
	{
		bool ign = false;
		x = ++index;
		while (C && (ign || C != 34))
		{
			if (ign)
			{
				ign = false;
			}
			else
			if (C == '\\')
			{
				ign = true;
			}
			++index;
		}
		size_t sz = index - x;
		ord = std::string(buffer + x, sz);
		if(ign)
			decode(ord);
		++index;
		m_type = TYPE_STRING;
		return -m_type;
	}
	else
	if (!strncmp(&buffer[index], "null", 4)) // null value
	{
		m_type = TYPE_UNKNOWN;
		ord = std::string(buffer + x, 4);
		index += 4;
		return -m_type;
	}
	else
	if (!strncmp(&buffer[index], "true", 4)) // Boolean: true
	{
		m_type = TYPE_BOOLEAN;
		ord = std::string(buffer + x, 4);
		m_d_value = 1.0;
		index += 4;
		return -m_type;
	}
	else
	if (!strncmp(&buffer[index], "false", 5)) // Boolean: false
	{
		m_type = TYPE_BOOLEAN;
		ord = std::string(buffer + x, 5);
		m_d_value = 0.0;
		index += 5;
		return -m_type;
	}
	return buffer[index++];
}


// --------------------------------------------------------------------------------
char Json::Parse(const char *buffer, size_t& index)
{
    // add by rock
    try {

	std::string ord;
	int token = Token(buffer, index, ord);
	if (token == -TYPE_REAL || token == -TYPE_INTEGER || token == -TYPE_SIGN_INTEGER)
	{
		if (token == -TYPE_REAL)
		{
			m_d_value = atof(ord.c_str());
		}
		else if(token == -TYPE_INTEGER)
		{
			m_d_value = Utility::atoi64(ord.c_str());
		}
		else if(token == -TYPE_SIGN_INTEGER)
		{
			m_d_value = Utility::atoi64_sign(ord.c_str());
		}
	}
	else
	if (token == -TYPE_STRING)
	{
		m_str_value = ord;
	}
	else
	if (token == -TYPE_UNKNOWN)
	{
	}
	else
	if (token == -TYPE_BOOLEAN)
	{
	}
	else
	if (token == '[') // Array
	{
		m_type = TYPE_ARRAY;
		while (true)
		{
			char res;
			Json o;
			if ((res = o.Parse(buffer, index)) == 0)
			{
				m_array.push_back(o);
			}
			else
			if (res == ']')
			{
				break;
			}
			else
			if (res == ',') // another element follows
			{
			}
			else
			{
				throw Exception(std::string("Unexpected end of Array: ") + res);
			}
		}
	}
	else
	if (token == ']') // end of Array
	{
		return ']';
	}
	else
	if (token == '{') // Object
	{
		m_type = TYPE_OBJECT;
		int state = 0;
		std::string element_name;
		bool quit = false;
		while (!quit)
		{
			Json o;
			char res = o.Parse(buffer, index);
			switch (state)
			{
			case 0:
				if (res == ',') // another object follow
					break;
				if (res == '}') // end of Object
				{
					quit = true;
					break;
				}
				if (res || o.Type() != TYPE_STRING)
					throw Exception("Object element name missing");
				element_name = o.GetString();
				state = 1;
				break;
			case 1:
				if (res != ':')
					throw Exception("Object element separator missing");
				state = 2;
				break;
			case 2:
				if (res)
					throw Exception(std::string("Unexpected character when parsing anytype: ") + res);
				m_object[element_name] = o;
				state = 0;
				break;
			}
		}
	}
	else
	if (token == '}') // end of Object
	{
		return '}';
	}
	else
	if (token == ',')
	{
		return ',';
	}
	else
	if (token == ':')
	{
		return ':';
	}
	else
	{
		throw Exception("Can't parse Json representation: " + std::string(&buffer[index]));
	}

    // add by itc
    }catch(Exception e) {
        return -1;
    }

	return 0;
}


// --------------------------------------------------------------------------------
std::string Json::ToString(bool quote) const
{
	switch (m_type)
	{
	case TYPE_UNKNOWN:
		break;
	case TYPE_SIGN_INTEGER:
		return Utility::bigint2string((int64_t)m_d_value);
	case TYPE_INTEGER:
		return Utility::bigint2string((uint64_t)m_d_value);
	case TYPE_REAL:
	{
		char slask[100];
		sprintf(slask, "%.2f", m_d_value);
		if(!strcmp(slask, "inf"))
			return "0.0";
		return slask;
	}
	case TYPE_STRING:
	{
		// modify by itc
		std::ostringstream tmp;
		tmp << "\"" << encode(m_str_value) << "\"";
		return tmp.str();
		//return m_str_value;
	}
	case TYPE_BOOLEAN:
		return m_d_value ? "true" : "false";
	case TYPE_ARRAY:
	{
		std::ostringstream tmp;
		bool first = true;
		tmp << "[";
		for (json_list_t::const_iterator it = m_array.begin(); it != m_array.end(); it++)
		{
			const Json& ref = *it;
			if (!first)
				tmp << ",";
			tmp << ref.ToString(quote);
			first = false;
		}
		tmp << "]";
		return tmp.str();
	}
	case TYPE_OBJECT:
	{
		std::ostringstream tmp;
		bool first = true;
		tmp << "{";
		for (json_map_t::const_iterator it = m_object.begin(); it != m_object.end(); it++)
		{
			const std::pair<std::string, Json>& ref = *it;
			if (!first)
				tmp << ",";
			if (quote)
				tmp << "\"" << encode(ref.first) << "\":" << ref.second.ToString(quote);
			else
				tmp << ref.first << ":" << ref.second.ToString(quote);
			first = false;
		}
		tmp << "}";
		return tmp.str();
	}
	}
	return "null";
}


// --------------------------------------------------------------------------------
void Json::encode(std::string& src) const
{
	size_t pos = src.find("\\");
	while (pos != std::string::npos)
	{
		src.replace(pos, 1, "\\\\");
		pos = src.find("\\", pos + 2);
	}
	pos = src.find("\r");
	while (pos != std::string::npos)
	{
		src.replace(pos, 1, "\\r");
		pos = src.find("\r", pos + 2);
	}
	pos = src.find("\n");
	while (pos != std::string::npos)
	{
		src.replace(pos, 1, "\\n");
		pos = src.find("\n", pos + 2);
	}
	pos = src.find("\"");
	while (pos != std::string::npos)
	{
		src.replace(pos, 1, "\\\"");
		pos = src.find("\"", pos + 2);
	}
}


// --------------------------------------------------------------------------------
void Json::decode(std::string& src) const
{
	size_t pos = src.find("\\\"");
	while (pos != std::string::npos)
	{
		src.replace(pos, 2, "\"");
		pos = src.find("\\\"", pos + 1);
	}
	pos = src.find("\\r");
	while (pos != std::string::npos)
	{
		src.replace(pos, 2, "\r");
		pos = src.find("\\r", pos + 1);
	}
	pos = src.find("\\n");
	while (pos != std::string::npos)
	{
		src.replace(pos, 2, "\n");
		pos = src.find("\\n", pos + 1);
	}
	pos = src.find("\\\\");
	while (pos != std::string::npos)
	{
		src.replace(pos, 2, "\\");
		pos = src.find("\\\\", pos + 1);
	}
}


// --------------------------------------------------------------------------------
std::string Json::encode(const std::string& src) const
{
	std::string tmp(src);
	encode(tmp);
	return tmp;
}

Json& Json::operator[](int iArrIdx)
{
	if (m_type != TYPE_ARRAY && m_type != TYPE_UNKNOWN)
		return s_js_inner;
	if(m_type == TYPE_UNKNOWN)
		m_type = TYPE_ARRAY;
	if(m_type != TYPE_ARRAY)
		return s_js_inner;

	if(iArrIdx < 0) {
		Json::json_list_t::reverse_iterator it = m_array.rbegin();
		iArrIdx++; // -1 表示倒数第一个,依次类推
		while(iArrIdx < 0 && it != m_array.rend()) {
			iArrIdx++;
			it++;
		}
		if(it != m_array.rend())
			return *it;
		return s_js_inner;
	}
	else if(iArrIdx < (int)m_array.size()) {
		Json::json_list_t::iterator it = m_array.begin();
		while(it != m_array.end() && iArrIdx > 0) {
			iArrIdx--;
			it++;
		}
		return *it;
	}
	else if(iArrIdx == (int)m_array.size()) {
		Json js;
		m_array.push_back(js);
		Json::json_list_t::reverse_iterator it = m_array.rbegin();
		return *it;
	}
	return s_js_inner;
}

int Json::GetArrayItemCount()
{
	if (m_type != TYPE_ARRAY && m_type != TYPE_UNKNOWN)
		return 1;
	return (int)(m_array.size());
}

// add by itc
const Json& Json::GetArrayValByIdx(int iArrIdx) const
{
	if (m_type != TYPE_ARRAY && m_type != TYPE_UNKNOWN)
		return s_js_inner;

	// 从后往前取
	if(iArrIdx < 0) {
		Json::json_list_t::const_reverse_iterator it = m_array.rbegin();
		iArrIdx++; // -1 表示倒数第一个,依次类推
		while(iArrIdx < 0 && it != m_array.rend()) {
			iArrIdx++;
			it++;
		}
		if(it != m_array.rend())
			return *it;
		return s_js_inner;
	}

	// 从前往后取
	Json::json_list_t::const_iterator it = m_array.begin();
	while(it != m_array.end() && iArrIdx > 0) {
		iArrIdx--;
		it++;
	}
	if(it != m_array.end())
		return *it;
	return s_js_inner;
}

Json::operator std::string& () 
{        
    return m_str_value;
}    

Json::operator const std::string& () const
{        
    return m_str_value;
}    

const Json& Json::operator[](int idx) const
{
	return GetArrayValByIdx(idx);
}

#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif

