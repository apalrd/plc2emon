/*
 * plc2emon.c
 * Entry point of plc2emon
 *
 * This program reads data from a PLC over Modbus TCP
 * and publishes the results to emoncms.
 *
 * It requires a configuration file named ow2emon.conf in the same directory
 * as the executable.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>


/**
 * Configuration Section
 **/

/* emoncms server name (not including http://) */
const char EmonServer[] = "emoncms.palnet.net";

/* Path to input php */
const char EmonPath[] = "emoncms";

/* Port number for emon HTTP server */
const int EmonPort = 80;

/* PLC to connect to */
const char Plc[] = "septicplc.palnet.net";
const int PlcPort = 502;

/* Node name */
const char EmonNode[] = "iridium_plc";

/* emoncms key */
const char EmonKey[] = "ef803cb2a1c0c7bc3f3c5cd085693ae6";

/* Names associated with each sensor above */
const char * EmonParamNames[] = {
    "X001_OFF_NO",
    "X002_OFF_NC",
    "X003_ON_NO",
    "X004_ON_NC",
    "X005_ALARM_NO",
    "X006_ALARM_NC",
    "X007_PUMP_POWER_SENSE",
    "Y001_PUMP_RELAY",
    "Y002_ALARM_CLEAR_RELAY",
    "Y003_SOFT_FAULT",
    "C1_OFF_FLOAT",
    "C2_ON_FLOAT",
    "C3_ALARM_FLOAT",
    "C7_PUMP_REQUEST_ON",
    "C8_PUMP_REQUEST",
    "C9_ALARM_LATCH",
};

/* Modbus struct */
typedef struct
{
    uint16_t TransID;
    uint16_t ProtoID;
    uint16_t Length;
    uint8_t  UnitID;
    uint8_t  Func;
    uint8_t  Data[];
} ModbusHdr_t;

