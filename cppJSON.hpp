/*******************************************************************************
 * @file     cppJSON.hpp
 * @brief    cppJSON - lightweight C++ JSON library (global class, no namespace)
 * @note     Derived from Bwar/CJsonObject (formerly neb::CJsonObject).
 *           Original author: bwarliao (2014).
 *           Reworked by AI coding tools (Claude Code) — see README.md.
 *           JSON core: cJSON v1.7.19 (MIT) + patches; float printing: Ryu.
 *           See README.md and THIRD_PARTY_LICENSES.md.
 * @date     2014-7-16 (original) / 2026-08-02 (cppJSON rename & extensions)
 * @author   bwarliao (original), cppJSON contributors
 ******************************************************************************/

#ifndef CPPJSON_HPP_
#define CPPJSON_HPP_

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <malloc.h>
#include <limits.h>
#include <math.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <cstddef>
#include <type_traits>
#if __cplusplus >= 201101L
#include <unordered_map>
#endif
#ifdef __cplusplus
extern "C" {
#endif
#include "cJSON.h"
#ifdef __cplusplus
}
#endif

#ifndef INT32                    /* old cJSON defined these types, new cJSON does not */
typedef int32_t int32;
#endif
#ifndef UINT32
typedef uint32_t uint32;
#endif
#ifndef INT64
typedef int64_t int64;
#endif
#ifndef UINT64
typedef uint64_t uint64;
#endif


#if __cplusplus >= 201101L
/* --- STL container detection (SFINAE, C++11) --- */
template <typename...>
struct CJsonVoid
{
    typedef void type;
};
template <typename... T>
using cjson_void_t = typename CJsonVoid<T...>::type;

/* a map-style container has both key_type and mapped_type */
template <typename T, typename = void>
struct CJsonIsMap : std::false_type
{
};
template <typename T>
struct CJsonIsMap<T, cjson_void_t<typename T::key_type, typename T::mapped_type> > : std::true_type
{
};

/* a sequence-style container has value_type and is neither a map nor a string */
template <typename T, typename = void>
struct CJsonIsSeq : std::false_type
{
};
template <typename T>
struct CJsonIsSeq<T, cjson_void_t<typename T::value_type> > :
    std::integral_constant<bool, !CJsonIsMap<T>::value && !std::is_same<T, std::string>::value && !std::is_same<T, const char*>::value>
{
};

/* a sequence container that supports reserve() (std::vector) */
template <typename T, typename = void>
struct CJsonHasReserve : std::false_type
{
};
template <typename T>
struct CJsonHasReserve<T, cjson_void_t<decltype(std::declval<T&>().reserve(static_cast<size_t>(0)))> > : std::true_type
{
};
template <typename T>
typename std::enable_if<CJsonHasReserve<T>::value, void>::type
CJsonReserveSize(T& result, size_t size)
{
    result.reserve(size);
}
template <typename T>
typename std::enable_if<!CJsonHasReserve<T>::value, void>::type
CJsonReserveSize(T& result, size_t size)
{
    (void)result;
    (void)size;
}

#endif

class cppJSON
{
public:     // method of ordinary json object or json array
    cppJSON();
    cppJSON(const std::string& strJson);
    cppJSON(const cppJSON* pJsonObject);
    cppJSON(const cppJSON& oJsonObject);
#if __cplusplus >= 201101L
    cppJSON(cppJSON&& oJsonObject);
#endif
    virtual ~cppJSON();

