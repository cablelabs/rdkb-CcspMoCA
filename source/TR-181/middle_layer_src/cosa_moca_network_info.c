/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**************************************************************************

    module: cosa_moca_network_info.c

        For COSA Data Model Library Development

    -------------------------------------------------------------------

    copyright:

        Cisco Systems, Inc.
        All Rights Reserved.

    -------------------------------------------------------------------

    description:

        This file implementes the syncing of MoCA device informations

    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        COSA XML TOOL CODE GENERATOR 1.0

    -------------------------------------------------------------------

    revision:

        01/11/2011    initial revision.

**************************************************************************/

#include <sysevent/sysevent.h>
#include <time.h>
#include "cosa_moca_network_info.h"

#include "ansc_platform.h"
#include "ccsp_mocaLog_wrapper.h"
#include "moca_hal.h"

MoCADeviceList *mocaList = NULL;
pthread_mutex_t mocaListMutex = PTHREAD_MUTEX_INITIALIZER;

extern  ANSC_HANDLE                        bus_handle;
extern char                     g_Subsystem[32];
char deviceMAC[32]={'\0'}; 

int WebpaInterface_DiscoverComponent(char** pcomponentName, char** pcomponentPath )
{
    char CrName[256] = {0};
    int ret = 0;
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s ENTER\n", __FUNCTION__ ));

    CrName[0] = 0;
    strcpy(CrName, "eRT.");
    strcat(CrName, CCSP_DBUS_INTERFACE_CR);

    componentStruct_t **components = NULL;
    int compNum = 0;
    int res = CcspBaseIf_discComponentSupportingNamespace (
            bus_handle,
            CrName,
#ifndef _XF3_PRODUCT_REQ_
            "Device.X_CISCO_COM_CableModem.MACAddress",
#else
            "Device.DPoE.Mac_address",
#endif      
            "",
            &components,
            &compNum);
    if(res != CCSP_SUCCESS || compNum < 1)
    {
        CcspTraceError(("WebpaInterface_DiscoverComponent find eRT PAM component error %d\n", res));
        ret = -1;
    }
    else
    {
        *pcomponentName = AnscCloneString(components[0]->componentName);
        *pcomponentPath = AnscCloneString(components[0]->dbusPath);
        free_componentStruct_t(bus_handle, compNum, components);
        CcspTraceInfo(("WebpaInterface_DiscoverComponent find eRT PAM component %s--%s\n", *pcomponentName, *pcomponentPath));
    }
    
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s EXIT\n", __FUNCTION__ ));
    return ret;
}

char* getDeviceMac()
{
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s ENTER\n", __FUNCTION__ ));

    while(!strlen(deviceMAC))
    {
        int ret = -1, val_size =0,cnt =0;
        char *pcomponentName = NULL, *pcomponentPath = NULL;
        parameterValStruct_t **parameterval = NULL;

#ifndef _XF3_PRODUCT_REQ_
        char *getList[] = {"Device.X_CISCO_COM_CableModem.MACAddress"};
#else
        char *getList[] = {"Device.DPoE.Mac_address"};
#endif

        if (strlen(deviceMAC))
        {
            break;
        }

        CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, Before WebpaInterface_DiscoverComponent ret: %d\n",ret));

        if(pcomponentPath == NULL || pcomponentName == NULL)
        {
            if(-1 == WebpaInterface_DiscoverComponent(&pcomponentName, &pcomponentPath)){
                CcspTraceError(("%s ComponentPath or pcomponentName is NULL, waiting for compponent to come up\n", __FUNCTION__));
                sleep(30);
                continue;
            }
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, WebpaInterface_DiscoverComponent ret: %d  ComponentPath %s ComponentName %s \n",ret, pcomponentPath, pcomponentName));
        }

        ret = CcspBaseIf_getParameterValues(bus_handle,
                    pcomponentName, pcomponentPath,
                    getList,
                    1, &val_size, &parameterval);
        if(ret == CCSP_SUCCESS)
        {
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, val_size : %d\n",val_size));
            for (cnt = 0; cnt < val_size; cnt++)
            {
                CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->parameterName : %s\n",cnt,parameterval[cnt]->parameterName));
                CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->parameterValue : %s\n",cnt,parameterval[cnt]->parameterValue));
                CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->type :%d\n",cnt,parameterval[cnt]->type));
            
            }
            strcpy(deviceMAC, parameterval[0]->parameterValue);
            if(pcomponentName)
            {
                AnscFreeMemory(pcomponentName);
            }
            if(pcomponentPath)
            {
                AnscFreeMemory(pcomponentPath);
            }

        }
        else
        {
            CcspTraceError(("RDK_LOG_ERROR, Failed to get values for %s ret: %d\n",getList[0],ret));
            sleep(10);
        }
     
        CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, Before free_parameterValStruct_t...\n"));
        free_parameterValStruct_t(bus_handle, val_size, parameterval);
        CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, After free_parameterValStruct_t...\n"));        
    }
        
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s EXIT\n", __FUNCTION__ ));

    return deviceMAC;
}


