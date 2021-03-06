//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
// Filename:        $HeadURL$
//
// Version:         $Revision$,
//                  $Date$
//                  $Author$
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// More information of Gurux products: http://www.gurux.org
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------

#include "../include/GXDLMSProfileGeneric.h"
#include "../include/GXDLMSClient.h"
#include "../include/GXDLMSObjectFactory.h"

CGXDLMSProfileGeneric::~CGXDLMSProfileGeneric()
{
    for(std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it = m_CaptureObjects.begin();
            it != m_CaptureObjects.end(); ++it)
    {
        delete it->second;
    }
    m_CaptureObjects.clear();
}

void CGXDLMSProfileGeneric::Init()
{
    m_SortObjectAttributeIndex = 0;
    m_SortObjectDataIndex = 0;
    m_SortObject = NULL;
    m_CapturePeriod = 3600;
    m_EntriesInUse = m_ProfileEntries = 0;
    m_SortMethod = GX_SORT_METHOD_FIFO;
}

int CGXDLMSProfileGeneric::GetColumns(CGXByteBuffer& data)
{
    int cnt = m_CaptureObjects.size();
    data.SetUInt8(DLMS_DATA_TYPE_ARRAY);
    //Add count
    GXHelpers::SetObjectCount(cnt, data);
    std::string ln;
    int ret;
    CGXDLMSVariant tmp, ai, di;
    for (std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it = m_CaptureObjects.begin(); it != m_CaptureObjects.end(); ++it)
    {
        data.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
        data.SetUInt8(4); //Count
        tmp = it->first->GetObjectType();
        if ((ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_UINT16, tmp)) != 0) //ClassID
        {
            return ret;
        }
        (*it).first->GetLogicalName(ln);
        tmp = ln;
        ai = (*it).second->GetAttributeIndex();
        di = (*it).second->GetDataIndex();
        if ((ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_OCTET_STRING, tmp)) != 0 || //LN
                (ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_INT8, ai)) != 0 || //Attribute Index
                (ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_UINT16, di)) != 0) //Data Index
        {
            return ret;
        }
    }
    return ERROR_CODES_OK;
}

int CGXDLMSProfileGeneric::GetData(std::vector< std::vector<CGXDLMSVariant> > table, CGXByteBuffer& data)
{
    data.SetUInt8(DLMS_DATA_TYPE_ARRAY);
    GXHelpers::SetObjectCount(table.size(), data);
    std::vector<DLMS_DATA_TYPE> types;
    DLMS_DATA_TYPE type;
    int ret;
    for (std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it = m_CaptureObjects.begin();
            it != m_CaptureObjects.end(); ++it)
    {
        if ((ret = (*it).first->GetDataType((*it).second->GetAttributeIndex(), type)) != 0)
        {
            return ret;
        }
        types.push_back(type);
    }
    for (std::vector< std::vector<CGXDLMSVariant> >::iterator row = table.begin(); row != table.end(); ++row)
    {
        data.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
        GXHelpers::SetObjectCount((*row).size(), data);
        int pos = -1;
        for (std::vector<CGXDLMSVariant>::iterator value = (*row).begin(); value != (*row).end(); ++value)
        {
            DLMS_DATA_TYPE tp = types[++pos];
            if (tp == DLMS_DATA_TYPE_NONE)
            {
                //TODO:
                //tp = GXCommon.GetValueType(value);
                //types[pos] = tp;
            }
            if ((ret = GXHelpers::SetData(data, tp, *value)) != 0)
            {
                return ret;
            }
        }
    }
    return ERROR_CODES_OK;
}

/*
* Add new capture object (column) to the profile generic.
*/
int CGXDLMSProfileGeneric::AddCaptureObject(CGXDLMSObject* pObj, int attributeIndex, int dataIndex)
{
    if (pObj == NULL)
    {
        //Invalid Object
        return ERROR_CODES_INVALID_PARAMETER;
    }
    if (attributeIndex < 1)
    {
        //Invalid attribute index
        return ERROR_CODES_INVALID_PARAMETER;
    }
    if (dataIndex < 0)
    {
        //Invalid data index
        return ERROR_CODES_INVALID_PARAMETER;
    }
    CGXDLMSCaptureObject* pCO = new CGXDLMSCaptureObject(attributeIndex, dataIndex);
    m_CaptureObjects.push_back(std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*>(pObj, pCO));
    return ERROR_CODES_OK;
}