    cppJSON& operator=(const cppJSON& oJsonObject);
#if __cplusplus >= 201101L
    cppJSON& operator=(cppJSON&& oJsonObject);
#endif
    /* map-style value assignment: a["key"] = value, a[i] = value.
     * works with operator[] which auto-creates the key in its parent (map semantics). */
    cppJSON& operator=(int32 iValue);
    cppJSON& operator=(uint32 uiValue);
    cppJSON& operator=(int64 llValue);
    cppJSON& operator=(uint64 ullValue);
    cppJSON& operator=(float fValue);
    cppJSON& operator=(double dValue);
    cppJSON& operator=(bool bValue);
    cppJSON& operator=(const std::string& strValue);
    cppJSON& operator=(const char* strValue);
#if __cplusplus >= 201101L
    /* o["k"] = { vector / list / deque / set / map / unordered_map } */
    template <typename T>
    typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, cppJSON&>::type
    operator=(const T& container);
#endif
    /* ArduinoJson-style default-value read:  auto v = obj["key"] | 42;
     * NOTE: the expression evaluates obj["key"] first. On a non-const object that
     * operator[] has map semantics (it creates the key if missing). For a
     * side-effect-free read use the const overload (const auto& ro = obj; ...)
     * or Get(key, defval). */
    template <typename T>
    T operator|(T defval) const
    {
        cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
        return(ExtractValue(pNode, defval));
    }
#if __cplusplus >= 201101L
    /* o["k"] | nullptr : ArduinoJson-style pointer default. Returns the internal
     * string pointer when the value is a JSON string, otherwise nullptr.
     * NOTE: NULL is plain 0 in C++ and takes the numeric default path (T=int);
     * use nullptr for the pointer semantics. */
    const char* operator|(std::nullptr_t) const
    {
        cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
        if (pNode == NULL)
        {
            return(NULL);
        }
        if (pNode->type & cJSON_String)
        {
            return(pNode->valuestring);
        }
        return(NULL);
    }
#endif
    bool operator==(const cppJSON& oJsonObject) const;
    bool Parse(const std::string& strJson);
    void Clear();
    bool IsEmpty() const;
    bool IsArray() const;
    std::string ToString() const;
    std::string ToFormattedString() const;
    /* deep copy of the underlying cJSON node tree; caller owns the result (free with cJSON_Delete), NULL if empty */
    cJSON* DuplicateNode() const;
    /* adopt a deep copy of the given cJSON node as this object's data (used by container reads) */
    void FromNode(cJSON* pNode);
    const std::string& GetErrMsg() const
    {
        return(m_strErrMsg);
    }

public:     // method of ordinary json object
    bool AddEmptySubObject(const std::string& strKey);
    bool AddEmptySubArray(const std::string& strKey);
    bool GetKey(std::string& strKey);
    void ResetTraversing();
    cppJSON& operator[](const std::string& strKey);
    /* const (read-only) access: never creates the key. Use with a const reference
     * to keep auto v = obj["key"] | defval side-effect free. */
    const cppJSON& operator[](const std::string& strKey) const;
    std::string operator()(const std::string& strKey) const;
    bool KeyExist(const std::string& strKey) const;
    int ValueType(const std::string& strKey) const;
    bool Get(const std::string& strKey, cppJSON& oJsonObject) const;
    bool Get(const std::string& strKey, std::string& strValue) const;
    bool Get(const std::string& strKey, int32& iValue) const;
    bool Get(const std::string& strKey, uint32& uiValue) const;
    bool Get(const std::string& strKey, int64& llValue) const;
    bool Get(const std::string& strKey, uint64& ullValue) const;
    bool Get(const std::string& strKey, bool& bValue) const;
    bool Get(const std::string& strKey, float& fValue) const;
    bool Get(const std::string& strKey, double& dValue) const;
#if __cplusplus >= 201101L
    /* o.Get("k", vector&) / ("k", map&): extract a JSON array/object into an STL container */
    template <typename T>
    typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
    Get(const std::string& strKey, T& container) const;
    template <typename T>
    std::vector<T> ToVector() const;
    template <typename K, typename V>
    std::map<K, V> ToMap() const;
#endif
    /* default-value read: side-effect free (no key creation). Returns the stored
     * value converted to T if present and type-compatible, else defval. */
    template <typename T, typename std::enable_if<!CJsonIsSeq<T>::value && !CJsonIsMap<T>::value, int>::type = 0>
    T Get(const std::string& strKey, T defval) const
    {
        cJSON* pJsonStruct = NULL;
        if (m_pJsonData != NULL)
        {
            if (m_pJsonData->type & cJSON_Object)
            {
                pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
            }
        }
        else if (m_pExternJsonDataRef != NULL)
        {
            if (m_pExternJsonDataRef->type & cJSON_Object)
            {
                pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
            }
        }
        return(ExtractValue(pJsonStruct, defval));
    }
    bool IsNull(const std::string& strKey) const;
    bool Add(const std::string& strKey, const cppJSON& oJsonObject);
#if __cplusplus < 201101L
    bool AddWithMove(const std::string& strKey, cppJSON& oJsonObject);
#else
    bool Add(const std::string& strKey, cppJSON&& oJsonObject);
#endif
    bool Add(const std::string& strKey, const std::string& strValue);
    bool Add(const std::string& strKey, int32 iValue);
    bool Add(const std::string& strKey, uint32 uiValue);
    bool Add(const std::string& strKey, int64 llValue);
    bool Add(const std::string& strKey, uint64 ullValue);
    bool Add(const std::string& strKey, bool bValue, bool bValueAgain);
    bool Add(const std::string& strKey, float fValue);
    bool Add(const std::string& strKey, double dValue);
#if __cplusplus >= 201101L
    /* o.Add("k", vector) / ("k", map): convert an STL container into a JSON sub-value */
    template <typename T>
    typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
    Add(const std::string& strKey, const T& container);
#endif
    bool AddNull(const std::string& strKey);    // add null like this:   "key":null
    bool Delete(const std::string& strKey);
    bool Replace(const std::string& strKey, const cppJSON& oJsonObject);
#if __cplusplus < 201101L
    bool ReplaceWithMove(const std::string& strKey, cppJSON& oJsonObject);
#else
    bool Replace(const std::string& strKey, cppJSON&& oJsonObject);
#endif
    bool Replace(const std::string& strKey, const std::string& strValue);
    bool Replace(const std::string& strKey, int32 iValue);
    bool Replace(const std::string& strKey, uint32 uiValue);
    bool Replace(const std::string& strKey, int64 llValue);
    bool Replace(const std::string& strKey, uint64 ullValue);
    bool Replace(const std::string& strKey, bool bValue, bool bValueAgain);
    bool Replace(const std::string& strKey, float fValue);
    bool Replace(const std::string& strKey, double dValue);
    bool ReplaceWithNull(const std::string& strKey);    // replace value with null
#if __cplusplus < 201101L
    bool ReplaceAdd(const std::string& strKey, const cppJSON& oJsonObject);
    bool ReplaceAdd(const std::string& strKey, const std::string& strValue);
    template <typename T>
    bool ReplaceAdd(const std::string& strKey, T value) 
    {
        if (KeyExist(strKey))
        {
            return(Replace(strKey, value));
        }
        return(Add(strKey, value));
    }
#else
    template <typename T>
    bool ReplaceAdd(const std::string& strKey, T&& value)
    {
        if (KeyExist(strKey))
        {
            return(Replace(strKey, std::forward<T>(value)));
        }
        return(Add(strKey, std::forward<T>(value)));
    }
#endif

public:     // method of json array
    int GetArraySize() const;
    /* sequential array traversal (O(n) total, vs O(n^2) for Get(i) in a loop).
     * call ResetArrayTraversing() first, then GetNextValue() until it returns false. */
    void ResetArrayTraversing();
    bool GetNextValue(std::string& strValue);
    bool GetNextValue(int32& iValue);
    bool GetNextValue(uint32& uiValue);
    bool GetNextValue(int64& llValue);
    bool GetNextValue(uint64& ullValue);
    bool GetNextValue(float& fValue);
    bool GetNextValue(double& dValue);
    bool GetNextValue(cppJSON& oJsonObject);
    cppJSON& operator[](unsigned int uiWhich);
    const cppJSON& operator[](unsigned int uiWhich) const;
    std::string operator()(unsigned int uiWhich) const;
    int ValueType(int iWhich) const;
    bool Get(int iWhich, cppJSON& oJsonObject) const;
    bool Get(int iWhich, std::string& strValue) const;
    bool Get(int iWhich, int32& iValue) const;
    bool Get(int iWhich, uint32& uiValue) const;
    bool Get(int iWhich, int64& llValue) const;
    bool Get(int iWhich, uint64& ullValue) const;
    bool Get(int iWhich, bool& bValue) const;
    bool Get(int iWhich, float& fValue) const;
    bool Get(int iWhich, double& dValue) const;
#if __cplusplus >= 201101L
    /* o.Get(i, vector&) / (i, map&): extract array element i into an STL container */
    template <typename T>
    typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
    Get(int iWhich, T& container) const;
#endif
    bool IsNull(int iWhich) const;
    bool Add(const cppJSON& oJsonObject);
#if __cplusplus < 201101L
    bool AddWithMove(cppJSON& oJsonObject);
#else
    bool Add(cppJSON&& oJsonObject);
#endif
    bool Add(const std::string& strValue);
    bool Add(int32 iValue);
    bool Add(uint32 uiValue);
    bool Add(int64 llValue);
    bool Add(uint64 ullValue);
    bool Add(int iAnywhere, bool bValue);
    bool Add(float fValue);
    bool Add(double dValue);
#if __cplusplus >= 201101L
    /* o.Add(vector) / (map): append an STL container as the next array element */
    template <typename T>
    typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
    Add(const T& container);
#endif
    bool AddNull();   // add a null value
    bool AddAsFirst(const cppJSON& oJsonObject);
#if __cplusplus < 201101L
    bool AddAsFirstWithMove(cppJSON& oJsonObject);
#else
    bool AddAsFirst(cppJSON&& oJsonObject);
#endif
    bool AddAsFirst(const std::string& strValue);
    bool AddAsFirst(int32 iValue);
    bool AddAsFirst(uint32 uiValue);
    bool AddAsFirst(int64 llValue);
    bool AddAsFirst(uint64 ullValue);
    bool AddAsFirst(int iAnywhere, bool bValue);
    bool AddAsFirst(float fValue);
    bool AddAsFirst(double dValue);
    bool AddNullAsFirst();     // add a null value
    bool Delete(int iWhich);
    bool Replace(int iWhich, const cppJSON& oJsonObject);
#if __cplusplus < 201101L
    bool ReplaceWithMove(int iWhich, cppJSON& oJsonObject);
#else
    bool Replace(int iWhich, cppJSON&& oJsonObject);
#endif
    bool Replace(int iWhich, const std::string& strValue);
    bool Replace(int iWhich, int32 iValue);
    bool Replace(int iWhich, uint32 uiValue);
    bool Replace(int iWhich, int64 llValue);
    bool Replace(int iWhich, uint64 ullValue);
    bool Replace(int iWhich, bool bValue, bool bValueAgain);
    bool Replace(int iWhich, float fValue);
    bool Replace(int iWhich, double dValue);
    bool ReplaceWithNull(int iWhich);      // replace with a null value

private:
    cppJSON(cJSON* pJsonData);
    cJSON* GetNextArrayItem();   // returns current array element and advances the cursor
    void SetNodeAsNumber(int64_t valueint, double valuedouble, int sign);  // in-place node rewrite (keeps key/position)
    void SetNodeAsString(const char* strValue);
    void SetNodeAsBool(bool bValue);
    void SetNodeFromCJson(cJSON* pNew);   // steal payload from pNew into current node (no deep copy)
    /* extract value as T from a cJSON node, else return defval (SFINAE-selected) */
    template <typename T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, int>::type = 0>
    T ExtractValue(cJSON* pJsonStruct, T defval) const
    {
        if (pJsonStruct == NULL)
        {
            return(defval);
        }
        if (pJsonStruct->type & cJSON_Number)
        {
            return((T)((pJsonStruct->sign != 0) ? pJsonStruct->valueint : (int64_t)pJsonStruct->valuedouble));
        }
        return(defval);
    }
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    T ExtractValue(cJSON* pJsonStruct, T defval) const
    {
        if (pJsonStruct == NULL)
        {
            return(defval);
        }
        if (pJsonStruct->type & cJSON_Number)
        {
            return((T)pJsonStruct->valuedouble);
        }
        return(defval);
    }
    template <typename T, typename std::enable_if<std::is_same<T, bool>::value, int>::type = 0>
    T ExtractValue(cJSON* pJsonStruct, T defval) const
    {
        if (pJsonStruct == NULL)
        {
            return(defval);
        }
        if (pJsonStruct->type & (cJSON_True | cJSON_False))
        {
            return((T)((pJsonStruct->type & cJSON_True) != 0));
        }
        return(defval);
    }
    template <typename T, typename std::enable_if<std::is_same<T, std::string>::value, int>::type = 0>
    T ExtractValue(cJSON* pJsonStruct, T defval) const
    {
        if (pJsonStruct == NULL)
        {
            return(defval);
        }
        if (pJsonStruct->type & cJSON_String)
        {
            return(std::string(pJsonStruct->valuestring));
        }
        return(defval);
    }
    template <typename T, typename std::enable_if<std::is_same<T, const char*>::value, int>::type = 0>
    T ExtractValue(cJSON* pJsonStruct, T defval) const
    {
        if (pJsonStruct == NULL)
        {
            return(defval);
        }
        if (pJsonStruct->type & cJSON_String)
        {
            return(pJsonStruct->valuestring);
        }
        return(defval);
    }
    /* any other T: no conversion, always the default */
    template <typename T, typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value && !std::is_same<T, bool>::value && !std::is_same<T, std::string>::value && !std::is_same<T, const char*>::value, int>::type = 0>
    T ExtractValue(cJSON* pJsonStruct, T defval) const
    {
        (void)pJsonStruct;
        return(defval);
    }

private:
    cJSON* m_pJsonData;
    cJSON* m_pExternJsonDataRef;
    cJSON* m_pKeyTravers;
    cJSON* m_pArrayTravers;   // sequential array traversal cursor
    const char* mc_pError;
    std::string m_strErrMsg;
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*> m_mapJsonArrayRef;
    std::map<unsigned int, cppJSON*>::iterator m_array_iter;
    std::map<std::string, cppJSON*> m_mapJsonObjectRef;
    std::map<std::string, cppJSON*>::iterator m_object_iter;
#else
    mutable std::unordered_map<unsigned int, cppJSON*> m_mapJsonArrayRef;
    mutable std::unordered_map<std::string, cppJSON*>::iterator m_object_iter;
    mutable std::unordered_map<std::string, cppJSON*> m_mapJsonObjectRef;
    mutable std::unordered_map<unsigned int, cppJSON*>::iterator m_array_iter;
#endif
};