MoCADeviceInfo* FindDeviceInMoCAList(char* device_mac)
{
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  ENTRY [%s] \n", __FUNCTION__ ));
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  Input MAC [%s] \n", device_mac ));

    pthread_mutex_lock(&mocaListMutex);
    if(mocaList)
    {
        MoCADeviceInfo* tmp = mocaList->deviceList;
        while(tmp)
        {
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  tmp device Mac [%s] \n", tmp->deviceMac ));

            if(!strcasecmp(tmp->deviceMac, device_mac))
            {
                pthread_mutex_unlock(&mocaListMutex);
                return tmp;
            }

            tmp = tmp->next;    
        }
        
        pthread_mutex_unlock(&mocaListMutex);
        return NULL;          
    }
    else
    {
        pthread_mutex_unlock(&mocaListMutex);
        return NULL;
    }
  
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  EXIT [%s] \n", __FUNCTION__ ));

}

void add_to_moca_list(MoCADeviceInfo* moca_device)
{
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  ENTRY [%s] \n", __FUNCTION__ ));
    pthread_mutex_lock(&mocaListMutex);

    if(!mocaList)
    {
        mocaList = malloc(sizeof(*mocaList));
        mocaList->deviceList = moca_device;
        mocaList->deviceList->next = NULL;
        CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  New List Add Mac [%s] \n", moca_device->deviceMac ));

    }
    else
    {
        MoCADeviceInfo *prevnode = NULL, *curnode = NULL;
        prevnode = mocaList->deviceList;
        curnode = prevnode->next;

        while(curnode != NULL)
        {
            prevnode = curnode;
            curnode = curnode->next;
        }

        prevnode->next = moca_device;
        prevnode->next->next = NULL;
        CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  Existing List Add Mac [%s] \n", moca_device->deviceMac ));

    }
    
    pthread_mutex_unlock(&mocaListMutex);

    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG,  EXIT [%s] \n", __FUNCTION__ ));

}