int CGXDLMSProfileGeneric::GetProfileGenericData(int selector, CGXDLMSVariant& parameters, CGXByteBuffer& reply)
{
    CGXDLMSVariant from, to;
    //If all data is read.
    if (selector == 0 || parameters.vt == DLMS_DATA_TYPE_NONE)
    {
        return GetData(GetBuffer(), reply);
    }
    std::vector< std::vector<CGXDLMSVariant> >& table = GetBuffer();
    std::vector< std::vector<CGXDLMSVariant> > items;
    //TODO: Lock synchronized (this)
    {
        if (selector == 1) //Read by range
        {
            struct tm tmp = from.dateTime.GetValue();
            time_t start = mktime(&tmp);
            tmp = to.dateTime.GetValue();
            time_t end = mktime(&tmp);
            for (std::vector< std::vector<CGXDLMSVariant> >::iterator row = table.begin(); row != table.end(); ++row)
            {
                tmp = (*row)[0].dateTime.GetValue();
                time_t tm = mktime(&tmp);
                if (tm >= start && tm <= end)
                {
                    items.push_back(*row);
                }
            }
        }
        else if (selector == 2) //Read by entry.
        {
            int start = from.ToInteger();
            int count = to.ToInteger();
            for (int pos = 0; pos < count; ++pos)
            {
                if ((unsigned int) (pos + start) == table.size())
                {
                    break;
                }
                items.push_back(table[start + pos]);
            }
        }
        else
        {
            return ERROR_CODES_INVALID_PARAMETER;
        }
    }
    return GetData(items, reply);
}

/**
 Constructor.
*/
CGXDLMSProfileGeneric::CGXDLMSProfileGeneric() : CGXDLMSObject(OBJECT_TYPE_PROFILE_GENERIC)
{
    Init();
}

//SN Constructor.
CGXDLMSProfileGeneric::CGXDLMSProfileGeneric(unsigned short sn) : CGXDLMSObject(OBJECT_TYPE_PROFILE_GENERIC, sn)
{
    Init();
}

//SN Constructor.
CGXDLMSProfileGeneric::CGXDLMSProfileGeneric(unsigned short sn, CGXDLMSVariant value) : CGXDLMSObject(OBJECT_TYPE_PROFILE_GENERIC, sn)
{
    Init();
}

/**
 Constructor.

 @param ln Logical Name of the object.
*/
CGXDLMSProfileGeneric::CGXDLMSProfileGeneric(std::string ln) : CGXDLMSObject(OBJECT_TYPE_PROFILE_GENERIC, ln)
{
    Init();
}

/**
 Data of profile generic.
*/
std::vector< std::vector<CGXDLMSVariant> >& CGXDLMSProfileGeneric::GetBuffer()
{
    return m_Buffer;
}

/**
 Captured Objects.
*/
std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >& CGXDLMSProfileGeneric::GetCaptureObjects()
{
    return m_CaptureObjects;
}

/**
 How often values are captured.
*/
int CGXDLMSProfileGeneric::GetCapturePeriod()
{
    return m_CapturePeriod;
}
void CGXDLMSProfileGeneric::SetCapturePeriod(int value)
{
    m_CapturePeriod = value;
}

/**
 How columns are sorted.
*/
GX_SORT_METHOD CGXDLMSProfileGeneric::GetSortMethod()
{
    return m_SortMethod;
}
void CGXDLMSProfileGeneric::SetSortMethod(GX_SORT_METHOD value)
{
    m_SortMethod = value;
}