#if __cplusplus >= 201101L
/* ---------- STL container <-> cJSON conversion helpers (C++11) ---------- */

/* forward declarations so recursive conversion (nested containers) sees all overloads */
template <typename T>
typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, cJSON*>::type
CJsonValueToNode(const T& v);
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, cJSON*>::type
CJsonValueToNode(const T& v);
inline cJSON* CJsonValueToNode(bool v);
inline cJSON* CJsonValueToNode(const std::string& v);
inline cJSON* CJsonValueToNode(const char* v);
inline cJSON* CJsonValueToNode(const cppJSON& v);
template <typename T>
typename std::enable_if<CJsonIsMap<T>::value, cJSON*>::type
CJsonValueToNode(const T& m);
template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value, cJSON*>::type
CJsonValueToNode(const T& s);

/* scalar -> cJSON node */
template <typename T>
typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, cJSON*>::type
CJsonValueToNode(const T& v)
{
    return(cJSON_CreateInt64((int64_t)v));
}
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, cJSON*>::type
CJsonValueToNode(const T& v)
{
    return(cJSON_CreateNumber((double)v));
}
inline cJSON* CJsonValueToNode(bool v) { return(cJSON_CreateBool(v)); }
inline cJSON* CJsonValueToNode(const std::string& v) { return(cJSON_CreateString(v.c_str())); }
inline cJSON* CJsonValueToNode(const char* v) { return(cJSON_CreateString(v)); }
inline cJSON* CJsonValueToNode(const cppJSON& v) { return(v.DuplicateNode()); }

