/*******************************************************************************
* ALMA - Atacama Large Millimeter Array
* (c) Associated Universities Inc., 2005
*
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Lesser General Public
*License as published by the Free Software Foundation; either
*version 2.1 of the License, or (at your option) any later version.
*
*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Lesser General Public License for more details.
*
*You should have received a copy of the GNU Lesser General Public
*License along with this library; if not, write to the Free Software
*Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
* "@(#) $Id$"
*
*/

#include "NIXNetBusInterface.h"
#include <stdio.h>
#include <nixnet.h>
#include "portable.h"
#include "stringConvert.h"

// --------------------------------------------------------------------------
//private:

const nodeList_t* NIXNetBusInterface::findNodes(AmbChannel channel) {
    // Not shutting down.  Is the channel open?
    if (!channelNodeMap_m.isOpenChannel(channel)) {
        // no. Try opening the channel:
        if (!openChannel(channel)) {
            printf("NIXNetBusInterface::findNodes cannot open channel\n");
            return NULL;
        }
    }
    findChannelNodes(channel);
    return &(channelNodeMap_m.getNodes(channel));
}


bool NIXNetBusInterface::openChannel(AmbChannel channel) {
    bool success = false;
    
    nxSessionRef_t m_InputSessionRef = 0;
    nxSessionRef_t m_OutputSessionRef = 0;

    
    char *l_pSelectedInterface = (std::string "CAN" + to_string(channel)).c_str();
    char *l_pSelectedDatabase = ":memory:";
    char *l_pSelectedCluster = "";
    char *l_pSelectedFrameList = "";

    u8 l_Buffer[250 * sizeof(nxFrameCAN_t)]; // Use a large buffer for stream input
    nxFrameVar_t *l_pFrame = NULL;
    u32 l_NumBytes = 0;
    u64 l_BaudRate = 1000000;
    nxStatus_t l_Status = 0;

    l_Status = nxCreateSession(l_pSelectedDatabase, l_pSelectedCluster,
        l_pSelectedFrameList, l_pSelectedInterface, nxMode_FrameInStream,
        &m_InputSessionRef);
    if (nxSuccess != l_Status) {
        printf("NIXNetBusInterface error creating input stream")        
    }

    l_Status = nxCreateSession(l_pSelectedDatabase, l_pSelectedCluster,
        l_pSelectedFrameList, l_pSelectedInterface, nxMode_FrameOutStream,
        &m_OutputSessionRef);
    if (nxSuccess != l_Status) {
        printf("NIXNetBusInterface error creating output stream")        
    }

    // We are not using a predefined database, so we need to set the Baud Rate
    l_Status = nxSetProperty(m_InputSessionRef, nxPropSession_IntfBaudRate64,
        sizeof(u64), &l_BaudRate);
    if (nxSuccess != l_Status) {
        printf("NIXNetBusInterface error setting input stream properties")
    }
    l_Status = nxSetProperty(m_OutputSessionRef, nxPropSession_IntfBaudRate64,
        sizeof(u64), &l_BaudRate);
    if (nxSuccess != l_Status) {
        printf("NIXNetBusInterface error setting input stream properties")
    }

    // Configure the CAN Network Interface Object:
    AttrIdList[0] =         NC_ATTR_BAUD_RATE;
    AttrValueList[0] =      BaudRate;
    AttrIdList[1] =         NC_ATTR_START_ON_OPEN;
    AttrValueList[1] =      NC_TRUE;
    AttrIdList[2] =         NC_ATTR_READ_Q_LEN;
    AttrValueList[2] =      50;
    AttrIdList[3] =         NC_ATTR_WRITE_Q_LEN;
    AttrValueList[3] =      50;
    AttrIdList[4] =         NC_ATTR_CAN_COMP_STD;
    AttrValueList[4] =      0;
    AttrIdList[5] =         NC_ATTR_CAN_MASK_STD;
    AttrValueList[5] =      NC_CAN_MASK_STD_DONTCARE;
    AttrIdList[6] =         NC_ATTR_CAN_COMP_XTD;
    AttrValueList[6] =      0;
    AttrIdList[7] =         NC_ATTR_CAN_MASK_XTD;
    AttrValueList[7] =      NC_CAN_MASK_XTD_DONTCARE;

    // initialize the CAN interface:

    NCTYPE_STATUS status = ncConfig(ObjName, 8, AttrIdList, AttrValueList);
    success = (status >= 0);
    
    if (!success) {
        printf("NIXNetBusInterface: ncConfig failed. obj=%s status=%d\n", ObjName, (int) status);
        printNCStatus(status, "NIXNetBusInterface::openChannel");
    }
        
    NCTYPE_OBJH handle = 0;
    if (success) {
        status = ncOpenObject(ObjName, &handle);
        success = (status >= 0);
        if (!success) {
            printf("NIXNetBusInterface: ncOpenObject failed. obj=%s status=%d\n", ObjName, (int) status);
            printNCStatus(status, "NIXNetBusInterface::openChannel");
        }
    }

    
    // Set the channel connected status:
    if (success) {
        channelNodeMap_m.setHandle(channel, handle);
        channelNodeMap_m.openChannel(channel);
    }

    return success;        
}

