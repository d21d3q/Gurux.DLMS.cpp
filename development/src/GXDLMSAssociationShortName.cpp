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

#include "../include/GXDLMSVariant.h"
#include "../include/GXDLMSClient.h"
#include "../include/GXDLMSAssociationShortName.h"
#include "../include/GXDLMSObjectFactory.h"

int CGXDLMSAssociationShortName::GetAccessRights(CGXDLMSObject* pObj, CGXByteBuffer& data)
{
    data.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
    data.SetUInt8(3);
    CGXDLMSVariant ln = pObj->GetShortName();
    GXHelpers::SetData(data, DLMS_DATA_TYPE_UINT16, ln);
    data.SetUInt8(DLMS_DATA_TYPE_ARRAY);
    data.SetUInt8(pObj->GetAttributes().size());
    CGXDLMSVariant empty;
    CGXDLMSVariant index, access;
    for(std::vector<CGXDLMSAttribute>::iterator att = pObj->GetAttributes().begin(); att != pObj->GetAttributes().end(); ++att)
    {
        data.SetUInt8(DLMS_DATA_TYPE_STRUCTURE); //attribute_access_item
        data.SetUInt8(3);
        index = att->GetIndex();
        access = att->GetAccess();
        GXHelpers::SetData(data, DLMS_DATA_TYPE_INT8, index);
        GXHelpers::SetData(data, DLMS_DATA_TYPE_ENUM, access);
        GXHelpers::SetData(data, DLMS_DATA_TYPE_NONE, empty);
    }
    data.SetUInt8(DLMS_DATA_TYPE_ARRAY);
    data.SetUInt8(pObj->GetMethodAttributes().size());
    for(std::vector<CGXDLMSAttribute>::iterator it = pObj->GetMethodAttributes().begin(); it != pObj->GetMethodAttributes().end(); ++it)
    {
        data.SetUInt8(DLMS_DATA_TYPE_STRUCTURE); //attribute_access_item
        data.SetUInt8(2);
        index = it->GetIndex();
        access = it->GetAccess();
        GXHelpers::SetData(data, DLMS_DATA_TYPE_INT8, index);
        GXHelpers::SetData(data, DLMS_DATA_TYPE_ENUM, access);
    }
    return ERROR_CODES_OK;
}

void CGXDLMSAssociationShortName::UpdateAccessRights(CGXDLMSVariant& buff)
{
    for(std::vector<CGXDLMSVariant>::iterator access = buff.Arr.begin(); access != buff.Arr.end(); ++access)
    {
        int sn = access->Arr[0].ToInteger();
        CGXDLMSObject* pObj = m_ObjectList.FindBySN(sn);
        if (pObj != NULL)
        {
            for(std::vector<CGXDLMSVariant>::iterator attributeAccess = access->Arr[1].Arr.begin();
                    attributeAccess != access->Arr[1].Arr.end(); ++attributeAccess)
            {
                int id = attributeAccess->Arr[0].ToInteger();
                int tmp = attributeAccess->Arr[1].ToInteger();
                pObj->SetAccess(id, (ACCESSMODE) tmp);
            }
            for(std::vector<CGXDLMSVariant>::iterator methodAccess = access->Arr[2].Arr.begin();
                    methodAccess != access->Arr[2].Arr.end(); ++methodAccess)
            {
                int id = methodAccess->Arr[0].ToInteger();
                int tmp = methodAccess->Arr[1].ToInteger();
                pObj->SetMethodAccess(id, (METHOD_ACCESSMODE) tmp);
            }
        }
    }
}

//Constructor.
CGXDLMSAssociationShortName::CGXDLMSAssociationShortName() : CGXDLMSObject(OBJECT_TYPE_ASSOCIATION_SHORT_NAME)
{
    GXHelpers::SetLogicalName("0.0.40.0.0.255", m_LN);
    m_SN = 0xFA00;
}

CGXDLMSObjectCollection& CGXDLMSAssociationShortName::GetObjectList()
{
    return m_ObjectList;
}

/* TODO:
Object GetAccessRightsList()
{
    return m_AccessRightsList;
}
void SetAccessRightsList(Object value)
{
    m_AccessRightsList = value;
}

Object GetSecuritySetupReference()
{
    return m_SecuritySetupReference;
}
void SetSecuritySetupReference(Object value)
{
    m_SecuritySetupReference = value;
}
*/

void CGXDLMSAssociationShortName::GetValues(std::vector<std::string>& values)
{
    values.clear();
    std::string ln;
    GetLogicalName(ln);
    values.push_back(ln);
    values.push_back(m_ObjectList.ToString());
    values.push_back(m_AccessRightsList.ToString());
    values.push_back(m_SecuritySetupReference);
}

void CGXDLMSAssociationShortName::GetAttributeIndexToRead(std::vector<int>& attributes)
{
    //LN is static and read only once.
    if (CGXDLMSObject::IsLogicalNameEmpty(m_LN))
    {
        attributes.push_back(1);
    }
    //ObjectList is static and read only once.
    if (!IsRead(2))
    {
        attributes.push_back(2);
    }
    //AccessRightsList is static and read only once.
    if (!IsRead(3))
    {
        attributes.push_back(3);
    }
    //SecuritySetupReference is static and read only once.
    if (!IsRead(4))
    {
        attributes.push_back(4);
    }
}

// Returns amount of attributes.
int CGXDLMSAssociationShortName::GetAttributeCount()
{
    return 4;
}

// Returns amount of methods.
int CGXDLMSAssociationShortName::GetMethodCount()
{
    return 8;
}

