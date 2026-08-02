/*******************************************************************************
 * @file     cppJSON.cpp
 * @brief    cppJSON - lightweight C++ JSON library implementation
 * @note     Derived from Bwar/CJsonObject (formerly neb::CJsonObject).
 *           Original author: bwarliao (2014).
 *           Reworked by AI coding tools (Claude Code) — see README.md.
 *           JSON core: cJSON v1.7.19 (MIT) + patches; float printing: Ryu.
 *           See README.md and THIRD_PARTY_LICENSES.md.
 * @date     2014-7-16 (original) / 2026-08-02 (cppJSON rename & extensions)
 * @author   bwarliao (original), cppJSON contributors
 ******************************************************************************/

#include "cppJSON.hpp"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define snprintf _snprintf_s
#endif


cppJSON::cppJSON()
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL), m_pKeyTravers(NULL), m_pArrayTravers(NULL)
{
    // m_pJsonData = cJSON_CreateObject();  
    m_array_iter = m_mapJsonArrayRef.end();
    m_object_iter = m_mapJsonObjectRef.end();
}

cppJSON::cppJSON(const std::string& strJson)
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL), m_pKeyTravers(NULL), m_pArrayTravers(NULL)
{
    m_array_iter = m_mapJsonArrayRef.end();
    m_object_iter = m_mapJsonObjectRef.end();
    Parse(strJson);
}

cppJSON::cppJSON(const cppJSON* pJsonObject)
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL), m_pKeyTravers(NULL), m_pArrayTravers(NULL)
{
    if (pJsonObject)
    {
        m_array_iter = m_mapJsonArrayRef.end();
        m_object_iter = m_mapJsonObjectRef.end();
        Parse(pJsonObject->ToString());
    }
}

cppJSON::cppJSON(const cppJSON& oJsonObject)
    : m_pJsonData(NULL), m_pExternJsonDataRef(NULL), m_pKeyTravers(NULL), m_pArrayTravers(NULL)
{
    m_array_iter = m_mapJsonArrayRef.end();
    m_object_iter = m_mapJsonObjectRef.end();
    Parse(oJsonObject.ToString());
}

#if __cplusplus >= 201101L
cppJSON::cppJSON(cppJSON&& oJsonObject)
    : m_pJsonData(oJsonObject.m_pJsonData),
      m_pExternJsonDataRef(oJsonObject.m_pExternJsonDataRef),
      m_pKeyTravers(oJsonObject.m_pKeyTravers),
      m_pArrayTravers(oJsonObject.m_pArrayTravers),
      mc_pError(oJsonObject.mc_pError)
{
    oJsonObject.m_pJsonData = NULL;
    oJsonObject.m_pExternJsonDataRef = NULL;
    oJsonObject.m_pKeyTravers = NULL;
    oJsonObject.m_pArrayTravers = NULL;
    oJsonObject.mc_pError = NULL;
    m_strErrMsg = std::move(oJsonObject.m_strErrMsg);
    m_mapJsonArrayRef = std::move(oJsonObject.m_mapJsonArrayRef);
    m_mapJsonObjectRef = std::move(oJsonObject.m_mapJsonObjectRef);
    m_array_iter = m_mapJsonArrayRef.end();
    m_object_iter = m_mapJsonObjectRef.end();
}
#endif

cppJSON::~cppJSON()
{
    Clear();
}

cppJSON& cppJSON::operator=(const cppJSON& oJsonObject)
{
    if (m_pExternJsonDataRef != NULL)
    {
        /* map semantics: this object wraps a node inside a parent (e.g. a["key"]),
         * so copy the source content into that node in place, keeping key & position. */
        cJSON* pSrc = (oJsonObject.m_pJsonData != NULL) ? oJsonObject.m_pJsonData : oJsonObject.m_pExternJsonDataRef;
        if (pSrc == NULL)
        {
            /* empty source -> replace with null */
            if (m_pExternJsonDataRef->valuestring != NULL)
            {
                cJSON_free(m_pExternJsonDataRef->valuestring);
                m_pExternJsonDataRef->valuestring = NULL;
            }
            if (m_pExternJsonDataRef->child != NULL)
            {
                cJSON_Delete(m_pExternJsonDataRef->child);
                m_pExternJsonDataRef->child = NULL;
            }
            m_pExternJsonDataRef->type = cJSON_NULL;
            m_pExternJsonDataRef->valueint = 0;
            m_pExternJsonDataRef->valuedouble = 0.0;
            m_pExternJsonDataRef->sign = 0;
        }
        else
        {
            cJSON* pDup = cJSON_Duplicate(pSrc, 1);
            if (pDup != NULL)
            {
                /* steal the duplicated payload into the wrapped node */
                if (m_pExternJsonDataRef->valuestring != NULL)
                {
                    cJSON_free(m_pExternJsonDataRef->valuestring);
                }
                if (m_pExternJsonDataRef->child != NULL)
                {
                    cJSON_Delete(m_pExternJsonDataRef->child);
                }
                m_pExternJsonDataRef->type = pDup->type;
                m_pExternJsonDataRef->valuestring = pDup->valuestring;
                m_pExternJsonDataRef->child = pDup->child;
                m_pExternJsonDataRef->valueint = pDup->valueint;
                m_pExternJsonDataRef->valuedouble = pDup->valuedouble;
                m_pExternJsonDataRef->sign = pDup->sign;
                pDup->valuestring = NULL;
                pDup->child = NULL;
                cJSON_Delete(pDup);
            }
        }
        return(*this);
    }
    Parse(oJsonObject.ToString().c_str());
    return(*this);
}