ANSC_STATUS
MocaIf_GetAssocDevices
    (
        int               ulInterfaceIndex,
        PULONG                      pulCount,
        PCOSA_DML_MOCA_ASSOC_DEVICE *ppDeviceArray
    )
{
   int ulSize = 0;
   int allocNum = 0;
   int DeviceArrayCount = 0;
    
    if ( !pulCount || !ppDeviceArray )
    {
        return ANSC_STATUS_FAILURE;
    }

    *pulCount      = 0;
    *ppDeviceArray = NULL;

    if ( ulInterfaceIndex == 0 )
    {
        moca_cpe_t cpes[kMoca_MaxCpeList];
        int        pnum_cpes     = 0,
                   iReturnStatus = STATUS_SUCCESS;

        iReturnStatus =  moca_GetMocaCPEs(ulInterfaceIndex, cpes, &pnum_cpes);
        AnscTraceWarning(("MocaIf_GetAssocDevices -- pnum_cpes:%d\n", pnum_cpes));
        if(( iReturnStatus == STATUS_SUCCESS ) && ( 0 < pnum_cpes ))
        {
            iReturnStatus = moca_GetNumAssociatedDevices(ulInterfaceIndex, pulCount);
            AnscTraceWarning(("MocaIf_GetAssocDevices -- ulInterfaceIndex:%lu, pulCount:%lu\n", ulInterfaceIndex, *pulCount));

            if(( iReturnStatus == STATUS_SUCCESS ) && ( 0 < *pulCount ))
            {
                moca_associated_device_t*       pdevice_array  = NULL;
                int                             i;

                /* Allocate the larger of pnum_cpes or pulCount since the */
                /* allocated array will be appended. */

                if (pnum_cpes > *pulCount)
                {
                    allocNum = pnum_cpes;
                }
                else
                {
                    allocNum = *pulCount;
                }

                ulSize = sizeof(COSA_DML_MOCA_ASSOC_DEVICE) * (allocNum);
                    
                *ppDeviceArray = (PCOSA_DML_MOCA_ASSOC_DEVICE)AnscAllocateMemory(ulSize);
                if (*ppDeviceArray)
                {
                    _ansc_memset(*ppDeviceArray, 0, sizeof(ulSize));
                }

                pdevice_array = (moca_associated_device_t *)
                    AnscAllocateMemory
                        (
                            sizeof(moca_associated_device_t) * (allocNum + COSA_DML_MOCA_AssocDeviceSafeguard)
                        );
                if (pdevice_array)
                {
                    _ansc_memset(pdevice_array ,0 , sizeof(moca_associated_device_t) * (allocNum + COSA_DML_MOCA_AssocDeviceSafeguard));
                }
                    
                if ( *ppDeviceArray && pdevice_array )
                {
                    INT                 iReturnStatus   = STATUS_SUCCESS;
                    PCOSA_DML_MOCA_ASSOC_DEVICE pDeviceArray = *ppDeviceArray;

                    iReturnStatus = moca_GetAssociatedDevices(ulInterfaceIndex, &pdevice_array);
                    if ( iReturnStatus == STATUS_SUCCESS )
                    {
                        /* Translate the Data Structures */
                        for (i = 0; i < *pulCount; i++)  
                        {
                           if(pDeviceArray && ((&pdevice_array[i]) != NULL) && (i < allocNum))
                           {
                            memcpy(pDeviceArray->MACAddress,            pdevice_array[i].MACAddress, 6);
                            pDeviceArray->NodeID                      = pdevice_array[i].NodeID;
                            pDeviceArray->PreferredNC                 = pdevice_array[i].PreferredNC;
                            memcpy(pDeviceArray->HighestVersion,        pdevice_array[i].HighestVersion, 64);
                            pDeviceArray->PHYTxRate                   = pdevice_array[i].PHYTxRate;
                            pDeviceArray->PHYRxRate                   = pdevice_array[i].PHYRxRate;
                            pDeviceArray->TxPowerControlReduction     = pdevice_array[i].TxPowerControlReduction;
                            pDeviceArray->RxPowerLevel                = pdevice_array[i].RxPowerLevel;
                            pDeviceArray->TxBcastRate                 = pdevice_array[i].TxBcastRate;
                            pDeviceArray->RxBcastPowerLevel           = pdevice_array[i].RxBcastPowerLevel;
                            pDeviceArray->TxPackets                   = pdevice_array[i].TxPackets;
                            pDeviceArray->RxPackets                   = pdevice_array[i].RxPackets;
                            pDeviceArray->RxErroredAndMissedPackets   = pdevice_array[i].RxErroredAndMissedPackets;
                            pDeviceArray->QAM256Capable               = pdevice_array[i].QAM256Capable;
                            pDeviceArray->PacketAggregationCapability = pdevice_array[i].PacketAggregationCapability;
                            pDeviceArray->RxSNR                       = pdevice_array[i].RxSNR;
                            pDeviceArray->Active                      = pdevice_array[i].Active;
                            pDeviceArray->X_CISCO_COM_RxBcastRate     = pdevice_array[i].RxBcastRate;
                            pDeviceArray->X_CISCO_COM_NumberOfClients = pdevice_array[i].NumberOfClients;
                            ++pDeviceArray;  
                            ++DeviceArrayCount;
                           }
                        }

                        /* Append missing entries */

                        if(pnum_cpes != *pulCount)
                        {
                           char mac[18];
                           char mac1[18];
                           int j = 0,k = 0;
                           int appendEntry = FALSE;

                           memset(mac,0,sizeof(mac));
                           memset(mac1,0,sizeof(mac));

                           *pulCount = DeviceArrayCount;

                           for(j =0; j < pnum_cpes; j++)
                           {

#if defined(_COSA_BCM_MIPS_)
                               sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", 0xff & cpes[j].mac_addr[0],0xff & cpes[j].mac_addr[1], 0xff & cpes[j].mac_addr[2], 0xff & cpes[j].mac_addr[3], 0xff & cpes[j].mac_addr[4],0xff & cpes[j].mac_addr[5]);
#else
                               sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",cpes[j].mac_addr[0],cpes[j].mac_addr[1],cpes[j].mac_addr[2],cpes[j].mac_addr[3],cpes[j].mac_addr[4],cpes[j].mac_addr[5]);
#endif
                               appendEntry = FALSE;

                               /* Check all MACS in cpes and append if missing from pdevice_array */

                               for(k = 0; k < i; k++)
                               {

                                    sprintf(mac1, "%02x:%02x:%02x:%02x:%02x:%02x",pdevice_array[k].MACAddress[0],pdevice_array[k].MACAddress[1],pdevice_array[k].MACAddress[2],pdevice_array[k].MACAddress[3],pdevice_array[k].MACAddress[4],pdevice_array[k].MACAddress[5]);

                                    if(!strncmp(mac1, mac, 18))
					                {
					                    appendEntry = FALSE;
					                    break;
					                }
					                else
					                    appendEntry = TRUE;

                                }

                                /* Append the missing entry onto the pDeviceArray */

				                if(appendEntry == TRUE) 
			            	    {
                                    if((DeviceArrayCount < allocNum) && pDeviceArray)
                                    {
                                        AnscTraceWarning(("MocaIf_GetAssocDevices -- appending entry\n"));
                                        memcpy(pDeviceArray->MACAddress, cpes[j].mac_addr, 6);
                                        ++pDeviceArray;
                                        ++DeviceArrayCount;

                                        /* The returned count is the actual number of elements in the returned array */

                                        *pulCount = DeviceArrayCount;
                                    }
                                }
                              
                                memset(mac,0,sizeof(mac));
                            } 
                        }

						if ( pdevice_array )
						{
                            AnscFreeMemory(pdevice_array);
						}
                        return  ANSC_STATUS_SUCCESS;
                    }
                    else
                    {
                        if ( pdevice_array )
                        {
                            AnscFreeMemory(pdevice_array);
                        }
                        if ( *ppDeviceArray )
                        {
                            AnscFreeMemory(*ppDeviceArray);
                            *ppDeviceArray = NULL;
                            *pulCount      = 0;
                        }

                        return iReturnStatus;
                    }
                }
                else
                {
                    AnscTraceWarning(("MocaIf_GetAssocDevices -- Memory Allocation Failure "
                                        "ulInterfaceIndex:%lu, pulCount:%lu\n", ulInterfaceIndex, *pulCount));

                    if ( pdevice_array )
                    {
                        AnscFreeMemory(pdevice_array);
                    }
                    if ( *ppDeviceArray )
                    {
                        AnscFreeMemory(*ppDeviceArray);
                        *ppDeviceArray = NULL;
                        *pulCount      = 0;
                    }

                    return  ANSC_STATUS_RESOURCES;
                }
            }
        }
        else
        {
            *pulCount = 0;
        }
    }


    return ANSC_STATUS_SUCCESS;
}