/* map container -> JSON object node */
template <typename T>
typename std::enable_if<CJsonIsMap<T>::value, cJSON*>::type
CJsonValueToNode(const T& m)
{
    cJSON* obj = cJSON_CreateObject();
    if (obj == NULL)
    {
        return(NULL);
    }
    for (typename T::const_iterator it = m.begin(); it != m.end(); ++it)
    {
        cJSON* item = CJsonValueToNode(it->second);
        if (item == NULL || !cJSON_AddItemToObject(obj, it->first.c_str(), item))
        {
            if (item != NULL)
            {
                cJSON_Delete(item);
            }
            cJSON_Delete(obj);
            return(NULL);
        }
    }
    return(obj);
}

/* sequence container -> JSON array node */
template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value, cJSON*>::type
CJsonValueToNode(const T& s)
{
    cJSON* arr = cJSON_CreateArray();
    if (arr == NULL)
    {
        return(NULL);
    }
    for (typename T::const_iterator it = s.begin(); it != s.end(); ++it)
    {
        cJSON* item = CJsonValueToNode(*it);
        if (item == NULL || !cJSON_AddItemToArray(arr, item))
        {
            if (item != NULL)
            {
                cJSON_Delete(item);
            }
            cJSON_Delete(arr);
            return(NULL);
        }
    }
    return(arr);
}

