//******************************************************************************
//	 GPS.C
//
//	 Programmed by HONG ROK KIM, CEST 
//	 03/14/2009
//******************************************************************************
#include "define.h"
#include "sys.h"
#include "sys_uart.h"
#include "gps.h"
#include "xstream.h"

GPS_NMEA g_GpsNMEA;

void GPS_Test()
{
    char str[25];  
  
    while (1)
    {
        GPS_Process(); 
        if (GPS_CheckValidData() == TRUE) 
        {
            sprintf(str, "%2d:%2d:%2d (%2d/%2d/%4d)\r\n", g_GpsNMEA.bHour, g_GpsNMEA.bMinute, g_GpsNMEA.bSecond, \
                    g_GpsNMEA.bRMCMonth, g_GpsNMEA.bRMCDay, g_GpsNMEA.wRMCYear);
//            XSTREAM_WriteFrame(str, 21);  
        }
    }
}

void GPS_Initialize(void)
{
	GPS_Enable();
    
    GPS_Reset();
}

void GPS_Enable(void)
{
    GPS_ENABLE_PxOUT |= GPS_ENABLE_BIT;
    
}

void GPS_Disable(void)
{
	GPS_ENABLE_PxOUT &= ~GPS_ENABLE_BIT;
}

void GPS_Reset(void)
{
    int i;
  
    GPS_RESET_PxOUT &= ~GPS_RESET_BIT;
    for (i=0; i<1000; i++);
    GPS_RESET_PxOUT |= GPS_RESET_BIT;
}

void GPS_Process(void)
{
	char data;
	
     if (SYS_ReadByteUART(GPS_UART_MODE, &data) == TRUE) {
    
        switch(g_GpsNMEA.nState)
        {
            ///////////////////////////////////////////////////////////////////////
            // Search for start of message '$'
            case GPS_STATE_SOM :
                if (data == '$')
                {
                    g_GpsNMEA.bChecksum = 0;			// reset checksum
                    g_GpsNMEA.wIndex = 0;				// reset index
                    g_GpsNMEA.nState = GPS_STATE_CMD;
                }
                break;
    
            ///////////////////////////////////////////////////////////////////////
            // Retrieve command (NMEA Address)
            case GPS_STATE_CMD :
                if ((data != ',') && (data != '*'))
                {
                    g_GpsNMEA.pCommand[g_GpsNMEA.wIndex++] = data;
                    g_GpsNMEA.bChecksum ^= data;
    
                    // Check for command overflow
                    if(g_GpsNMEA.wIndex >= GPS_MAX_CMD_LEN)
                    {
                        g_GpsNMEA.nState = GPS_STATE_SOM;
                    }
                }
                else
                {
                    g_GpsNMEA.pCommand[g_GpsNMEA.wIndex] = '\0';	// terminate command
                    g_GpsNMEA.bChecksum ^= data;
                    g_GpsNMEA.wIndex = 0;
                    g_GpsNMEA.nState = GPS_STATE_DATA;		// goto get data state
                }
                break;
    
            // Store data and check for end of sentence or checksum flag
            case GPS_STATE_DATA :
                if (data == '*') // checksum flag?
                {
                    g_GpsNMEA.pData[g_GpsNMEA.wIndex] = '\0';
                    g_GpsNMEA.nState = GPS_STATE_CHECKSUM_1;
                }
                else // no checksum flag, store data
                {
                    // Check for end of sentence with no checksum
                    if (data == '\r')
                    {
                        g_GpsNMEA.pData[g_GpsNMEA.wIndex] = '\0';
                        GPS_ProcessCommand(g_GpsNMEA.pCommand, g_GpsNMEA.pData);
                        g_GpsNMEA.nState = GPS_STATE_SOM;
                        return;
                    }
    
                    // Store data and calculate checksum
                    g_GpsNMEA.bChecksum ^= data;
                    g_GpsNMEA.pData[g_GpsNMEA.wIndex] = data;
                    if (++ g_GpsNMEA.wIndex >= GPS_MAX_DATA_LEN) // Check for buffer overflow
                    {
                        g_GpsNMEA.nState = GPS_STATE_SOM;
                    }
                }
                break;
    
            ///////////////////////////////////////////////////////////////////////
            case GPS_STATE_CHECKSUM_1 :
                if ((data - '0') <= 9)
                {
                    g_GpsNMEA.bReceivedChecksum = (data - '0') << 4;
                }
                else
                {
                    g_GpsNMEA.bReceivedChecksum = (data - 'A' + 10) << 4;
                }
    
                g_GpsNMEA.nState = GPS_STATE_CHECKSUM_2;
    
                break;
    
            ///////////////////////////////////////////////////////////////////////
            case GPS_STATE_CHECKSUM_2 :
                if ((data - '0') <= 9)
                {
                    g_GpsNMEA.bReceivedChecksum |= (data - '0');
                }
                else
                {
                    g_GpsNMEA.bReceivedChecksum |= (data - 'A' + 10);
                }
    
                if (g_GpsNMEA.bChecksum == g_GpsNMEA.bReceivedChecksum)
                {
                    GPS_ProcessCommand(g_GpsNMEA.pCommand, g_GpsNMEA.pData);
                }
    
                g_GpsNMEA.nState = GPS_STATE_SOM;
                break;
    
            ///////////////////////////////////////////////////////////////////////
            default : g_GpsNMEA.nState = GPS_STATE_SOM;
        }
    }
}