#if __cplusplus >= 201101L
cppJSON& cppJSON::operator=(cppJSON&& oJsonObject)
{
    m_pJsonData = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    m_pExternJsonDataRef = oJsonObject.m_pExternJsonDataRef;
    oJsonObject.m_pExternJsonDataRef = NULL;
    m_pKeyTravers = oJsonObject.m_pKeyTravers;
    oJsonObject.m_pKeyTravers = NULL;
    m_pArrayTravers = oJsonObject.m_pArrayTravers;
    oJsonObject.m_pArrayTravers = NULL;
    mc_pError = oJsonObject.mc_pError;
    oJsonObject.mc_pError = NULL;
    m_strErrMsg = std::move(oJsonObject.m_strErrMsg);
    m_mapJsonArrayRef = std::move(oJsonObject.m_mapJsonArrayRef);
    m_mapJsonObjectRef = std::move(oJsonObject.m_mapJsonObjectRef);
    m_array_iter = m_mapJsonArrayRef.end();
    m_object_iter = m_mapJsonObjectRef.end();
    return(*this);
}
#endif

bool cppJSON::operator==(const cppJSON& oJsonObject) const
{
    return(this->ToString() == oJsonObject.ToString());
}

bool cppJSON::AddEmptySubObject(const std::string& strKey)
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
    cJSON* pJsonStruct = cJSON_CreateObject();
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("create sub empty object error!");
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::AddEmptySubArray(const std::string& strKey)
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
    cJSON* pJsonStruct = cJSON_CreateArray();
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("create sub empty array error!");
        return(false);
    }
    cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct);
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::GetKey(std::string& strKey)
{
    if (IsArray())
    {
        return(false);
    }
    if (m_pKeyTravers == NULL)
    {
        if (m_pJsonData != NULL)
        {
            m_pKeyTravers = m_pJsonData;
        }
        else if (m_pExternJsonDataRef != NULL)
        {
            m_pKeyTravers = m_pExternJsonDataRef;
        }
        return(false);
    }
    else if (m_pKeyTravers == m_pJsonData || m_pKeyTravers == m_pExternJsonDataRef)
    {
        cJSON *c = m_pKeyTravers->child;
        if (c)
        {
            strKey = c->string;
            m_pKeyTravers = c->next;
            return(true);
        }
        else
        {
            return(false);
        }
    }
    else
    {
        strKey = m_pKeyTravers->string;
        m_pKeyTravers = m_pKeyTravers->next;
        return(true);
    }
}

void cppJSON::ResetTraversing()
{
    if (m_pJsonData != NULL)
    {
        m_pKeyTravers = m_pJsonData;
    }
    else
    {
        m_pKeyTravers = m_pExternJsonDataRef;
    }
}

cppJSON& cppJSON::operator[](const std::string& strKey)
{
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter == m_mapJsonObjectRef.end())
    {
        cJSON* pJsonStruct = NULL;
        cJSON* pParent = NULL;
        if (m_pJsonData != NULL)
        {
            if (m_pJsonData->type & cJSON_Object)
            {
                pParent = m_pJsonData;
                pJsonStruct = cJSON_GetObjectItem(pParent, strKey.c_str());
            }
        }
        else if (m_pExternJsonDataRef != NULL)
        {
            if (m_pExternJsonDataRef->type & cJSON_Object)
            {
                pParent = m_pExternJsonDataRef;
                pJsonStruct = cJSON_GetObjectItem(pParent, strKey.c_str());
            }
        }
        if (pJsonStruct == NULL)
        {
            /* map semantics: auto-create an empty sub-object in the parent, so that
             * a["key"] = value  or  a["key"].Add(...)  reflects back to the parent. */
            if (pParent == NULL)
            {
                /* no parent data yet: build the root object first */
                m_pJsonData = cJSON_CreateObject();
                m_pKeyTravers = m_pJsonData;
                pParent = m_pJsonData;
            }
            cJSON* pNewStruct = cJSON_CreateObject();
            if (pNewStruct == NULL || !cJSON_AddItemToObject(pParent, strKey.c_str(), pNewStruct))
            {
                if (pNewStruct != NULL)
                {
                    cJSON_Delete(pNewStruct);
                }
                cppJSON* pJsonObject = new cppJSON();
                m_mapJsonObjectRef.insert(std::pair<std::string, cppJSON*>(strKey, pJsonObject));
                return(*pJsonObject);
            }
            cppJSON* pJsonObject = new cppJSON(pNewStruct);
            m_mapJsonObjectRef.insert(std::pair<std::string, cppJSON*>(strKey, pJsonObject));
            return(*pJsonObject);
        }
        else
        {
            cppJSON* pJsonObject = new cppJSON(pJsonStruct);
            m_mapJsonObjectRef.insert(std::pair<std::string, cppJSON*>(strKey, pJsonObject));
            return(*pJsonObject);
        }
    }
    else
    {
        return(*(iter->second));
    }
}