/**
 Column that is used for sorting.
*/
CGXDLMSObject* CGXDLMSProfileGeneric::GetSortObject()
{
    return m_SortObject;
}
void CGXDLMSProfileGeneric::SetSortObject(CGXDLMSObject* value)
{
    m_SortObject = value;
}

/**
 Entries (rows) in Use.
*/
unsigned long CGXDLMSProfileGeneric::GetEntriesInUse()
{
    return m_EntriesInUse;
}

void CGXDLMSProfileGeneric::SetEntriesInUse(unsigned long value)
{
    m_EntriesInUse = value;
}

/**
 Maximum Entries (rows) count.
*/
unsigned long CGXDLMSProfileGeneric::GetProfileEntries()
{
    return m_ProfileEntries;
}

void CGXDLMSProfileGeneric::SetProfileEntries(unsigned long value)
{
    m_ProfileEntries = value;
}

/**
Attribute index of sort object.
*/
int CGXDLMSProfileGeneric::GetSortObjectAttributeIndex()
{
    return m_SortObjectAttributeIndex;
}
void CGXDLMSProfileGeneric::SetSortObjectAttributeIndex(int value)
{
    m_SortObjectAttributeIndex = value;
}

/**
 Data index of sort object.
*/
int CGXDLMSProfileGeneric::GetSortObjectDataIndex()
{
    return m_SortObjectDataIndex;
}
void CGXDLMSProfileGeneric::SetSortObjectDataIndex(int value)
{
    m_SortObjectDataIndex = value;
}

/**
 Clears the buffer.
*/
void CGXDLMSProfileGeneric::Reset()
{
    //TODO:
}

/**
 Copies the values of the objects to capture
 into the buffer by reading capture objects.
*/
void CGXDLMSProfileGeneric::Capture()
{
    //TODO:
}

void CGXDLMSProfileGeneric::GetValues(std::vector<std::string>& values)
{
    values.clear();
    std::string ln;
    GetLogicalName(ln);
    values.push_back(ln);
    std::stringstream sb;
    bool empty = true;
    for(std::vector< std::vector<CGXDLMSVariant> >::iterator row = m_Buffer.begin(); row != m_Buffer.end(); ++row)
    {
        for(std::vector<CGXDLMSVariant>::iterator cell = row->begin(); cell != row->end(); ++cell)
        {
            sb << cell->ToString();
            sb << " | ";
        }
        sb << "\r\n";
    }
    values.push_back(sb.str());
    //Clear str.
    sb.str(std::string());
    sb << '[';
    empty = true;
    for(std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it = m_CaptureObjects.begin(); it != m_CaptureObjects.end(); ++it)
    {
        if (!empty)
        {
            sb << ", ";
        }
        empty = false;
        std::string str = it->first->GetName().ToString();
        sb.write(str.c_str(), str.size());
    }
    sb << ']';
    values.push_back(sb.str());

    values.push_back(CGXDLMSVariant(m_CapturePeriod).ToString());
    values.push_back(CGXDLMSVariant(m_SortMethod).ToString());
    if (m_SortObject == NULL)
    {
        values.push_back("");
    }
    else
    {
        values.push_back(m_SortObject->GetName().ToString());
    }
    values.push_back(CGXDLMSVariant(m_EntriesInUse).ToString());
    values.push_back(CGXDLMSVariant(m_ProfileEntries).ToString());
}

void CGXDLMSProfileGeneric::GetAttributeIndexToRead(std::vector<int>& attributes)
{
    //LN is static and read only once.
    if (CGXDLMSObject::IsLogicalNameEmpty(m_LN))
    {
        attributes.push_back(1);
    }
    //Buffer
    if (!IsRead(2))
    {
        attributes.push_back(2);
    }
    //CaptureObjects
    if (!IsRead(3))
    {
        attributes.push_back(3);
    }
    //CapturePeriod
    if (!IsRead(4))
    {
        attributes.push_back(4);
    }
    //SortMethod
    if (!IsRead(5))
    {
        attributes.push_back(5);
    }
    //SortObject
    if (!IsRead(6))
    {
        attributes.push_back(6);
    }
    //EntriesInUse
    if (!IsRead(7))
    {
        attributes.push_back(7);
    }
    //ProfileEntries
    if (!IsRead(8))
    {
        attributes.push_back(8);
    }
}