/* forward declarations for recursive conversion */
template <typename T>
typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type
CJsonNodeToValue(cJSON* pNode);
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
CJsonNodeToValue(cJSON* pNode);
template <typename T>
typename std::enable_if<std::is_same<T, bool>::value, T>::type
CJsonNodeToValue(cJSON* pNode);
template <typename T>
typename std::enable_if<std::is_same<T, std::string>::value, T>::type
CJsonNodeToValue(cJSON* pNode);
template <typename T>
typename std::enable_if<std::is_same<T, const char*>::value, T>::type
CJsonNodeToValue(cJSON* pNode);
template <typename T>
typename std::enable_if<CJsonIsMap<T>::value, T>::type
CJsonNodeToValue(cJSON* pNode);
template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value, T>::type
CJsonNodeToValue(cJSON* pNode);
template <typename T>
typename std::enable_if<std::is_same<T, cppJSON>::value, T>::type
CJsonNodeToValue(cJSON* pNode);

/* cJSON node -> scalar */
template <typename T>
typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    if (pNode == NULL || !(pNode->type & cJSON_Number))
    {
        return((T)0);
    }
    return((T)((pNode->sign != 0) ? pNode->valueint : (int64_t)pNode->valuedouble));
}
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    if (pNode == NULL || !(pNode->type & cJSON_Number))
    {
        return((T)0.0);
    }
    return((T)pNode->valuedouble);
}
template <typename T>
typename std::enable_if<std::is_same<T, bool>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    return((T)(pNode != NULL && (pNode->type & cJSON_True)));
}
template <typename T>
typename std::enable_if<std::is_same<T, std::string>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    if (pNode == NULL || !(pNode->type & cJSON_String) || pNode->valuestring == NULL)
    {
        return(std::string());
    }
    return(std::string(pNode->valuestring));
}
template <typename T>
typename std::enable_if<std::is_same<T, const char*>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    if (pNode == NULL || !(pNode->type & cJSON_String))
    {
        return(NULL);
    }
    return(pNode->valuestring);
}