const cppJSON& cppJSON::operator[](const std::string& strKey) const
{
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        return(*(iter->second));
    }
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
    /* read-only: never creates the JSON key, just caches a wrapper */
    cppJSON* pJsonObject = new cppJSON(pJsonStruct);
    m_mapJsonObjectRef.insert(std::pair<std::string, cppJSON*>(strKey, pJsonObject));
    return(*pJsonObject);
}

cppJSON& cppJSON::operator[](unsigned int uiWhich)
{
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(uiWhich);
#else
    auto iter = m_mapJsonArrayRef.find(uiWhich);
#endif
    if (iter == m_mapJsonArrayRef.end())
    {
        cJSON* pJsonStruct = NULL;
        if (m_pJsonData != NULL)
        {
            if (m_pJsonData->type & cJSON_Array)
            {
                pJsonStruct = cJSON_GetArrayItem(m_pJsonData, uiWhich);
            }
        }
        else if (m_pExternJsonDataRef != NULL)
        {
            if (m_pExternJsonDataRef->type & cJSON_Array)
            {
                pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, uiWhich);
            }
        }
        if (pJsonStruct == NULL)
        {
            cppJSON* pJsonObject = new cppJSON();
            m_mapJsonArrayRef.insert(std::pair<unsigned int, cppJSON*>(uiWhich, pJsonObject));
            return(*pJsonObject);
        }
        else
        {
            cppJSON* pJsonObject = new cppJSON(pJsonStruct);
            m_mapJsonArrayRef.insert(std::pair<unsigned int, cppJSON*>(uiWhich, pJsonObject));
            return(*pJsonObject);
        }
    }
    else
    {
        return(*(iter->second));
    }
}

const cppJSON& cppJSON::operator[](unsigned int uiWhich) const
{
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(uiWhich);
#else
    auto iter = m_mapJsonArrayRef.find(uiWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        return(*(iter->second));
    }
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, uiWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if (m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, uiWhich);
        }
    }
    /* read-only: never mutates the JSON, just caches a wrapper */
    cppJSON* pJsonObject = new cppJSON(pJsonStruct);
    m_mapJsonArrayRef.insert(std::pair<unsigned int, cppJSON*>(uiWhich, pJsonObject));
    return(*pJsonObject);
}

std::string cppJSON::operator()(const std::string& strKey) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(std::string(""));
    }
    if (pJsonStruct->type & cJSON_String)
    {
        return(pJsonStruct->valuestring);
    }
    else if (pJsonStruct->type & cJSON_Number)
    {
        char szNumber[128] = {0};
        if (pJsonStruct->sign != 0)
        {
            /* exact 64-bit integer */
            if (pJsonStruct->sign == -1)
            {
                snprintf(szNumber, sizeof(szNumber), "%lld", (long long)pJsonStruct->valueint);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%llu", (unsigned long long)((uint64_t)pJsonStruct->valueint));
            }
        }
        else
        {
            double d = pJsonStruct->valuedouble;
            if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9)
            {
                snprintf(szNumber, sizeof(szNumber), "%e", d);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%f", d);
            }
        }
        return(std::string(szNumber));
    }
    else if (pJsonStruct->type & cJSON_False)
    {
        return(std::string("false"));
    }
    else if (pJsonStruct->type & cJSON_True)
    {
        return(std::string("true"));
    }
    return(std::string(""));
}

std::string cppJSON::operator()(unsigned int uiWhich) const
{
    cJSON* pJsonStruct = NULL;
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pJsonData, uiWhich);
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, uiWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(std::string(""));
    }
    if (pJsonStruct->type & cJSON_String)
    {
        return(pJsonStruct->valuestring);
    }
    else if (pJsonStruct->type & cJSON_Number)
    {
        char szNumber[128] = {0};
        if (pJsonStruct->sign != 0)
        {
            /* exact 64-bit integer */
            if (pJsonStruct->sign == -1)
            {
                snprintf(szNumber, sizeof(szNumber), "%lld", (long long)pJsonStruct->valueint);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%llu", (unsigned long long)((uint64_t)pJsonStruct->valueint));
            }
        }
        else
        {
            double d = pJsonStruct->valuedouble;
            if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9)
            {
                snprintf(szNumber, sizeof(szNumber), "%e", d);
            }
            else
            {
                snprintf(szNumber, sizeof(szNumber), "%f", d);
            }
        }
        return(std::string(szNumber));
    }
    else if (pJsonStruct->type & cJSON_False)
    {
        return(std::string("false"));
    }
    else if (pJsonStruct->type & cJSON_True)
    {
        return(std::string("true"));
    }
    return(std::string(""));
}