int main(void)
{
    
    uint8_t * Buf;
    uint8_t Temp[1024];
    ModbusHdr_t * ModHdr = &Temp;
    size_t S;
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    uint32_t DataBits[3];

    /* Continue forever */
    while(1)
    {

        /* Open socket to use for connections */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
        {
            printf("ERROR: Failed to open socket\n");
            return -1;
        }
        
        /* Get PLC by name */
        server = gethostbyname(Plc);

        if(server == NULL)
        {
            printf("ERROR: Failed to resolve hostname %s\n",Plc);
            return -1;
        }

        printf("If we got here, we got the hostname\n");

        /* Setup socket parameters */
        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
        serv_addr.sin_port = htons(PlcPort);

        /* Connect to PLC */
        if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("ERROR: Failed to connect to PLC\n");
            return -1;
        }

        printf("If we got here, we got the connection\n");

        /* Read raw inputs (0x0 base) - 8 inputs */
        ModHdr->TransID = htons(1);
        ModHdr->ProtoID = htons(0);
        ModHdr->Length = htons(6);
        ModHdr->UnitID = 0xFF;
        ModHdr->Func = 0x2;
        *(uint16_t *)(&ModHdr->Data[0]) = htons(0);
        *(uint16_t *)(&ModHdr->Data[2]) = htons(8);
    
        /* Total length is Length field + 4 (for TransID/ProtoID) */
        n = write(sockfd,Temp,6+6);
        printf("Put %d bytes: TID %x %x ProtoID %x %x Len %x %x UID %x Func %x Data %x %x %x %x\n",
                n, Temp[0],Temp[1],Temp[2],Temp[3],Temp[4],Temp[5],Temp[6],Temp[7],
                Temp[8],Temp[9],Temp[10],Temp[11]);

        /* Read what we get back */
        n = read(sockfd,Temp,1024);
        printf("Got %d bytes: TID %x %x ProtoID %x %x Len %x %x UID %x Func %x Data %x %x %x %x\n",
                n, Temp[0],Temp[1],Temp[2],Temp[3],Temp[4],Temp[5],Temp[6],Temp[7],
                Temp[8],Temp[9],Temp[10],Temp[11]);

        /* If the func returned is valid (MSB not set), log data */
        if(ModHdr->Func & 0x80)
        {
            printf("PLC returned error\n");
            DataBits[0] = 0x80000000;
        }
        else
        {
            printf("PLC returned data %x\n",Temp[9]);
            DataBits[0] = Temp[9];
        }

        /* Try to read outputs (0x2000 offset) */
        ModHdr->TransID = htons(2);
        ModHdr->ProtoID = htons(0);
        ModHdr->Length = htons(6);
        ModHdr->UnitID = 0xFF;
        ModHdr->Func = 0x1;
        *(uint16_t *)(&ModHdr->Data[0]) = htons(0x2000);
        *(uint16_t *)(&ModHdr->Data[2]) = htons(8);

        
    
        /* Total length is Length field + 4 (for TransID/ProtoID) */
        n = write(sockfd,Temp,6+6);
        printf("Put %d bytes: TID %x %x ProtoID %x %x Len %x %x UID %x Func %x Data %x %x %x %x\n",
                n, Temp[0],Temp[1],Temp[2],Temp[3],Temp[4],Temp[5],Temp[6],Temp[7],
                Temp[8],Temp[9],Temp[10],Temp[11]);

        /* Read what we get back */
        n = read(sockfd,Temp,1024);
        printf("Got %d bytes: TID %x %x ProtoID %x %x Len %x %x UID %x Func %x Data %x %x %x %x\n",
                n, Temp[0],Temp[1],Temp[2],Temp[3],Temp[4],Temp[5],Temp[6],Temp[7],
                Temp[8],Temp[9],Temp[10],Temp[11]);

        /* If the func returned is valid (MSB not set), log data */
        if(ModHdr->Func & 0x80)
        {
            printf("PLC returned error\n");
            DataBits[1] = 0x80000000;
        }
        else
        {
            printf("PLC returned data %x\n",Temp[9]);
            DataBits[1] = Temp[9];
        }

        /* Try to read internal signals (0x4000 offset) */
        ModHdr->TransID = htons(3);
        ModHdr->ProtoID = htons(0);
        ModHdr->Length = htons(6);
        ModHdr->UnitID = 0xFF;
        ModHdr->Func = 0x1;
        *(uint16_t *)(&ModHdr->Data[0]) = htons(0x4000);
        *(uint16_t *)(&ModHdr->Data[2]) = htons(16);

        
    
        /* Total length is Length field + 4 (for TransID/ProtoID) */
        n = write(sockfd,Temp,6+6);
        printf("Put %d bytes: TID %x %x ProtoID %x %x Len %x %x UID %x Func %x Data %x %x %x %x\n",
                n, Temp[0],Temp[1],Temp[2],Temp[3],Temp[4],Temp[5],Temp[6],Temp[7],
                Temp[8],Temp[9],Temp[10],Temp[11]);

        /* Read what we get back */
        n = read(sockfd,Temp,1024);
        printf("Got %d bytes: TID %x %x ProtoID %x %x Len %x %x UID %x Func %x Data %x %x %x %x\n",
                n, Temp[0],Temp[1],Temp[2],Temp[3],Temp[4],Temp[5],Temp[6],Temp[7],
                Temp[8],Temp[9],Temp[10],Temp[11]);

        /* If the func returned is valid (MSB not set), log data */
        if(ModHdr->Func & 0x80)
        {
            printf("PLC returned error\n");
            DataBits[2] = 0x80000000;
        }
        else
        {
            DataBits[2] = Temp[9] | Temp[10]<<8;
            printf("PLC returned data %x\n",DataBits[2]);
        }
        
        close(sockfd);

        /* Open socket to emoncms */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0)
        {
            printf("ERROR: Failed to open socket to emoncms\n");
            return -1;
        }
        
        /* Get server name */
        server = gethostbyname(EmonServer);
        
        if(server == NULL)
        {
            printf("ERROR: Failed to resolve hostname %s\n",EmonServer);
            return -1;
        }
        
        /* Setup socket parameters */
        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
        serv_addr.sin_port = htons(EmonPort);
        
        /* Connect to server */
        if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("ERROR: Failed to connect to server\n");
            return -1;
        }
        
        /* Write GET and initial input string to the server */
        sprintf(Temp,
                "GET /%s/input/post?node=%s&apikey=%s&fulljson={",
                EmonPath,
                EmonNode,
                EmonKey);
        printf("%s",Temp);
        n = write(sockfd, Temp,strlen(Temp));
        
        /* Write each sensor value */
        int ParamID = 0;
        /* Inputs */
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[0] & 0x1) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[0] & 0x2) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[0] & 0x4) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[0] & 0x8) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[0] & 0x10) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[0] & 0x20) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[0] & 0x40) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        /* Outputs */
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[1] & 0x01) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[1] & 0x02) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[1] & 0x04) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        /* Internal signals */
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[2] & 0x01) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[2] & 0x02) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[2] & 0x04) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[2] & 0x40) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d,",EmonParamNames[ParamID++],(DataBits[2] & 0x80) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
        sprintf(Temp,"\"%s\":%d",EmonParamNames[ParamID++],(DataBits[2] & 0x100) != 0);
        write(sockfd,Temp,strlen(Temp));printf("%s",Temp);
 
        /* Write end of header */
        sprintf(Temp,"} HTTP/1.1\r\nHost: %s\r\n\r\n",EmonServer);
        printf("%s",Temp);
        n = write(sockfd,Temp,strlen(Temp));
        
        printf("Got %d\n",n);
        
        n = read(sockfd, Temp, 1025);
        printf("Got: %d\n",n);
        printf("Returned: %s\n",Temp);
        /* Close socket */
        close(sockfd);

        /* Poll at 30s interval */
        sleep(30);
    }

}