void Set_MoCADevices_Status_Online(char* assoc_device_mac, int assoc_dev_num)
{
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s ENTER\n", __FUNCTION__ ));
    char paramname[128];
    snprintf(paramname, sizeof(paramname), "Device.MoCA.Interface.1.AssociatedDevice.%d.", assoc_dev_num+1);

    MoCADeviceInfo* device_info = FindDeviceInMoCAList(assoc_device_mac);

    if(!device_info)
    {
        MoCADeviceInfo *moca_device = malloc(sizeof(*moca_device));
        memset(moca_device, 0, sizeof(*moca_device));

        moca_device->deviceMac = strdup(assoc_device_mac);
        getDeviceMac();

        if(deviceMAC)
            moca_device->parentMac = strdup(deviceMAC);

        moca_device->AssociatedDevice = strdup(paramname);
        moca_device->ssidType = strdup("Device.MoCA.Interface.1.");
        moca_device->deviceType = strdup("empty");
        moca_device->RSSI = 0;
        moca_device->Status = 1;
        moca_device->Updated = 1;
        moca_device->StatusChange = 1;
        add_to_moca_list(moca_device);
    }
    else
    {
        if(device_info->Status)
        {
            device_info->StatusChange = 0;
            device_info->Updated = 1;
        }
        else
        {
            device_info->Status = 1;
            device_info->Updated = 1;
            device_info->StatusChange = 1;
            strcpy(device_info->AssociatedDevice, paramname);
            strcpy(device_info->ssidType, "Device.MoCA.Interface.1.");
        }
    }
}          

