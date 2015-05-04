#pragma once

/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <stdint.h>
#include <string>
#include <vector>

// whether use nxjson
// @see: https://bitbucket.org/yarosla/nxjson
#undef __SRS_JSON_USE_NXJSON
#define __SRS_JSON_USE_NXJSON

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// json decode
// 1. SrsJsonAny: read any from str:char*
//        SrsJsonAny* pany = NULL;
//        if ((ret = srs_json_read_any(str, &pany)) != ERROR_SUCCESS) {
//            return ret;
//         }
//        srs_assert(pany); // if success, always valid object.
// 2. SrsJsonAny: convert to specifid type, for instance, string
//        SrsJsonAny* pany = ...
//        if (pany->is_string()) {
//            string v = pany->to_str();
//        }
//
// for detail usage, see interfaces of each object.
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
// @see: https://bitbucket.org/yarosla/nxjson
// @see: https://github.com/udp/json-parser

class SrsJsonArray;
class SrsJsonObject;

class SrsJsonAny
{
public:
    char marker;
// donot directly create this object,
// instead, for examle, use SrsJsonAny::str() to create a concreated one.
protected:
    SrsJsonAny();
public:
    virtual ~SrsJsonAny();
public:
    virtual bool is_string();
    virtual bool is_boolean();
    virtual bool is_integer();
    virtual bool is_number();
    virtual bool is_object();
    virtual bool is_array();
    virtual bool is_null();
public:
    /**
    * get the string of any when is_string() indicates true.
    * user must ensure the type is a string, or assert failed.
    */
    virtual std::string to_str();
    /**
    * get the boolean of any when is_boolean() indicates true.
    * user must ensure the type is a boolean, or assert failed.
    */
    virtual bool to_boolean();
    /**
    * get the integer of any when is_integer() indicates true.
    * user must ensure the type is a integer, or assert failed.
    */
    virtual int64_t to_integer();
    /**
    * get the number of any when is_number() indicates true.
    * user must ensure the type is a number, or assert failed.
    */
    virtual double to_number();
    /**
    * get the object of any when is_object() indicates true.
    * user must ensure the type is a object, or assert failed.
    */
    virtual SrsJsonObject* to_object();
    /**
    * get the ecma array of any when is_ecma_array() indicates true.
    * user must ensure the type is a ecma array, or assert failed.
    */
    virtual SrsJsonArray* to_array();
public:
    static SrsJsonAny* str(const char* value = NULL);
    static SrsJsonAny* boolean(bool value = false);
    static SrsJsonAny* ingeter(int64_t value = 0);
    static SrsJsonAny* number(double value = 0.0);
    static SrsJsonAny* null();
    static SrsJsonObject* object();
    static SrsJsonArray* array();
public:
    /**
    * read json tree from str:char*
    */
    static SrsJsonAny* loads(char* str);
};

class SrsJsonObject : public SrsJsonAny
{
private:
    typedef std::pair<std::string, SrsJsonAny*> SrsJsonObjectPropertyType;
    std::vector<SrsJsonObjectPropertyType> properties;
private:
    // use SrsJsonAny::object() to create it.
    friend class SrsJsonAny;
    SrsJsonObject();
public:
    virtual ~SrsJsonObject();
public:
    virtual int count();
    // @remark: max index is count().
    virtual std::string key_at(int index);
    // @remark: max index is count().
    virtual SrsJsonAny* value_at(int index);
public:
    virtual void set(std::string key, SrsJsonAny* value);
    virtual SrsJsonAny* get_property(std::string name);
    virtual SrsJsonAny* ensure_property_string(std::string name);
};