void NIXNetBusInterface::closeChannel(AmbChannel channel) {
    if (channelNodeMap_m.isOpenChannel(channel)) {
        // Close the CAN object:
        NCTYPE_STATUS status = ncCloseObject(channelNodeMap_m.getHandle(channel));
        // Mark the channel closed in the map:
        channelNodeMap_m.closeChannel(channel);        
    }
}

// Do the actual work of clearing the read buffer, sending a monitor request,
// and waiting for a reply before returning.
void NIXNetBusInterface::monitorImpl(unsigned long _handle, AmbMessage_t &msg)
{
    NCTYPE_OBJH handle = _handle; 
        
    // Read and discard any stale data in the read buffer:
    flushReadBuffer(handle);

    // Request the monitor parameter:
    NCTYPE_STATUS status;
    NCTYPE_STATE currentState = 0;
    NCTYPE_CAN_FRAME request;
    memset(&request, 0, sizeof(NCTYPE_CAN_FRAME));
    NCTYPE_CAN_STRUCT response;
    memset(&response, 0, sizeof(NCTYPE_CAN_STRUCT));
    request.ArbitrationId = NC_FL_CAN_ARBID_XTD + msg.address;
    request.IsRemote = 0;
    request.DataLength = 0;
    memset(request.Data, 0, AMB_DATA_MSG_SIZE);

#ifdef MEASURE_CAN_LATENCY
    static bool firstTime(true);
    static LARGE_INTEGER sum, freq, start, end;
    sum.QuadPart = 0;
    static int count = 0;
    static const int averagingSize = 1000;
    if (firstTime) {
    	firstTime = false;
    	QueryPerformanceFrequency(&freq);
    	printf("NIXNetBusInterface::monitorImpl:Ticks\nN\tStart\tEnd\tSum\tAvg\tFreq\n");
    }
    QueryPerformanceCounter(&start);
#endif
    status = ncWrite(handle, sizeof(NCTYPE_CAN_FRAME), &request);

    bool success = false;
    bool timeout = false;
    while (!success && !timeout) {
        // Wait for data ready to read:
        try {
            status = ncWaitForState(handle, NC_ST_READ_AVAIL, monitorTimeout_m, &currentState);
        } catch (...) {
            status = -1;
        }
        if (status >= 0) {

#ifdef MEASURE_CAN_LATENCY
        	QueryPerformanceCounter(&end);
        	sum.QuadPart += (end.QuadPart - start.QuadPart);
        	if (++count == averagingSize) {
        		double avgTicks(double(sum.QuadPart) / count);
        		printf("%d\t%f\t%f\t%f\t%f\t%f\n",
        				count,
						double(start.QuadPart),
						double(end.QuadPart),
						double(sum.QuadPart),
						avgTicks,
						double(freq.QuadPart));
        		sum.QuadPart = 0;
        		count = 0;
        	}
#endif
            status = ncRead(handle, sizeof(NCTYPE_CAN_STRUCT), &response);
        
            if (enableDebug_m) {        
                printf("monitorImpl: %x -> %x [ ", (unsigned) request.ArbitrationId, (unsigned) response.ArbitrationId);
                for (int index = 0; index < response.DataLength; ++index)
                    printf("%02X ", response.Data[index]);
                printf("]\n");
            }

            if (response.ArbitrationId == request.ArbitrationId)
                success = true;
        } else
            timeout = true;
    }
    if (success) {
        // Save the received data length into the caller's pointer:
        if (msg.completion_p -> dataLength_p)
            *(msg.completion_p -> dataLength_p) = response.DataLength;
        // Save the received data:
        if (msg.completion_p -> data_p)
            memcpy(msg.completion_p -> data_p, response.Data, response.DataLength);
        // Result is good status:
        if (msg.completion_p -> status_p)
            *(msg.completion_p -> status_p) = AMBERR_NOERR;
    
    } else if (timeout) {
        // Read timed out or other error.  Return zero data:
        if (msg.completion_p -> dataLength_p)
            *(msg.completion_p -> dataLength_p) = 0;
        if (msg.completion_p -> data_p)
            memset(msg.completion_p -> data_p, 0, AMB_DATA_MSG_SIZE);
        if (msg.completion_p -> status_p)
            *(msg.completion_p -> status_p) = AMBERR_TIMEOUT;
    }
}                                       
  