int CGXDLMSAssociationShortName::GetDataType(int index, DLMS_DATA_TYPE& type)
{
    if (index == 1)
    {
        type = DLMS_DATA_TYPE_OCTET_STRING;
    }
    else if (index == 2)
    {
        type = DLMS_DATA_TYPE_ARRAY;
    }
    else if (index == 3)
    {
        type = DLMS_DATA_TYPE_ARRAY;
    }
    else if (index == 4)
    {
        type = DLMS_DATA_TYPE_OCTET_STRING;
    }
    else
    {
        return ERROR_CODES_INVALID_PARAMETER;
    }
    return ERROR_CODES_OK;
}

// Returns SN Association View.
int CGXDLMSAssociationShortName::GetObjects(CGXByteBuffer& data)
{
    int ret;
    data.SetUInt8(DLMS_DATA_TYPE_ARRAY);
    //Add count
    GXHelpers::SetObjectCount(m_ObjectList.size(), data);
    for(CGXDLMSObjectCollection::iterator it = m_ObjectList.begin(); it != m_ObjectList.end(); ++it)
    {
        data.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
        data.SetUInt8(4);//Count
        CGXDLMSVariant type = (*it)->GetObjectType();
        CGXDLMSVariant version = (*it)->GetVersion();
        CGXDLMSVariant sn = (*it)->GetShortName();
        CGXDLMSVariant ln((*it)->m_LN, 6, DLMS_DATA_TYPE_OCTET_STRING);
        if ((ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_INT16, sn)) != 0 || //base address.
                (ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_UINT16, type)) != 0 || //ClassID
                (ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_UINT8, version)) != 0 || //Version
                (ret = GXHelpers::SetData(data, DLMS_DATA_TYPE_OCTET_STRING, ln)) != 0) //LN
        {
            return ret;
        }
    }
    return ERROR_CODES_OK;
}

int CGXDLMSAssociationShortName::GetValue(int index, int selector, CGXDLMSVariant& parameters, CGXDLMSVariant& value)
{
    if (index == 1)
    {
        return GetLogicalName(this, value);
    }
    else if (index == 2)
    {
        CGXByteBuffer Packets;
        int ret = GetObjects(Packets);
        value = Packets;
        return ret;
    }
    else if (index == 3)
    {
        int ret;
        bool lnExists = m_ObjectList.FindBySN(GetShortName()) != NULL;
        //Add count
        int cnt = m_ObjectList.size();
        if (!lnExists)
        {
            ++cnt;
        }
        CGXByteBuffer data;
        data.SetUInt8(DLMS_DATA_TYPE_ARRAY);
        GXHelpers::SetObjectCount(cnt, data);
        for(std::vector<CGXDLMSObject*>::iterator it = m_ObjectList.begin(); it != m_ObjectList.end(); ++it)
        {
            if ((ret = GetAccessRights(*it, data)) != 0)
            {
                return ret;
            }
        }
        if (!lnExists)
        {
            if ((ret = GetAccessRights(this, data)) != 0)
            {
                return ret;
            }
        }
        value = data;
    }
    else if (index == 4)
    {
        CGXByteBuffer data;
        CGXDLMSVariant tmp = m_SecuritySetupReference;
        GXHelpers::SetData(data, DLMS_DATA_TYPE_OCTET_STRING, tmp);
        value = data;
    }
    else
    {
        return ERROR_CODES_INVALID_PARAMETER;
    }
    return ERROR_CODES_OK;
}

int CGXDLMSAssociationShortName::SetValue(CGXDLMSSettings* settings, int index, CGXDLMSVariant& value)
{
    if (index == 1)
    {
        return SetLogicalName(this, value);
    }
    else if (index == 2)
    {
        m_ObjectList.clear();
        if (value.vt == DLMS_DATA_TYPE_ARRAY)
        {
            for(std::vector<CGXDLMSVariant>::iterator item = value.Arr.begin(); item != value.Arr.end(); ++item)
            {
                int sn = item->Arr[0].ToInteger();
                CGXDLMSObject* pObj = NULL;
                if (settings != NULL)
                {
                    pObj = settings->GetObjects().FindBySN(sn);
                }
                if (pObj == NULL)
                {
                    OBJECT_TYPE type = (OBJECT_TYPE) item->Arr[1].ToInteger();
                    int version = item->Arr[2].ToInteger();
                    std::string ln;
                    GXHelpers::GetLogicalName((*item).Arr[3].byteArr, ln);
                    pObj = CGXDLMSObjectFactory::CreateObject(type, ln);
                    pObj->SetShortName(sn);
                    pObj->SetVersion(version);
                }
                m_ObjectList.push_back(pObj);
            }
        }
    }
    else if (index == 3)
    {
        if (value.vt == DLMS_DATA_TYPE_NONE)
        {
            for(std::vector<CGXDLMSObject*>::iterator it = m_ObjectList.begin(); it != m_ObjectList.end(); ++it)
            {
                for(int pos = 1; pos != (*it)->GetAttributeCount(); ++pos)
                {
                    (*it)->SetAccess(pos, ACCESSMODE_NONE);
                }
            }
        }
        else
        {
            UpdateAccessRights(value);
        }
    }
    else if (index == 4)
    {
        if (value.vt == DLMS_DATA_TYPE_STRING)
        {
            m_SecuritySetupReference = value.ToString();
        }
        else
        {
            int ret;
            CGXDLMSVariant tmp;
            if ((ret = CGXDLMSClient::ChangeType(value, DLMS_DATA_TYPE_OCTET_STRING, tmp)) != 0)
            {
                return ret;
            }
            m_SecuritySetupReference = tmp.ToString();
        }
    }
    else
    {
        return ERROR_CODES_INVALID_PARAMETER;
    }
    return ERROR_CODES_OK;
}