class SrsJsonArray : public SrsJsonAny
{
private:
    std::vector<SrsJsonAny*> properties;

private:
    // use SrsJsonAny::array() to create it.
    friend class SrsJsonAny;
    SrsJsonArray();
public:
    virtual ~SrsJsonArray();
public:
    virtual int count();
    // @remark: max index is count().
    virtual SrsJsonAny* at(int index);
    virtual void add(SrsJsonAny* value);
};

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/* json encode
    cout<< __SRS_JOBJECT_START
            << __SRS_JFIELD_STR("name", "srs") << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("version", 100) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_NAME("features") << __SRS_JOBJECT_START
                << __SRS_JFIELD_STR("rtmp", "released") << __SRS_JFIELD_CONT
                << __SRS_JFIELD_STR("hls", "released") << __SRS_JFIELD_CONT
                << __SRS_JFIELD_STR("dash", "plan")
            << __SRS_JOBJECT_END << __SRS_JFIELD_CONT
            << __SRS_JFIELD_STR("author", "srs team")
        << __SRS_JOBJECT_END
it's:
    cont<< "{"
            << "name:" << "srs" << ","
            << "version:" << 100 << ","
            << "features:" << "{"
                << "rtmp:" << "released" << ","
                << "hls:" << "released" << ","
                << "dash:" << "plan"
            << "}" << ","
            << "author:" << "srs team"
        << "}"
that is:
    """
        {
            "name": "srs",
            "version": 100,
            "features": {
                "rtmp": "released",
                "hls": "released",
                "dash": "plan"
            },
            "author": "srs team"
        }
    """
*/
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
#define __SRS_JOBJECT_START "{"
#define __SRS_JFIELD_NAME(k) "\"" << k << "\":"
#define __SRS_JFIELD_STR(k, v) "\"" << k << "\":\"" << v << "\""
#define __SRS_JFIELD_ORG(k, v) "\"" << k << "\":" << std::dec << v
#define __SRS_JFIELD_ERROR(ret) "\"" << "code" << "\":" << ret
#define __SRS_JFIELD_CONT ","
#define __SRS_JOBJECT_END "}"
#define __SRS_JARRAY_START "["
#define __SRS_JARRAY_END "]"

#ifdef __SRS_JSON_USE_NXJSON

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Copyright (c) 2013 Yaroslav Stavnichiy <yarosla@gmail.com>
 *
 * This file is part of NXJSON.
 *
 * NXJSON is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * NXJSON is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NXJSON. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NXJSON_H
#define NXJSON_H

#ifdef  __cplusplus
extern "C" {
#endif


typedef enum nx_json_type {
    NX_JSON_NULL,    // this is null value
    NX_JSON_OBJECT,  // this is an object; properties can be found in child nodes
    NX_JSON_ARRAY,   // this is an array; items can be found in child nodes
    NX_JSON_STRING,  // this is a string; value can be found in text_value field
    NX_JSON_INTEGER, // this is an integer; value can be found in int_value field
    NX_JSON_DOUBLE,  // this is a double; value can be found in dbl_value field
    NX_JSON_BOOL     // this is a boolean; value can be found in int_value field
} nx_json_type;

typedef struct nx_json {
    nx_json_type type;       // type of json node, see above
    const char* key;         // key of the property; for object's children only
    const char* text_value;  // text value of STRING node
    long int_value;          // the value of INTEGER or BOOL node
    double dbl_value;        // the value of DOUBLE node
    int length;              // number of children of OBJECT or ARRAY
    struct nx_json* child;   // points to first child
    struct nx_json* next;    // points to next child
    struct nx_json* last_child;
} nx_json;

typedef int (*nx_json_unicode_encoder)(unsigned int codepoint, char* p, char** endp);

extern nx_json_unicode_encoder nx_json_unicode_to_utf8;

const nx_json* nx_json_parse(char* text, nx_json_unicode_encoder encoder);
const nx_json* nx_json_parse_utf8(char* text);
void nx_json_free(const nx_json* js);
const nx_json* nx_json_get(const nx_json* json, const char* key); // get object's property by key
const nx_json* nx_json_item(const nx_json* json, int idx); // get array element by index


#ifdef  __cplusplus
}
#endif

#endif  /* NXJSON_H */

#endif /*__SRS_JSON_USE_NXJSON*/