void GPS_ProcessCommand(BYTE *pCommand, BYTE *pData) 
{
	// GPRMC
	if (strcmp((char *)pCommand, "GPRMC") == NULL)
	{
		GPS_ProcessGPRMC(pData);
	}

        // GPGGA
//	else if (strcmp((char *)pCommand, "GPGGA") == NULL)
//	{
//		GPS_ProcessGPGGA(pData);
//	}

	// GPGSA
//	else if (strcmp((char *)pCommand, "GPGSA") == NULL)
//	{
//		GPS_ProcessGPGSA(pData);
//	}

	// GPGSV
//	else if (strcmp((char *)pCommand, "GPGSV") == NULL)
//	{
//		GPS_ProcessGPGSV(pData);
//	}

	// GPRMB
//	else if (strcmp((char *)pCommand, "GPRMB") == NULL)
//	{
//		GPS_ProcessGPRMB(pData);
//	}

	// GPZDA
//	else if (strcmp((char *)pCommand, "GPZDA") == NULL)
//	{
//		GPS_ProcessGPZDA(pData);
//	}

	g_GpsNMEA.dwCommandCount++;
}

void GPS_ProcessGPGGA(BYTE *pData)
{
	BYTE field[MAX_GPS_FIELD];
	char buf[10];

	// Time
	if (GPS_GetField(pData, field, 0, MAX_GPS_FIELD))
	{
		// Hour
		buf[0] = field[0];
		buf[1] = field[1];
		buf[2] = '\0';
		g_GpsNMEA.bHour = atoi(buf);

		// minute
		buf[0] = field[2];
		buf[1] = field[3];
		buf[2] = '\0';
		g_GpsNMEA.bMinute = atoi(buf);

		// Second
		buf[0] = field[4];
		buf[1] = field[5];
		buf[2] = '\0';
		g_GpsNMEA.bSecond = atoi(buf);

		// Milisecond
		buf[0] = field[7];
		buf[1] = field[8];
		buf[2] = field[9];
		buf[3] = '\0';
		g_GpsNMEA.wMSecond = atoi(buf);
	}

	// Latitude
	if (GPS_GetField(pData, field, 1, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dLatitude = atof((char *)field + 2) / 60.0;
		field[2] = '\0';
		g_GpsNMEA.dLatitude += atof((char *)field);

	}
	if (GPS_GetField(pData, field, 2, MAX_GPS_FIELD))
	{
		if (field[0] == 'S')
		{
			g_GpsNMEA.dLatitude = -g_GpsNMEA.dLatitude;
		}
	}

	// Longitude
	if (GPS_GetField(pData, field, 3, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dLongitude = atof((char *)field + 3) / 60.0;
		field[3] = '\0';
		g_GpsNMEA.dLongitude += atof((char *)field);
	}
	if (GPS_GetField(pData, field, 4, MAX_GPS_FIELD))
	{
		if(field[0] == 'W')
		{
			g_GpsNMEA.dLongitude = -g_GpsNMEA.dLongitude;
		}
	}

	// GPS quality
	if (GPS_GetField(pData, field, 5, MAX_GPS_FIELD))
	{
		g_GpsNMEA.bGGAGPSQuality = field[0] - '0';
	}

	// Satellites in use
	if (GPS_GetField(pData, field, 6, MAX_GPS_FIELD))
	{
		buf[0] = field[0];
		buf[1] = field[1];
		buf[2] = '\0';
		g_GpsNMEA.bGGANumOfSatsInUse = atoi(buf);
	}

	// HDOP
	if (GPS_GetField(pData, field, 7, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dGGAHDOP = atof((char *)field);
	}
	
	// Altitude
	if (GPS_GetField(pData, field, 8, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dGGAAltitude = atof((char *)field);
	}

	g_GpsNMEA.dwGGACount++;
}

void GPS_ProcessGPRMC(BYTE *pData)
{
	BYTE field[MAX_GPS_FIELD];
	char buf[10];

	// Time
	if (GPS_GetField(pData, field, 0, MAX_GPS_FIELD))
	{
		// Hour
		buf[0] = field[0];
		buf[1] = field[1];
		buf[2] = '\0';
		g_GpsNMEA.bHour = atoi(buf);

		// minute
		buf[0] = field[2];
		buf[1] = field[3];
		buf[2] = '\0';
		g_GpsNMEA.bMinute = atoi(buf);

		// Second
		buf[0] = field[4];
		buf[1] = field[5];
		buf[2] = '\0';
		g_GpsNMEA.bSecond = atoi(buf);

		// Milisecond
		buf[0] = field[7];
		buf[1] = field[8];
		buf[2] = field[9];
		buf[3] = '\0';
		g_GpsNMEA.wMSecond = atoi(buf);
	}

	// Data valid
	if(GPS_GetField(pData, field, 1, MAX_GPS_FIELD))
	{
		g_GpsNMEA.bRMCDataValid = field[0];
	}
	else
	{
		g_GpsNMEA.bRMCDataValid = 'V';
	}

        
	// Latitude
	if (GPS_GetField(pData, field, 2, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dLatitude = atof((char *)field + 2) / 60.0;
		field[2] = '\0';
		g_GpsNMEA.dLatitude += atof((char *)field);

	}
	if (GPS_GetField(pData, field, 3, MAX_GPS_FIELD))
	{
		if (field[0] == 'S')
		{
			g_GpsNMEA.dLatitude = -g_GpsNMEA.dLatitude;
		}
	}

	// Longitude
	if (GPS_GetField(pData, field, 4, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dLongitude = atof((char *)field + 3) / 60.0;
		field[3] = '\0';
		g_GpsNMEA.dLongitude += atof((char *)field);
	}
	if (GPS_GetField(pData, field, 5, MAX_GPS_FIELD))
	{
		if(field[0] == 'W')
		{
			g_GpsNMEA.dLongitude = -g_GpsNMEA.dLongitude;
		}
	}

	// Ground speed
	if(GPS_GetField(pData, field, 6, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dRMCGroundSpeed = atof((char *)field);
	}
	else
	{
		g_GpsNMEA.dRMCGroundSpeed = 0.0;
	}

	// course over ground, degrees true
	if(GPS_GetField(pData, field, 7, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dRMCCourse = atof((char *)field);
	}
	else
	{
		g_GpsNMEA.dRMCCourse = 0.0;
	}

	// Date
	if(GPS_GetField(pData, field, 8, MAX_GPS_FIELD))
	{
		// Day
		buf[0] = field[0];
		buf[1] = field[1];
		buf[2] = '\0';
		g_GpsNMEA.bRMCDay = atoi(buf);

		// Month
		buf[0] = field[2];
		buf[1] = field[3];
		buf[2] = '\0';
		g_GpsNMEA.bRMCMonth = atoi(buf);

		// Year (Only two digits. I wonder why?)
		buf[0] = field[4];
		buf[1] = field[5];
		buf[2] = '\0';
		g_GpsNMEA.wRMCYear = atoi(buf) + 2000;
	}

	// course over ground, degrees true
	if(GPS_GetField(pData, field, 9, MAX_GPS_FIELD))
	{
		g_GpsNMEA.dRMCMagVar = atof((char *)field);
	}
	else
	{
		g_GpsNMEA.dRMCMagVar = 0.0;
	}
	if(GPS_GetField(pData, field, 10, MAX_GPS_FIELD))
	{
		if(field[0] == 'W')
		{
			g_GpsNMEA.dRMCMagVar = -g_GpsNMEA.dRMCMagVar;
		}
	}

	g_GpsNMEA.dwRMCCount++;
}

BOOL GPS_GetField(BYTE *pData, BYTE *pField, int nFieldNum, int nMaxFieldLen)
{
	int i = 0, j = 0;
	int field = 0;

	// Validate params
	if (pData == NULL || pField == NULL || nMaxFieldLen <= 0) 
		return FALSE;

	// Go to the beginning of the selected field
	while (field != nFieldNum && pData[i])
	{
		if (pData[i] == ',')
			field++;

		i++;

		if (pData[i] == NULL)
		{
			pField[0] = '\0';
			return FALSE;
		}
	}

	if ((pData[i] == ',') || (pData[i] == '*'))
	{
		pField[0] = '\0';
		return FALSE;
	}

	// copy field from pData to Field
	while ((pData[i] != ',') && (pData[i] != '*') && pData[i])
	{
		pField[j] = pData[i];
		j++; i++;

		// check if field is too big to fit on passed parameter. If it is,
		// crop returned field to its max length.
		if (j >= nMaxFieldLen)
		{
			j = nMaxFieldLen - 1;
			break;
		}
	}
	pField[j] = '\0';

	return TRUE;
}

BOOL GPS_CheckValidData(void)
{
    if (g_GpsNMEA.bRMCDataValid == 'A') 
        return TRUE;
    else 
        return FALSE;
}

void GPS_WaitValidData(void)
{
    while (g_GpsNMEA.bRMCDataValid == 'V');
}

void GPS_ProcessGPGSA(BYTE *pData){}
void GPS_ProcessGPRMB(BYTE *pData){}
void GPS_ProcessGPGSV(BYTE *pData){}
void GPS_ProcessGPZDA(BYTE *pData){}