bool cppJSON::Parse(const std::string& strJson)
{
    Clear();
    m_pJsonData = cJSON_ParseWithOpts(strJson.c_str(), &mc_pError, 0);
    m_pKeyTravers = m_pJsonData;
    if (m_pJsonData == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + mc_pError;
        return(false);
    }
    return(true);
}

void cppJSON::Clear()
{
    m_pExternJsonDataRef = NULL;
    m_pKeyTravers = NULL;
    m_pArrayTravers = NULL;
    if (m_pJsonData != NULL)
    {
        cJSON_Delete(m_pJsonData);
        m_pJsonData = NULL;
    }
#if __cplusplus < 201101L
    for (std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.begin();
                    iter != m_mapJsonArrayRef.end(); ++iter)
#else
    for (auto iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); ++iter)
#endif
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
    }
    m_mapJsonArrayRef.clear();
    m_array_iter = m_mapJsonArrayRef.end();
#if __cplusplus < 201101L
    for (std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.begin();
                    iter != m_mapJsonObjectRef.end(); ++iter)
#else
    for (auto iter = m_mapJsonObjectRef.begin(); iter != m_mapJsonObjectRef.end(); ++iter)
#endif
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
    }
    m_mapJsonObjectRef.clear();
    m_object_iter = m_mapJsonObjectRef.end();
}

bool cppJSON::IsEmpty() const
{
    if (m_pJsonData != NULL)
    {
        return(false);
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        return(false);
    }
    return(true);
}

bool cppJSON::IsArray() const
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

    if (pFocusData == NULL)
    {
        return(false);
    }

    if (pFocusData->type & cJSON_Array)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

std::string cppJSON::ToString() const
{
    char* pJsonString = NULL;
    std::string strJsonData = "";
    if (m_pJsonData != NULL)
    {
        pJsonString = cJSON_PrintUnformatted(m_pJsonData);
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pJsonString = cJSON_PrintUnformatted(m_pExternJsonDataRef);
    }
    if (pJsonString != NULL)
    {
        strJsonData = pJsonString;
        free(pJsonString);
    }
    return(strJsonData);
}

std::string cppJSON::ToFormattedString() const
{
    char* pJsonString = NULL;
    std::string strJsonData = "";
    if (m_pJsonData != NULL)
    {
        pJsonString = cJSON_Print(m_pJsonData);
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        pJsonString = cJSON_Print(m_pExternJsonDataRef);
    }
    if (pJsonString != NULL)
    {
        strJsonData = pJsonString;
        free(pJsonString);
    }
    return(strJsonData);
}

bool cppJSON::KeyExist(const std::string& strKey) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    return(true);
}

int cppJSON::ValueType(const std::string& strKey) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(cJSON_False);
    }
    return(pJsonStruct->type);
}

bool cppJSON::Get(const std::string& strKey, cppJSON& oJsonObject) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    /* deep-copy the node directly instead of serialize+parse */
    oJsonObject.FromNode(pJsonStruct);
    return(true);
}

bool cppJSON::Get(const std::string& strKey, std::string& strValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_String))
    {
        return(false);
    }
    strValue = pJsonStruct->valuestring;
    return(true);
}