/* cJSON node -> map container */
template <typename T>
typename std::enable_if<CJsonIsMap<T>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    T result;
    if (pNode == NULL || !(pNode->type & cJSON_Object))
    {
        return(result);
    }
    for (cJSON* c = pNode->child; c != NULL; c = c->next)
    {
        if (c->string == NULL)
        {
            continue;
        }
        result[typename T::key_type(c->string)] = CJsonNodeToValue<typename T::mapped_type>(c);
    }
    return(result);
}

/* cJSON node -> cppJSON */
template <typename T>
typename std::enable_if<std::is_same<T, cppJSON>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    T result;
    if (pNode != NULL)
    {
        result.FromNode(pNode);
    }
    return(result);
}

/* cJSON node -> sequence container */
template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value, T>::type
CJsonNodeToValue(cJSON* pNode)
{
    T result;
    if (pNode == NULL || !(pNode->type & cJSON_Array))
    {
        return(result);
    }
    /* pre-reserve capacity for vectors to avoid repeated reallocation */
    size_t count = 0;
    for (cJSON* c = pNode->child; c != NULL; c = c->next)
    {
        count++;
    }
    CJsonReserveSize(result, count);
    for (cJSON* c = pNode->child; c != NULL; c = c->next)
    {
        result.insert(result.end(), CJsonNodeToValue<typename T::value_type>(c));
    }
    return(result);
}

