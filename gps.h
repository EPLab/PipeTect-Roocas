//******************************************************************************
//	 GPS.H
//
//	 Programmed by HONG ROK KIM, CEST 
//	 03/14/2009
//******************************************************************************
#include "define.h"

#define MAX_GPS_FIELD		25
#define GPS_MAX_CMD_LEN		8		// maximum command length (NMEA address)
#define GPS_MAX_DATA_LEN	256		// maximum data length

void GPS_Test(void);
void GPS_Initialize(void);
void GPS_Enable(void);
void GPS_Disable(void);
void GPS_Reset(void);
void GPS_Process(void);
void GPS_ProcessCommand(BYTE *pCommand, BYTE *pData); 
void GPS_ProcessGPGGA(BYTE *pData);
void GPS_ProcessGPGSA(BYTE *pData);
void GPS_ProcessGPRMC(BYTE *pData);
void GPS_ProcessGPRMB(BYTE *pData);
void GPS_ProcessGPGSV(BYTE *pData);
void GPS_ProcessGPZDA(BYTE *pData);
BOOL GPS_GetField(BYTE *pData, BYTE *pField, int nFieldNum, int nMaxFieldLen);
BOOL GPS_CheckValidData(void);
void GPS_WaitValidData(void);

enum GPS_STATE {
	GPS_STATE_SOM = 0,			// Search for start of message
	GPS_STATE_CMD,				// Get command
	GPS_STATE_DATA,				// Get data
	GPS_STATE_CHECKSUM_1,		// Get first checksum character
	GPS_STATE_CHECKSUM_2		// Get second checksum character
};

typedef struct _GPS_NMEA_{
	int nState;						// Current state protocol parser is in
	BYTE bChecksum;					// Calculated NMEA sentence checksum
	BYTE bReceivedChecksum;			// Received NMEA sentence checksum (if exists)
	WORD wIndex;					// Index used for command and data
	BYTE pCommand[GPS_MAX_CMD_LEN];	// NMEA command
	BYTE pData[GPS_MAX_DATA_LEN];	// NMEA data	
	DWORD dwCommandCount;			// number of NMEA commands received (processed or not processed)
	
	BYTE bHour;			        //
	BYTE bMinute;		        //
	BYTE bSecond;		        //
    BYTE wMSecond;              //
        
	double dLatitude;		    // < 0 = South, > 0 = North
	double dLongitude;		    // < 0 = West, > 0 = East
	BYTE bGGAGPSQuality;		// 0 = fix not available, 1 = GPS sps mode, 2 = Differential GPS, SPS mode, fix valid, 3 = GPS PPS mode, fix valid
	BYTE bGGANumOfSatsInUse;	//
	double dGGAHDOP;		    //
	double dGGAAltitude;		// Altitude: mean-sea-level (geoid) meters
	DWORD dwGGACount;		    //
        
	BYTE bRMCDataValid;		    // A = Data valid, V = navigation rx warning
	double dRMCGroundSpeed;		// speed over ground, knots
	double dRMCCourse;		    // course over ground, degrees true
	BYTE bRMCDay;			    //
	BYTE bRMCMonth;			    //
	WORD wRMCYear;			    //
	double dRMCMagVar;		    // magnitic variation, degrees East(+)/West(-)
	DWORD dwRMCCount;		    //
        
} GPS_NMEA;