bool cppJSON::Get(const std::string& strKey, int32& iValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        iValue = (pJsonStruct->sign != 0) ? (int32)(pJsonStruct->valueint) : (int32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(const std::string& strKey, uint32& uiValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        uiValue = (pJsonStruct->sign != 0) ? (uint32)(pJsonStruct->valueint) : (uint32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(const std::string& strKey, int64& llValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        llValue = (pJsonStruct->sign != 0) ? (int64)(pJsonStruct->valueint) : (int64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(const std::string& strKey, uint64& ullValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        ullValue = (pJsonStruct->sign != 0) ? (uint64)(pJsonStruct->valueint) : (uint64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(const std::string& strKey, bool& bValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & (cJSON_False | cJSON_True)))
    {
        return(false);
    }
    bValue = (pJsonStruct->type & cJSON_True) ? true : false;
    return(true);
}

bool cppJSON::Get(const std::string& strKey, float& fValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        fValue = (pJsonStruct->sign != 0) ? (float)(pJsonStruct->valueint) : (float)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(const std::string& strKey, double& dValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        dValue = (pJsonStruct->sign != 0) ? (double)(pJsonStruct->valueint) : (double)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::IsNull(const std::string& strKey) const
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
        if(m_pExternJsonDataRef->type & cJSON_Object)
        {
            pJsonStruct = cJSON_GetObjectItem(m_pExternJsonDataRef, strKey.c_str());
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_NULL))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(const std::string& strKey, const cppJSON& oJsonObject)
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
    cJSON* pJsonStruct = cJSON_ParseWithOpts(oJsonObject.ToString().c_str(), &mc_pError, 0) ;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + mc_pError;
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

#if __cplusplus < 201101L
bool cppJSON::AddWithMove(const std::string& strKey, cppJSON& oJsonObject)
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}
#else
bool cppJSON::Add(const std::string& strKey, cppJSON&& oJsonObject)
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    auto iter = m_mapJsonObjectRef.find(strKey);
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}
#endif

bool cppJSON::Add(const std::string& strKey, const std::string& strValue)
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
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Add(const std::string& strKey, int32 iValue)
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Add(const std::string& strKey, uint32 uiValue)
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Add(const std::string& strKey, int64 llValue)
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Add(const std::string& strKey, uint64 ullValue)
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Add(const std::string& strKey, bool bValue, bool bValueAgain)
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
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Add(const std::string& strKey, float fValue)
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
    cJSON* pJsonStruct = cJSON_CreateNumber(fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Add(const std::string& strKey, double dValue)
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
    cJSON* pJsonStruct = cJSON_CreateNumber(dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

#if __cplusplus < 201101L
bool cppJSON::ReplaceAdd(const std::string& strKey, const cppJSON& oJsonObject)
{
    if (KeyExist(strKey))
    {
        return(Replace(strKey, oJsonObject));
    }
    return(Add(strKey, oJsonObject));
}

bool cppJSON::ReplaceAdd(const std::string& strKey, const std::string& strValue)
{
    if (KeyExist(strKey))
    {
        return(Replace(strKey, strValue));
    }
    return(Add(strKey, strValue));
}
#endif

bool cppJSON::AddNull(const std::string& strKey)
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
    cJSON* pJsonStruct = cJSON_CreateNull();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Delete(const std::string& strKey)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON_DeleteItemFromObject(pFocusData, strKey.c_str());
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    m_pKeyTravers = pFocusData;
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, const cppJSON& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_ParseWithOpts(oJsonObject.ToString().c_str(), &mc_pError, 0);
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + mc_pError;
        return(false);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    return(true);
}

#if __cplusplus < 201101L
bool cppJSON::ReplaceWithMove(const std::string& strKey, cppJSON& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    return(true);
}
#else
bool cppJSON::Replace(const std::string& strKey, cppJSON&& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    auto iter = m_mapJsonObjectRef.find(strKey);
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    return(true);
}
#endif

bool cppJSON::Replace(const std::string& strKey, const std::string& strValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, int32 iValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, uint32 uiValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, int64 llValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, uint64 ullValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, bool bValue, bool bValueAgain)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, float fValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateNumber(fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(const std::string& strKey, double dValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateNumber(dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::ReplaceWithNull(const std::string& strKey)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateNull();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<std::string, cppJSON*>::iterator iter = m_mapJsonObjectRef.find(strKey);
#else
    auto iter = m_mapJsonObjectRef.find(strKey);
#endif
    if (iter != m_mapJsonObjectRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonObjectRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInObject(pFocusData, strKey.c_str(), pJsonStruct))
    {
        return(false);
    }
    return(true);
}

int cppJSON::GetArraySize() const
{
    if (m_pJsonData != NULL)
    {
        if (m_pJsonData->type & cJSON_Array)
        {
            return(cJSON_GetArraySize(m_pJsonData));
        }
    }
    else if (m_pExternJsonDataRef != NULL)
    {
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            return(cJSON_GetArraySize(m_pExternJsonDataRef));
        }
    }
    return(0);
}

int cppJSON::ValueType(int iWhich) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(cJSON_False);
    }
    return(pJsonStruct->type);
}

bool cppJSON::Get(int iWhich, cppJSON& oJsonObject) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    /* deep-copy the node directly instead of serialize+parse */
    oJsonObject.FromNode(pJsonStruct);
    return(true);
}

bool cppJSON::Get(int iWhich, std::string& strValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_String))
    {
        return(false);
    }
    strValue = pJsonStruct->valuestring;
    return(true);
}

bool cppJSON::Get(int iWhich, int32& iValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        iValue = (pJsonStruct->sign != 0) ? (int32)(pJsonStruct->valueint) : (int32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(int iWhich, uint32& uiValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        uiValue = (pJsonStruct->sign != 0) ? (uint32)(pJsonStruct->valueint) : (uint32)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(int iWhich, int64& llValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        llValue = (pJsonStruct->sign != 0) ? (int64)(pJsonStruct->valueint) : (int64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(int iWhich, uint64& ullValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        ullValue = (pJsonStruct->sign != 0) ? (uint64)(pJsonStruct->valueint) : (uint64)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(int iWhich, bool& bValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & (cJSON_False | cJSON_True)))
    {
        return(false);
    }
    bValue = (pJsonStruct->type & cJSON_True) ? true : false;
    return(true);
}

bool cppJSON::Get(int iWhich, float& fValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        fValue = (pJsonStruct->sign != 0) ? (float)(pJsonStruct->valueint) : (float)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::Get(int iWhich, double& dValue) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (pJsonStruct->type & cJSON_Number)
    {
        dValue = (pJsonStruct->sign != 0) ? (double)(pJsonStruct->valueint) : (double)(pJsonStruct->valuedouble);
        return(true);
    }
    return(false);
}

bool cppJSON::IsNull(int iWhich) const
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
        if(m_pExternJsonDataRef->type & cJSON_Array)
        {
            pJsonStruct = cJSON_GetArrayItem(m_pExternJsonDataRef, iWhich);
        }
    }
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_NULL))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(const cppJSON& oJsonObject)
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
    cJSON* pJsonStruct = cJSON_ParseWithOpts(oJsonObject.ToString().c_str(), &mc_pError, 0);
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + mc_pError;
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    unsigned int uiLastIndex = (unsigned int)cJSON_GetArraySize(pFocusData) - 1;
#if __cplusplus < 201101L
    for (std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.begin();
                    iter != m_mapJsonArrayRef.end(); )
#else
    for (auto iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); )
#endif
    {
        if (iter->first >= uiLastIndex)
        {
            if (iter->second != NULL)
            {
                delete (iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}

#if __cplusplus < 201101L
bool cppJSON::AddWithMove(cppJSON& oJsonObject)
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    unsigned int uiLastIndex = (unsigned int)cJSON_GetArraySize(pFocusData) - 1;
    for (std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); )
    {
        if (iter->first >= uiLastIndex)
        {
            if (iter->second != NULL)
            {
                delete (iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}
#else
bool cppJSON::Add(cppJSON&& oJsonObject)
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    unsigned int uiLastIndex = (unsigned int)cJSON_GetArraySize(pFocusData) - 1;
    for (auto iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); )
    {
        if (iter->first >= uiLastIndex)
        {
            if (iter->second != NULL)
            {
                delete (iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}
#endif

bool cppJSON::Add(const std::string& strValue)
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
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(int32 iValue)
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(uint32 uiValue)
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(int64 llValue)
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(uint64 ullValue)
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(int iAnywhere, bool bValue)
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
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(float fValue)
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
    cJSON* pJsonStruct = cJSON_CreateNumber(fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Add(double dValue)
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
    cJSON* pJsonStruct = cJSON_CreateNumber(dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddNull()
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
    cJSON* pJsonStruct = cJSON_CreateNull();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_AddItemToArray(pFocusData, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(const cppJSON& oJsonObject)
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
    cJSON* pJsonStruct = cJSON_ParseWithOpts(oJsonObject.ToString().c_str(), &mc_pError, 0);
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + mc_pError;
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
#if __cplusplus < 201101L
    for (std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.begin();
                    iter != m_mapJsonArrayRef.end(); )
#else
    for (auto iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); )
#endif
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter++);
    }
    return(true);
}

#if __cplusplus < 201101L
bool cppJSON::AddAsFirstWithMove(cppJSON& oJsonObject)
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    for (std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); )
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter++);
    }
    return(true);
}
#else
bool cppJSON::AddAsFirst(cppJSON&& oJsonObject)
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    for (auto iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); )
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter++);
    }
    return(true);
}
#endif

