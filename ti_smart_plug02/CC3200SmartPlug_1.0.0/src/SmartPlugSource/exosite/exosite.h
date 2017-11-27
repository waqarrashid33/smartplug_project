/*****************************************************************************
*
*  exosite.h - Exosite library interface header
*  Copyright (C) 2012 Exosite LLC
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Exosite LLC nor the names of its contributors may
*    be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

#ifndef EXOSITE_H
#define EXOSITE_H

#include <stdint.h>

// defines
enum UUIDInterfaceTypes
{
    IF_WIFI,
    IF_ENET,
    IF_FILE,
    IF_HDD,
    IF_I2C,
    IF_GPRS,
    IF_NONE
};

enum ExositeStatusCodes
{
    EXO_STATUS_OK,
    EXO_STATUS_INIT,
    EXO_STATUS_BAD_SN,
    EXO_STATUS_CONFLICT,
    EXO_STATUS_BAD_CIK,
    EXO_STATUS_NOAUTH,
    EXO_STATUS_END
};

typedef struct
{
  int           g_ClientSoc;
  OsiLockObj_t  g_ClientLockObj;

}s_ExoConnectionStatus;

typedef enum ExositeConnect
{
    EXO_DEFAULT_CON,
    EXO_SECURE_CON,
    EXO_NON_SECURE_CON
}ExositeConnect;

enum ExoReadDataEnum
{
    E_EXO_DATA_COMMAND = 0,
    E_EXO_DATA_CONTROL,
    E_EXO_DATA_REPOINT,
    E_EXO_DATA_POWMODE,
    E_EXO_DATA_POWRTHD,
    E_EXO_DATA_ENRGTHD,
    E_EXO_DATA_SCHEDUL
};

typedef struct
{
  unsigned int            val_flag;
  unsigned char           command;
  unsigned char           control;
  unsigned char           interval;
  unsigned char           psmode;
  float                   powerthd;
  float                   energythd;
  t_weekschedule          schedule;
  t_smartplugtime         schdTime;
  UINT8                   schdValidity;

}t_ExositeReadData;

#define EXOSITE_DEMO_UPDATE_INTERVAL            4000// ms
#define CIK_LENGTH                              META_CIK_SIZE

#define EXO_READ_COMMAND                        "control&command"
#define EXO_READ_DEV_INFO                       "control&reportint&powmode&powthld&enthld&schedule"

#define EXO_COMMAND_TOTL_DATA                   2
#define EXO_DEV_INFO_TOTL_DATA                  6

#define EXO_DATA_COMMAND                        "command"
#define EXO_DATA_CONTROL                        "control"
#define EXO_DATA_REPOINT                        "reportint"
#define EXO_DATA_POWMODE                        "powmode"
#define EXO_DATA_POWRTHD                        "powthld"
#define EXO_DATA_ENRGTHD                        "enthld"
#define EXO_DATA_SCHEDULE                       "schedule"

// functions for export
int Exosite_Write(char * pbuf, unsigned int bufsize);
int Exosite_Read(char * palias, t_ExositeReadData *ExodataRead);
int Exosite_Init(const unsigned char if_nbr, int reset);
int Exosite_Activate(void);
void Exosite_SetCIK(char * pCIK);
int Exosite_GetCIK(char * pCIK);
int Exosite_StatusCode(void);
int Exosite_GetResponse(void);
void Exosite_SetMRF(char *buffer, int length);
void Exosite_GetMRF(char *buffer, int length);
int Exosite_Time_Read(t_smartplugdate *SmartPlugDate);

int info_assemble(const char * vendor, const char *model, const char *sn);
int init_UUID(unsigned char if_nbr);
int get_http_status(long socket);
long connect_to_exosite(ExositeConnect con_type);
int sendLine(long socket, unsigned char LINE, const char * payload);

#endif