/* ---------- cppJSON STL container member templates ---------- */
template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
cppJSON::Add(const std::string& strKey, const T& container)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateObject();
        m_pKeyTravers = m_pJsonData;
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (!(pFocusData->type & cJSON_Object))
    {
        m_strErrMsg = "not a json object! json array?";
        return(false);
    }
    if (cJSON_GetObjectItem(pFocusData, strKey.c_str()) != NULL)
    {
        m_strErrMsg = "key exists!";
        return(false);
    }
    cJSON* pJsonStruct = CJsonValueToNode(container);
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "convert container to json error!";
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        cJSON_Delete(pJsonStruct);
        m_strErrMsg = "add container to json error!";
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, cppJSON&>::type
cppJSON::operator=(const T& container)
{
    cJSON* pNode = CJsonValueToNode(container);
    if (pNode != NULL)
    {
        /* steal the built node into the current node (no duplicate, no re-parse) */
        SetNodeFromCJson(pNode);
    }
    return(*this);
}

template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
cppJSON::Add(const T& container)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData != NULL)
    {
        pFocusData = m_pJsonData;
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
        m_pJsonData = cJSON_CreateArray();
        pFocusData = m_pJsonData;
    }
    if (pFocusData == NULL)
    {
        m_strErrMsg = "json data is null!";
        return(false);
    }
    if (!(pFocusData->type & cJSON_Array))
    {
        m_strErrMsg = "not a json array! json object?";
        return(false);
    }
    cJSON* pJsonStruct = CJsonValueToNode(container);
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "convert container to json error!";
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        cJSON_Delete(pJsonStruct);
        m_strErrMsg = "add container to json error!";
        return(false);
    }
    return(true);
}

template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
cppJSON::Get(const std::string& strKey, T& container) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pJsonData, strKey.c_str());
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if (m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    container = CJsonNodeToValue<T>(pJsonStruct);
    return(true);
}

template <typename T>
typename std::enable_if<CJsonIsSeq<T>::value || CJsonIsMap<T>::value, bool>::type
cppJSON::Get(int iWhich, T& container) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, iWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if (m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    container = CJsonNodeToValue<T>(pJsonStruct);
    return(true);
}

template <typename T>
std::vector<T> cppJSON::ToVector() const
{
    cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
    return(CJsonNodeToValue<std::vector<T> >(pNode));
}

template <typename K, typename V>
std::map<K, V> cppJSON::ToMap() const
{
    cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
    return(CJsonNodeToValue<std::map<K, V> >(pNode));
}
#endif

#endif /* CPPJSON_HPP_ */
