/**
 **	\file Json.h
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
#ifndef _SOCKETS_Json_H
#define _SOCKETS_Json_H

#include "Exception.h"
#include <list>
#include <map>

#ifdef _WIN32
#ifdef GetObject
#undef GetObject
#endif
#endif

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

class Json
{
public:
	typedef std::list<Json> json_list_t;
	typedef std::map<std::string, Json> json_map_t;

	typedef enum {
		TYPE_UNKNOWN = 1,
		TYPE_SIGN_INTEGER,
		TYPE_INTEGER,
		TYPE_REAL,
		TYPE_STRING,
		TYPE_BOOLEAN,
		TYPE_ARRAY,
		TYPE_OBJECT
	} json_type_t;

public:
	/** Default constructor */
	Json(); //m_type(TYPE_UNKNOWN);

	/** Basic type constructors */
	Json(char value);
	Json(short value);
	Json(unsigned short value);
	Json(unsigned int value); //m_type(TYPE_INTEGER), m_i_value(value);
	Json(int value); //m_type(TYPE_SIGN_INTEGER), m_sign_i_value(value);
	Json(double value); //m_type(TYPE_REAL), m_d_value(value);
	Json(float value); //m_type(TYPE_REAL), m_d_value(value);
	Json(const char *value); //m_type(TYPE_STRING), m_str_value(value);
	Json(const std::string& value); //m_type(TYPE_STRING), m_str_value(value);
	Json(bool value); //m_type(TYPE_BOOLEAN), m_b_value(value);
	Json(uint64_t value);
	Json(int64_t value);

	/** Complex type constructor (array, object) */
	Json(json_type_t t); //m_type(t);

	virtual ~Json();

	char Parse(const char *buffer, size_t& index);

	json_type_t Type() const;

	bool HasValue(const std::string& name) const;
	void RemoveValue(const std::string& name);

	operator char() const;
	operator short() const;
	operator unsigned short() const;
	operator double() const;
	operator float() const;
	operator std::string() const;
	operator std::string& ();
	operator const char*() const;
	operator bool() const;
	operator uint64_t() const;
	operator int64_t() const;
	operator int32_t() const;
	operator uint32_t() const;
    operator const std::string& () const;

	const Json& operator[](const char *name) const;
	Json& operator[](const char *name);
	const Json& operator[](const std::string& name) const;
	Json& operator[](const std::string& name);

	void Add(const Json & data);
	const std::string& GetString() const;

	int GetArrayItemCount(); 
	const Json& GetArrayValByIdx(int iArrIdx) const;
	const Json& operator[](int idx) const;
	Json& operator[](int idx);
	const json_list_t& GetArray() const;
	json_list_t& GetArray();

	const json_map_t& GetObject() const;
	json_map_t& GetObject();

	std::string ToString(bool quote = true) const;
	void encode(std::string& src) const;
	void decode(std::string& src) const;
	std::string encode(const std::string&) const;
	int Token(const char *buffer, size_t& index, std::string& ord);

	Json & operator = (const Json & j);
	Json & operator++();

public:
	static Json Parse(const std::string& data);

private:
	json_type_t m_type;

	double m_d_value;
	std::string m_str_value;

	json_list_t m_array;
	json_map_t m_object;
};

#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif

#endif // _SOCKETS_Json_H