//Returns amount of attributes.
int CGXDLMSProfileGeneric::GetAttributeCount()
{
    return 8;
}

//Returns amount of methods.
int CGXDLMSProfileGeneric::GetMethodCount()
{
    return 2;
}

int CGXDLMSProfileGeneric::GetDataType(int index, DLMS_DATA_TYPE& type)
{
    if (index == 1)
    {
        type = DLMS_DATA_TYPE_OCTET_STRING;
        return ERROR_CODES_OK;
    }
    if (index == 2)
    {
        type = DLMS_DATA_TYPE_ARRAY;
        return ERROR_CODES_OK;
    }
    if (index == 3)
    {
        type = DLMS_DATA_TYPE_ARRAY;
        return ERROR_CODES_OK;
    }
    if (index == 4)
    {
        type = DLMS_DATA_TYPE_INT8;
        return ERROR_CODES_OK;
    }
    if (index == 5)
    {
        type = DLMS_DATA_TYPE_INT8;
        return ERROR_CODES_OK;
    }
    if (index == 6)
    {
        type = DLMS_DATA_TYPE_ARRAY;
        return ERROR_CODES_OK;
    }
    if (index == 7)
    {
        type = DLMS_DATA_TYPE_UINT32;
        return ERROR_CODES_OK;
    }
    if (index == 8)
    {
        type = DLMS_DATA_TYPE_UINT32;
        return ERROR_CODES_OK;
    }
    return ERROR_CODES_INVALID_PARAMETER;
}

/*
* Returns value of given attribute.
*/
int CGXDLMSProfileGeneric::GetValue(int index, int selector, CGXDLMSVariant& parameters, CGXDLMSVariant& value)
{
    if (index == 1)
    {
        return GetLogicalName(this, value);
    }
    if (index == 2)
    {
        CGXByteBuffer tmp;
        tmp.AddRange(value.byteArr, value.size);
        return GetProfileGenericData(selector, parameters, tmp);
    }
    if (index == 3)
    {
        CGXByteBuffer data;
        return GetColumns(data);
    }
    if (index == 4)
    {
        value = GetCapturePeriod();
        return ERROR_CODES_OK;
    }
    if (index == 5)
    {
        value = GetSortMethod();
        return ERROR_CODES_OK;
    }
    if (index == 5)
    {
        return ERROR_CODES_INVALID_PARAMETER;
    }
    if (index == 7)
    {
        value = GetEntriesInUse();
        return ERROR_CODES_OK;
    }
    if (index == 8)
    {
        value = GetProfileEntries();
        return ERROR_CODES_OK;
    }
    return ERROR_CODES_INVALID_PARAMETER;
}

/*
 * Set value of given attribute.
 */