void Set_MoCADevices_Status_Offline()
{
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s ENTER\n", __FUNCTION__ ));
    
    pthread_mutex_lock(&mocaListMutex);

    if(mocaList)
    {
        MoCADeviceInfo* cur = mocaList->deviceList;
        while(cur != NULL)
        {
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->Status %d \n", __FUNCTION__ , cur->Status));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->Updated %d \n", __FUNCTION__ , cur->Updated));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->StatusChange  %d \n", __FUNCTION__ , cur->StatusChange ));
            
            if(!cur->Updated && cur->Status)
                {
                    cur->Status = 0;
                    strcpy(cur->AssociatedDevice, " ");
                    strcpy(cur->ssidType, " ");
                    cur->Updated = 1;
                    cur->StatusChange = 1;
                }

            cur = cur->next;
        }
    }

    pthread_mutex_unlock(&mocaListMutex);
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s EXIT\n", __FUNCTION__ ));
}


void Send_Update_to_LMLite(BOOL defaultSend)
{
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s ENTER defaultSend[%d] \n", __FUNCTION__, defaultSend ));
    
    pthread_mutex_lock(&mocaListMutex);

    if(mocaList)
    {
        CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s mocaList->deviceList %x \n", __FUNCTION__ , mocaList->deviceList));
        MoCADeviceInfo* cur = mocaList->deviceList;
        while(cur != NULL)
        {
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->deviceMac %s \n", __FUNCTION__ , cur->deviceMac));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->parentMac %s \n", __FUNCTION__ , cur->parentMac));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->ssidType %s \n", __FUNCTION__ , cur->ssidType));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->AssociatedDevice %s \n", __FUNCTION__ , cur->AssociatedDevice));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->RSSI %d \n", __FUNCTION__ , cur->RSSI));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->Status %d \n", __FUNCTION__ , cur->Status));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->Updated %d \n", __FUNCTION__ , cur->Updated));
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s cur->StatusChange  %d \n", __FUNCTION__ , cur->StatusChange ));

            if(cur->StatusChange || defaultSend == TRUE)
            {
                parameterValStruct_t notif_val[1];
                char                 param_name[256] = "Device.Hosts.X_RDKCENTRAL-COM_LMHost_Sync_From_MoCA";
                char                 component[256]  = "eRT.com.cisco.spvtg.ccsp.lmlite";
                char                 bus[256]        = "/com/cisco/spvtg/ccsp/lmlite";
                char                 str[2048]       = {0};
                char*                faultParam      = NULL;
                int                  ret             = 0; 
                

                /* 
                * Group Received Associated Params as below,
                * MAC_Address,AssociatedDevice_Alias,SSID_Alias,ParentMac,DeviceType,RSSI_Signal_Strength,Status
                */
                if(cur->Status)
		{
                snprintf(str, sizeof(str), "%s,%s,%s,%s,%s,%d,%d", 
                                            cur->deviceMac, 
                                            (NULL != cur->AssociatedDevice) ? cur->AssociatedDevice : "NULL", 
                                            (NULL != cur->ssidType) ? cur->ssidType : "NULL", 
                                            (NULL != cur->parentMac) ? cur->parentMac : "NULL", 
                                            (NULL != cur->deviceType) ? cur->deviceType : "NULL", 
                                            cur->RSSI,
                                            cur->Status);
		}
		else
		{
                snprintf(str, sizeof(str), "%s,%s,%s,%s,%s,%d,%d",
                                            cur->deviceMac,
                                            (NULL != cur->AssociatedDevice) ? cur->AssociatedDevice : "NULL",
                                            (NULL != cur->ssidType) ? cur->ssidType : "NULL",
                                            (NULL != cur->parentMac) ? cur->parentMac : "NULL",
                                            "Device.MoCA.Interface.1.",
                                            cur->RSSI,
                                            cur->Status);

		}
                
                AnscTraceWarning(("RDK_LOG_WARN, %s-%d [%s] \n",__FUNCTION__,__LINE__,str));

                notif_val[0].parameterName  = param_name;
                notif_val[0].parameterValue = str;
                notif_val[0].type           = ccsp_string;
                
                ret = CcspBaseIf_setParameterValues(  bus_handle,
                                                      component,
                                                      bus,
                                                      0,
                                                      0,
                                                      notif_val,
                                                      1,
                                                      TRUE,
                                                      &faultParam
                                                      );

                if(ret == CCSP_SUCCESS)
                {
                    cur->Updated = 0;
                    cur->StatusChange = 0;
                    CcspTraceWarning(("%s : Success \n",__FUNCTION__));
                    printf("%s : Success\n",__FUNCTION__);
                }
                else
                {
                    CcspTraceWarning(("%s : Failed ret %d\n",__FUNCTION__,ret));
                    CcspMoCAConsoleTrace(("RDK_LOG_WARN, CcspMoCA : Sending Notification Fail [%d] \n", ret));
                    printf("%s : Failed ret %d\n",__FUNCTION__,ret);
                }

            }
            else
            {
                cur->Updated = 0;
            }

        cur = cur->next;    
        }        
    }

    pthread_mutex_unlock(&mocaListMutex);

    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s EXIT\n", __FUNCTION__ ));

}