// Do the actual work of sending a command:
void NIXNetBusInterface::commandImpl(unsigned long _handle, AmbMessage_t &msg)
{
    NCTYPE_OBJH handle = _handle; 

    // Send the command:
    NCTYPE_STATUS status;
    NCTYPE_STATE currentState = 0;
    NCTYPE_CAN_FRAME command;
    command.ArbitrationId = NC_FL_CAN_ARBID_XTD + msg.address;
    command.IsRemote = 0;
    command.DataLength = msg.dataLen;
    memset(command.Data, 0, AMB_DATA_MSG_SIZE);
    memcpy(command.Data, msg.data, msg.dataLen);

    if (enableDebug_m) {
        printf("commandImpl: %x [ ", (unsigned) command.ArbitrationId);
        for (int index = 0; index < command.DataLength; ++index)
            printf("%02X ", command.Data[index]);
        printf("]\n");
    }

    try {
        status = ncWrite(handle, sizeof(NCTYPE_CAN_FRAME), &command);
        status = ncWaitForState(handle, NC_ST_WRITE_SUCCESS, monitorTimeout_m, &currentState);
    } catch (...) {
        status = -1;
    }
    if (msg.completion_p -> status_p) {
        if (status >= 0)
            *(msg.completion_p -> status_p) = AMBERR_NOERR;
        else
            *(msg.completion_p -> status_p) = AMBERR_WRITEERR;
    }
}

void NIXNetBusInterface::findChannelNodes(AmbChannel channel) {
    // Check that the channel is open:
    if (channelNodeMap_m.isOpenChannel(channel)) {
        // Drop all known nodes:
        channelNodeMap_m.clearNodes(channel);
        // Get the NICAN object handle:
        NCTYPE_OBJH handle = channelNodeMap_m.getHandle(channel);  
        // clear the CAN read buffer of any garbage:
        flushReadBuffer(handle);

        NCTYPE_CAN_FRAME request;
        request.ArbitrationId = NC_FL_CAN_ARBID_XTD;
        request.IsRemote = 0;
        request.DataLength = 0;
        memset(request.Data, 0, AMB_DATA_MSG_SIZE);
    
        // Write a broadcast request for all nodes to announce themselves:
        NCTYPE_STATUS status = ncWrite(handle, sizeof(NCTYPE_CAN_FRAME), &request);

        unsigned nodeId;
        int node_count = 0;
        NCTYPE_STATE currentState = 0;
        NCTYPE_CAN_STRUCT response;

        // Gather info on all nodes which reply:
        bool done = false;
        while (!done) {
            try {
                status = ncWaitForState(handle, NC_ST_READ_AVAIL, 200, &currentState);
            } catch (...) {
                status = -1;
            }
            if (status < 0)
                done = true;
            else {
                status = ncRead(handle,  sizeof(NCTYPE_CAN_STRUCT), &response);
                if (status >= 0) {
                    ++node_count;
                    nodeId = (response.ArbitrationId - NC_FL_CAN_ARBID_XTD) / 0x40000U - 1U;
                    channelNodeMap_m.addNode(channel, nodeId, response.Data);
                    if (enableDebug_m) {
                        printf("Node %02X (arb. %X):\n"
                               "  SN: %02X%02X%02X%02X%02X%02X%02X%02X\n",
                               static_cast<unsigned int>(nodeId),
                               static_cast<unsigned int>(response.ArbitrationId),
                               response.Data[0], response.Data[1], response.Data[2], response.Data[3],
                               response.Data[4], response.Data[5], response.Data[6], response.Data[7]);
                    }
                }
            }
        }
    }
}

void NIXNetBusInterface::flushReadBuffer(unsigned long _handle) {
    NCTYPE_OBJH handle = _handle;
    NCTYPE_STATUS status;
    NCTYPE_UINT32 attrValue;
    NCTYPE_CAN_STRUCT response;
        
    // Read and discard any stale data in the read buffer:
    bool done = false;
    while (!done) {
        status = ncGetAttribute(handle, NC_ATTR_STATE, sizeof(NCTYPE_UINT32), &attrValue);
        if (attrValue == NC_ST_READ_AVAIL) {
            status = ncRead(handle,  sizeof(NCTYPE_CAN_STRUCT), &response);
            printf("flushed one %d\n", (int) status);
        } else
            done = true;
    }
}