bool cppJSON::AddAsFirst(const std::string& strValue)
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
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(int32 iValue)
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(uint32 uiValue)
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(int64 llValue)
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(uint64 ullValue)
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(int iAnywhere, bool bValue)
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
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(float fValue)
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
    cJSON* pJsonStruct = cJSON_CreateNumber(fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddAsFirst(double dValue)
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
    cJSON* pJsonStruct = cJSON_CreateNumber(dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::AddNullAsFirst()
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
    cJSON* pJsonStruct = cJSON_CreateNull();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!cJSON_InsertItemInArray(pFocusData, 0, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Delete(int iWhich)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON_DeleteItemFromArray(pFocusData, iWhich);
#if __cplusplus < 201101L
    for (std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.begin();
                    iter != m_mapJsonArrayRef.end(); )
#else
    for (auto iter = m_mapJsonArrayRef.begin(); iter != m_mapJsonArrayRef.end(); )
#endif
    {
        if (iter->first >= (unsigned int)iWhich)
        {
            if (iter->second != NULL)
            {
                delete (iter->second);
                iter->second = NULL;
            }
            m_mapJsonArrayRef.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, const cppJSON& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_ParseWithOpts(oJsonObject.ToString().c_str(), &mc_pError, 0);
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = std::string("prase json string error at ") + mc_pError;
        return(false);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    return(true);
}

#if __cplusplus < 201101L
bool cppJSON::ReplaceWithMove(int iWhich, cppJSON& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    return(true);
}
#else
bool cppJSON::Replace(int iWhich, cppJSON&& oJsonObject)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = oJsonObject.m_pJsonData;
    oJsonObject.m_pJsonData = NULL;
    if (pJsonStruct == NULL)
    {
        m_strErrMsg = "can not move a non-independent(internal) cppJSON from one to another.";
        return(false);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    auto iter = m_mapJsonArrayRef.find(iWhich);
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    return(true);
}
#endif

bool cppJSON::Replace(int iWhich, const std::string& strValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateString(strValue.c_str());
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, int32 iValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)iValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, uint32 uiValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)uiValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, int64 llValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateInt64((int64_t)llValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, uint64 ullValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateUint64((uint64_t)ullValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, bool bValue, bool bValueAgain)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateBool(bValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, float fValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateNumber(fValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::Replace(int iWhich, double dValue)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateNumber(dValue);
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

bool cppJSON::ReplaceWithNull(int iWhich)
{
    cJSON* pFocusData = NULL;
    if (m_pJsonData == NULL)
    {
        pFocusData = m_pExternJsonDataRef;
    }
    else
    {
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
    cJSON* pJsonStruct = cJSON_CreateNull();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
#if __cplusplus < 201101L
    std::map<unsigned int, cppJSON*>::iterator iter = m_mapJsonArrayRef.find(iWhich);
#else
    auto iter = m_mapJsonArrayRef.find(iWhich);
#endif
    if (iter != m_mapJsonArrayRef.end())
    {
        if (iter->second != NULL)
        {
            delete (iter->second);
            iter->second = NULL;
        }
        m_mapJsonArrayRef.erase(iter);
    }
    if (!cJSON_ReplaceItemInArray(pFocusData, iWhich, pJsonStruct))
    {
        return(false);
    }
    return(true);
}

cJSON* cppJSON::GetNextArrayItem()
{
    cJSON* pJsonStruct = m_pArrayTravers;
    if (pJsonStruct == NULL)
    {
        return(NULL);
    }
    m_pArrayTravers = pJsonStruct->next;
    return(pJsonStruct);
}

void cppJSON::ResetArrayTraversing()
{
    if (m_pJsonData != NULL && (m_pJsonData->type & cJSON_Array))
    {
        m_pArrayTravers = m_pJsonData->child;
    }
    else if (m_pExternJsonDataRef != NULL && (m_pExternJsonDataRef->type & cJSON_Array))
    {
        m_pArrayTravers = m_pExternJsonDataRef->child;
    }
    else
    {
        m_pArrayTravers = NULL;
    }
}

bool cppJSON::GetNextValue(std::string& strValue)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_String))
    {
        return(false);
    }
    strValue = pJsonStruct->valuestring;
    return(true);
}

bool cppJSON::GetNextValue(int32& iValue)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_Number))
    {
        return(false);
    }
    iValue = (pJsonStruct->sign != 0) ? (int32)(pJsonStruct->valueint) : (int32)(pJsonStruct->valuedouble);
    return(true);
}

bool cppJSON::GetNextValue(uint32& uiValue)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_Number))
    {
        return(false);
    }
    uiValue = (pJsonStruct->sign != 0) ? (uint32)(pJsonStruct->valueint) : (uint32)(pJsonStruct->valuedouble);
    return(true);
}