void CleanupMoCAList()
{
    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s ENTER\n", __FUNCTION__ ));
    
    pthread_mutex_lock(&mocaListMutex);

    if(mocaList)
    {
        MoCADeviceInfo* cur = mocaList->deviceList;
        MoCADeviceInfo* next = NULL;

        while(cur != NULL)
        {
            next = cur->next;

            if(cur->ssidType)
                free(cur->ssidType);

            if(cur->AssociatedDevice)            
                free(cur->AssociatedDevice);

            if(cur->deviceMac)
                free(cur->deviceMac);

            if(cur->parentMac)
                free(cur->parentMac);

            if(cur->deviceType)
                free(cur->deviceType);

            free(cur);

            cur = next;
        }

        free(mocaList);
        mocaList = NULL;
    }

    pthread_mutex_unlock(&mocaListMutex);

    CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, CcspMoCA %s EXIT\n", __FUNCTION__ ));
}

void* SynchronizeMoCADevices(void *arg)
{
    ANSC_STATUS ret;
    ULONG ulCount = 0;
    int ulInterfaceIndex = 0;
    PCOSA_DML_MOCA_ASSOC_DEVICE ppDeviceArray = NULL;
    PCOSA_DML_MOCA_ASSOC_DEVICE ps = NULL;
    char CpeMacHoldingBuf[ 20 ] = {0};
    int i = 0;

    while(TRUE)
    {
        ulCount = 0;
        ret = MocaIf_GetAssocDevices(ulInterfaceIndex, &ulCount, &ppDeviceArray);
        if(ret == ANSC_STATUS_SUCCESS && ppDeviceArray && ulCount > 0)
        {
            CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, SynchronizeMoCADevices  ulCount [%d] \n", ulCount));

            for(i = 0, ps = ppDeviceArray;  i < ulCount; i++, ps++)
            {
                memset(CpeMacHoldingBuf, 0, sizeof CpeMacHoldingBuf);

                _ansc_sprintf
                    (
                        CpeMacHoldingBuf,
                        "%02X:%02X:%02X:%02X:%02X:%02X",
                        ps->MACAddress[0],
                        ps->MACAddress[1],
                        ps->MACAddress[2],
                        ps->MACAddress[3],
                        ps->MACAddress[4],
                        ps->MACAddress[5]
                    );

                CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, SynchronizeMoCADevices  MACAddress [%s] \n", CpeMacHoldingBuf));
                if(/* ( NULL != ps->MACAddress ) && \
					( '\0' != ps->MACAddress[ 0 ] ) && \*/
					( strcmp(CpeMacHoldingBuf, "00:00:00:00:00:00") )
				   )
            	{
                    Set_MoCADevices_Status_Online(CpeMacHoldingBuf, i);                    
            	}
            }

            Set_MoCADevices_Status_Offline();
            Send_Update_to_LMLite(FALSE);
        }
        else
        {
            if(mocaList)
            {
                Set_MoCADevices_Status_Offline();
                Send_Update_to_LMLite(FALSE);                
                CleanupMoCAList();
            }
        }

        if(ppDeviceArray)
        {
            AnscFreeMemory(ppDeviceArray);
            ppDeviceArray = NULL;            
        }
        
        CcspMoCAConsoleTrace(("RDK_LOG_DEBUG, SynchronizeMoCADevices  Sleeping MOCA_POLLINGINTERVAL secs: ulCount [%d] ret[%d] \n", ulCount, ret));

        sleep(MOCA_POLLINGINTERVAL);
    }

}