int CGXDLMSProfileGeneric::SetValue(CGXDLMSSettings* settings, int index, CGXDLMSVariant& value)
{
    int ret;
    if (index == 1)
    {
        return SetLogicalName(this, value);
    }
    else if (index == 2)
    {
        if (m_CaptureObjects.size() == 0)
        {
            //Read capture objects first.
            return ERROR_CODES_INVALID_PARAMETER;
        }
        m_Buffer.clear();
        if (value.vt != DLMS_DATA_TYPE_NONE)
        {
            std::vector<DLMS_DATA_TYPE> types;
            DLMS_DATA_TYPE type;
            for (std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it = m_CaptureObjects.begin(); it != m_CaptureObjects.end(); ++it)
            {
                if ((ret = (*it).first->GetUIDataType((*it).second->GetAttributeIndex(), type)) != 0)
                {
                    return ret;
                }
                types.push_back(type);
            }

            for (std::vector<CGXDLMSVariant >::iterator row = value.Arr.begin(); row != value.Arr.end(); ++row)
            {
                if ((*row).Arr.size() != m_CaptureObjects.size())
                {
                    //Number of columns do not match.
                    return ERROR_CODES_INVALID_PARAMETER;
                }
                CGXDLMSVariant data;
                for(unsigned int pos = 0; pos < (*row).Arr.size(); ++pos)
                {
                    DLMS_DATA_TYPE type = types[pos];
                    if (type != DLMS_DATA_TYPE_NONE && row->Arr[pos].vt == DLMS_DATA_TYPE_OCTET_STRING)
                    {
                        if ((ret = CGXDLMSClient::ChangeType(row->Arr[pos], type, data)) != 0)
                        {
                            return ret;
                        }
                        row->Arr[pos] = data;
                    }
                    std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> item = m_CaptureObjects[pos];
                    if (item.first->GetObjectType() == OBJECT_TYPE_REGISTER && item.second->GetAttributeIndex() == 2)
                    {
                        double scaler = ((CGXDLMSRegister*) item.first)->GetScaler();
                        if (scaler != 1)
                        {
                            row->Arr[pos] = row->Arr[pos].ToDouble() * scaler;
                        }
                    }
                }
                m_Buffer.push_back(row->Arr);
            }
        }
        m_EntriesInUse = m_Buffer.size();
    }
    else if (index == 3)
    {
        m_CaptureObjects.clear();
        m_Buffer.clear();
        m_EntriesInUse = 0;
        if (value.vt == DLMS_DATA_TYPE_ARRAY)
        {
            for (std::vector<CGXDLMSVariant >::iterator it = value.Arr.begin(); it != value.Arr.end(); ++it)
            {
                if ((*it).Arr.size() != 4)
                {
                    //Invalid structure format.
                    return ERROR_CODES_INVALID_PARAMETER;
                }
                OBJECT_TYPE type = (OBJECT_TYPE) (*it).Arr[0].ToInteger();
                std::string ln;
                GXHelpers::GetLogicalName((*it).Arr[1].byteArr, ln);
                CGXDLMSObject* pObj = NULL;
                if (settings != NULL)
                {
                    pObj = settings->GetObjects().FindByLN(type, ln);
                }
                if(pObj == NULL)
                {
                    pObj = CGXDLMSObjectFactory::CreateObject(type, ln);
                }
                AddCaptureObject(pObj, (*it).Arr[2].ToInteger(), (*it).Arr[3].ToInteger());
            }
        }
    }
    else if (index == 4)
    {
        m_CapturePeriod = value.ToInteger();
    }
    else if (index == 5)
    {
        m_SortMethod = (GX_SORT_METHOD) value.ToInteger();
    }
    else if (index == 6)
    {
        if (value.vt == DLMS_DATA_TYPE_NONE)
        {
            m_SortObject = NULL;
        }
        else
        {
            if (value.Arr.size() != 4)
            {
                //Invalid structure format.
                return ERROR_CODES_INVALID_PARAMETER;
            }
            OBJECT_TYPE type = (OBJECT_TYPE) value.Arr[0].ToInteger();
            std::string ln;
            GXHelpers::GetLogicalName(value.Arr[1].byteArr, ln);
            int attributeIndex = value.Arr[2].ToInteger();
            int dataIndex = value.Arr[3].ToInteger();
            m_SortObject = NULL;
            if (settings != NULL)
            {
                m_SortObject = settings->GetObjects().FindByLN(type, ln);
            }
            if(m_SortObject == NULL)
            {
                m_SortObject = CGXDLMSObjectFactory::CreateObject(type, ln);
            }
            m_SortObjectAttributeIndex = attributeIndex;
            m_SortObjectDataIndex = dataIndex;
        }
    }
    else if (index == 7)
    {
        m_EntriesInUse = value.ToInteger();
    }
    else if (index == 8)
    {
        m_ProfileEntries = value.ToInteger();
    }
    else
    {
        return ERROR_CODES_INVALID_PARAMETER;
    }
    return ERROR_CODES_OK;
}