bool cppJSON::GetNextValue(int64& llValue)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_Number))
    {
        return(false);
    }
    llValue = (pJsonStruct->sign != 0) ? (int64)(pJsonStruct->valueint) : (int64)(pJsonStruct->valuedouble);
    return(true);
}

bool cppJSON::GetNextValue(uint64& ullValue)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_Number))
    {
        return(false);
    }
    ullValue = (pJsonStruct->sign != 0) ? (uint64)(pJsonStruct->valueint) : (uint64)(pJsonStruct->valuedouble);
    return(true);
}

bool cppJSON::GetNextValue(float& fValue)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_Number))
    {
        return(false);
    }
    fValue = (pJsonStruct->sign != 0) ? (float)(pJsonStruct->valueint) : (float)(pJsonStruct->valuedouble);
    return(true);
}

bool cppJSON::GetNextValue(double& dValue)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    if (!(pJsonStruct->type & cJSON_Number))
    {
        return(false);
    }
    dValue = (pJsonStruct->sign != 0) ? (double)(pJsonStruct->valueint) : (double)(pJsonStruct->valuedouble);
    return(true);
}

bool cppJSON::GetNextValue(cppJSON& oJsonObject)
{
    cJSON* pJsonStruct = GetNextArrayItem();
    if (pJsonStruct == NULL)
    {
        return(false);
    }
    /* deep-copy the node directly instead of serialize+parse */
    oJsonObject.FromNode(pJsonStruct);
    return(true);
}

cppJSON::cppJSON(cJSON* pJsonData)
    : m_pJsonData(NULL), m_pExternJsonDataRef(pJsonData), m_pKeyTravers(pJsonData), m_pArrayTravers(NULL)
{
}

void cppJSON::SetNodeAsNumber(int64_t valueint, double valuedouble, int sign)
{
    cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
    if (pNode == NULL)
    {
        m_pJsonData = cJSON_CreateInt64(valueint);
        m_pJsonData->valuedouble = valuedouble;
        m_pJsonData->sign = sign;
        return;
    }
    /* free the old value payload, keep key (string) and list position (next/prev) */
    if (pNode->valuestring != NULL)
    {
        cJSON_free(pNode->valuestring);
        pNode->valuestring = NULL;
    }
    if (pNode->child != NULL)
    {
        cJSON_Delete(pNode->child);
        pNode->child = NULL;
    }
    pNode->type = cJSON_Number;
    pNode->valueint = valueint;
    pNode->valuedouble = valuedouble;
    pNode->sign = sign;
}

void cppJSON::SetNodeAsString(const char* strValue)
{
    cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
    if (pNode == NULL)
    {
        m_pJsonData = cJSON_CreateString(strValue);
        return;
    }
    if (pNode->child != NULL)
    {
        cJSON_Delete(pNode->child);
        pNode->child = NULL;
    }
    char* pNewString = (char*)malloc(strlen(strValue) + 1);
    strcpy(pNewString, strValue);
    if (pNode->valuestring != NULL)
    {
        cJSON_free(pNode->valuestring);
    }
    pNode->valuestring = pNewString;
    pNode->type = cJSON_String;
}

void cppJSON::SetNodeAsBool(bool bValue)
{
    cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
    if (pNode == NULL)
    {
        m_pJsonData = cJSON_CreateBool(bValue);
        return;
    }
    if (pNode->valuestring != NULL)
    {
        cJSON_free(pNode->valuestring);
        pNode->valuestring = NULL;
    }
    if (pNode->child != NULL)
    {
        cJSON_Delete(pNode->child);
        pNode->child = NULL;
    }
    pNode->type = bValue ? cJSON_True : cJSON_False;
}

void cppJSON::SetNodeFromCJson(cJSON* pNew)
{
    cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
    if (pNode == NULL)
    {
        /* standalone object: take ownership of pNew directly */
        m_pJsonData = pNew;
        m_pKeyTravers = pNew;
        return;
    }
    /* free the old value payload, keep key (string) and list position (next/prev) */
    if (pNode->valuestring != NULL)
    {
        cJSON_free(pNode->valuestring);
        pNode->valuestring = NULL;
    }
    if (pNode->child != NULL)
    {
        cJSON_Delete(pNode->child);
        pNode->child = NULL;
    }
    /* steal pNew's payload, then free the now-empty shell */
    pNode->type = pNew->type;
    pNode->valuestring = pNew->valuestring;
    pNode->child = pNew->child;
    pNode->valueint = pNew->valueint;
    pNode->valuedouble = pNew->valuedouble;
    pNode->sign = pNew->sign;
    pNew->valuestring = NULL;
    pNew->child = NULL;
    cJSON_Delete(pNew);
}

cJSON* cppJSON::DuplicateNode() const
{
    cJSON* pNode = (m_pExternJsonDataRef != NULL) ? m_pExternJsonDataRef : m_pJsonData;
    if (pNode == NULL)
    {
        return(NULL);
    }
    return(cJSON_Duplicate(pNode, 1));
}

void cppJSON::FromNode(cJSON* pNode)
{
    Clear();
    if (pNode != NULL)
    {
        m_pJsonData = cJSON_Duplicate(pNode, 1);
        m_pKeyTravers = m_pJsonData;
    }
}

cppJSON& cppJSON::operator=(int32 iValue)
{
    SetNodeAsNumber((int64_t)iValue, (double)iValue, (iValue < 0) ? -1 : 1);
    return(*this);
}

cppJSON& cppJSON::operator=(uint32 uiValue)
{
    SetNodeAsNumber((int64_t)uiValue, (double)uiValue, 1);
    return(*this);
}

cppJSON& cppJSON::operator=(int64 llValue)
{
    SetNodeAsNumber((int64_t)llValue, (double)llValue, -1);
    return(*this);
}

cppJSON& cppJSON::operator=(uint64 ullValue)
{
    SetNodeAsNumber((int64_t)ullValue, (double)ullValue, 1);
    return(*this);
}

cppJSON& cppJSON::operator=(float fValue)
{
    return(operator=((double)fValue));
}

cppJSON& cppJSON::operator=(double dValue)
{
    if ((fabs(dValue) < (double)INT64_MAX) && (dValue == (double)(int64_t)dValue) && !(dValue == 0.0 && std::signbit(dValue)))
    {
        SetNodeAsNumber((int64_t)dValue, dValue, (dValue < 0) ? -1 : 1);
    }
    else
    {
        SetNodeAsNumber((int64_t)dValue, dValue, 0);
    }
    return(*this);
}

cppJSON& cppJSON::operator=(bool bValue)
{
    SetNodeAsBool(bValue);
    return(*this);
}

cppJSON& cppJSON::operator=(const std::string& strValue)
{
    SetNodeAsString(strValue.c_str());
    return(*this);
}

cppJSON& cppJSON::operator=(const char* strValue)
{
    SetNodeAsString((strValue != NULL) ? strValue : "");
    return(*this);
}

