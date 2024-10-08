/*
    Driver type: SBIG CCD Camera INDI Driver

    Copyright (C) 2013-2018 Jasem Mutlaq (mutlaqja AT ikarustech DOT com)
    Copyright (C) 2005-2006 Jan Soldan (jsoldan AT asu DOT cas DOT cz)

    Acknowledgement:
    Matt Longmire  (matto AT sbig DOT com)

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

 */

#pragma once

#include "config.h"

#include <indiccd.h>
#include <indifilterinterface.h>

#ifdef __APPLE__
#include <libusb.h>
#include <libsbig/sbigudrv.h>
#else
#include <sbigudrv.h>
#endif

#include <string>

#define DEVICE struct usb_device *

/*
    How to use SBIG CFW-10 SA under Linux:

 1) If you have a RS-232 port on you computer, localize its serial device.
    It may look like this one: /dev/ttyS0
    In the case you use an USB to RS-232 adapter, the device may be
    located at: /dev/ttyUSB0

 2) Make a symbolic link to this device with the following name:
    ln -s /dev/ttyUSB0 ~/kdeedu/kstars/kstars/indi/sbigCFW
    Please note that the SBIG CFW-10 SA _always_ uses device named sbigCFW.

 3) Change mode of this device:
    chmod a+rw /dev/ttyUSB0

 4) Inside KStars:
    a) Open CFW panel
    b) Set CFW Type to CFW-10 SA
    c) Press Connect button
    d) The CFW's detection may take a few seconds due to its internal initialization.
*/

//=============================================================================
const int INVALID_HANDLE_VALUE = -1; // for file operations
//=============================================================================
// SBIG temperature constants:
const double T0                   = 25.000;
const double MAX_AD               = 4096.000;
const double R_RATIO_CCD          = 2.570;
const double R_BRIDGE_CCD         = 10.000;
const double DT_CCD               = 25.000;
const double R0                   = 3.000;
const double R_RATIO_AMBIENT      = 7.791;
const double R_BRIDGE_AMBIENT     = 3.000;
const double DT_AMBIENT           = 45.000;
const double MIN_CCD_TEMP         = -70.0;
const double MAX_CCD_TEMP         = 40.0;
const double CCD_TEMP_STEP        = 0.1;
const double DEF_CCD_TEMP         = 0.0;
const double TEMP_DIFF            = 0.5;
const double CCD_COOLER_THRESHOLD = 95.0;

const double MIN_POLLING_TIME  = 1.0;
const double MAX_POLLING_TIME  = 3600.0;
const double STEP_POLLING_TIME = 1.0;
const double CUR_POLLING_TIME  = 10.0;

// CCD BINNING: (see Sec. 3.2.3 of SBIG Universal Driver documentation)
const int CCD_BIN_1x1_I = 0;
const int CCD_BIN_2x2_I = 1;
const int CCD_BIN_3x3_I = 2;
const int CCD_BIN_1xN_I = 3;  // variable vertical binning modes
const int CCD_BIN_2xN_I = 4;
const int CCD_BIN_3xN_I = 5;
const int CCD_BIN_1x1_E = 6;  // off-chip binning modes
const int CCD_BIN_2x2_E = 7;
const int CCD_BIN_3x3_E = 8;
const int CCD_BIN_9x9_I = 9;
const int CCD_BIN_NxN_I = 10;

const double MIN_EXP_TIME  = 0.0;
const double MAX_EXP_TIME  = 3600.0;
const double EXP_TIME_STEP = 0.01;
const double DEF_EXP_TIME  = 1.0;

#ifdef USE_CFW_AUTO
const int MAX_CFW_TYPES = 17;
#else
const int MAX_CFW_TYPES = 16;
#endif

#define GET_BIG_ENDIAN(p) (((p & 0xff) << 8) | (p >> 8))

typedef enum { CCD_THERMISTOR, AMBIENT_THERMISTOR } THERMISTOR_TYPE;

typedef unsigned long   ulong;            /* Short for unsigned long */

class SBIGCCD : public INDI::CCD, public INDI::FilterInterface
{
    public:
        SBIGCCD();
        virtual ~SBIGCCD() override;

        virtual const char *getDefaultName() override;
        virtual bool initProperties() override;
        virtual void ISGetProperties(const char *dev) override;
        virtual bool updateProperties() override;
        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
        virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
        void updateTemperature();
        static void updateTemperatureHelper(void *);
#ifdef ASYNC_READOUT
        static void *grabCCDHelper(void *context);
#endif
        bool isExposureDone(INDI::CCDChip *targetChip);

        static void NSGuideHelper(void *context);
        static void WEGuideHelper(void *context);

    protected:
        virtual bool Connect() override;
        virtual bool Disconnect() override;

        virtual bool StartExposure(float duration) override;
        virtual bool AbortExposure() override;
        virtual bool UpdateCCDFrame(int x, int y, int w, int h) override;
        virtual bool UpdateCCDBin(int binx, int biny) override;
        virtual bool UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType) override;

        virtual bool StartGuideExposure(float duration) override;
        virtual bool AbortGuideExposure() override;

#ifdef __APPLE__
        libusb_device *dev;
        libusb_device_handle *handle;
#endif

        virtual void TimerHit() override;
        virtual int SetTemperature(double temperature) override;


        virtual bool saveConfigItems(FILE *fp) override;

        virtual bool UpdateGuiderFrame(int x, int y, int w, int h) override;
        virtual bool UpdateGuiderBin(int binx, int biny) override;

        virtual IPState GuideNorth(uint32_t ms) override;
        virtual IPState GuideSouth(uint32_t ms) override;
        virtual IPState GuideEast(uint32_t ms) override;
        virtual IPState GuideWest(uint32_t ms) override;

        // Filter Wheel CFW
        virtual int QueryFilter() override;
        virtual bool SelectFilter(int position) override;

        int m_fd;
        CAMERA_TYPE m_camera_type;
        int m_drv_handle;
        bool m_link_status;
        std::string m_start_exposure_timestamp;

        void InitVars();
        void loadFirmwareOnOSXifNeeded();
        int OpenDriver();
        int CloseDriver();
        unsigned short CalcSetpoint(double temperature);
        double CalcTemperature(short thermistorType, short ccdSetpoint);
        double BcdPixel2double(ulong bcd);

    private:
        DEVICE device;
        char name[MAXINDINAME];

        /////////////////////////////////////////////////////////////////////////////
        /// Product Information & Connection Properties
        /////////////////////////////////////////////////////////////////////////////
        ITextVectorProperty ProductInfoTP;
        IText ProductInfoT[2] {};

        ISwitchVectorProperty PortSP;
        ISwitch PortS[8];
        int SBIGPortMap[8];

        // IP Address
        ITextVectorProperty IpTP;
        IText IpT[1];

        /////////////////////////////////////////////////////////////////////////////
        /// Cooler Properties
        /////////////////////////////////////////////////////////////////////////////
        ISwitch FanStateS[2];
        ISwitchVectorProperty FanStateSP;

        ISwitch CoolerS[2];
        ISwitchVectorProperty CoolerSP;

        INumber CoolerN[1];
        INumberVectorProperty CoolerNP;

        /////////////////////////////////////////////////////////////////////////////
        /// Adaptive Optics Properties
        /////////////////////////////////////////////////////////////////////////////
        INumberVectorProperty AONSNP;
        INumber AONSN[2];
        enum
        {
            AO_NORTH,
            AO_SOUTH,
        };

        INumberVectorProperty AOWENP;
        INumber AOWEN[2];
        enum
        {
            AO_EAST,
            AO_WEST,
        };

        ISwitch CenterS[1];
        ISwitchVectorProperty CenterSP;

        AOTipTiltParams m_AOParams;

        /////////////////////////////////////////////////////////////////////////////
        /// Options Properties
        /////////////////////////////////////////////////////////////////////////////
        ISwitch IgnoreErrorsS[1];
        ISwitchVectorProperty IgnoreErrorsSP;

        /////////////////////////////////////////////////////////////////////////////
        /// Filter Wheel Properties
        /////////////////////////////////////////////////////////////////////////////
        IText FilterProdcutT[2] {};
        ITextVectorProperty FilterProdcutTP;

        ISwitch FilterTypeS[MAX_CFW_TYPES];
        ISwitchVectorProperty FilterTypeSP;
        int SBIGFilterMap[MAX_CFW_TYPES];

        ISwitch FilterConnectionS[2];
        ISwitchVectorProperty FilterConnectionSP;

        /////////////////////////////////////////////////////////////////////////////
        /// Camera capabilities
        /////////////////////////////////////////////////////////////////////////////
        bool m_isColor { false };
        bool m_useExternalTrackingCCD { false };
        bool m_hasGuideHead { false };
        bool m_hasFilterWheel { false };
        bool m_hasAO { false };

        /////////////////////////////////////////////////////////////////////////////
        /// Threading Variables
        /////////////////////////////////////////////////////////////////////////////
        std::mutex sbigLock;

        /////////////////////////////////////////////////////////////////////////////
        /// Exposure Variables
        /////////////////////////////////////////////////////////////////////////////
        int m_TimerID { -1 };
        std::chrono::system_clock::time_point ExpStart, GuideExpStart;
        float ExposureRequest;
        float GuideExposureRequest;
        float TemperatureRequest;

        /////////////////////////////////////////////////////////////////////////////
        /// Guiding Variables
        /////////////////////////////////////////////////////////////////////////////
        ActivateRelayParams rp;
        int m_NSTimerID {-1}, m_WETimerID {-1};

        inline int GetFileDescriptor()
        {
            return (m_fd);
        }
        inline void SetFileDescriptor(int val = -1)
        {
            m_fd = val;
        }
        inline bool IsDeviceOpen()
        {
            return ((m_fd == -1) ? false : true);
        }
        inline CAMERA_TYPE GetCameraType()
        {
            return (m_camera_type);
        }
        inline void SetCameraType(CAMERA_TYPE val = NO_CAMERA)
        {
            m_camera_type = val;
        }
        inline int GetDriverHandle()
        {
            return (m_drv_handle);
        }
        inline void SetDriverHandle(int val = INVALID_HANDLE_VALUE)
        {
            m_drv_handle = val;
        }
        inline bool GetLinkStatus()
        {
            return (m_link_status);
        }
        inline void SetLinkStatus(bool val = false)
        {
            m_link_status = val;
        }
        int SetDeviceName(const char *);
        inline std::string GetStartExposureTimestamp()
        {
            return (m_start_exposure_timestamp);
        }
        inline void SetStartExposureTimestamp(const char *p)
        {
            m_start_exposure_timestamp = p;
        }

        /////////////////////////////////////////////////////////////////////////////
        /// Driver Communication Commands
        /////////////////////////////////////////////////////////////////////////////
        int GetCFWSelType();
        int OpenDevice(uint32_t devType);
        int CloseDevice();
        int GetDriverInfo(GetDriverInfoParams *, void *);
        int SetDriverHandle(SetDriverHandleParams *);
        int GetDriverHandle(GetDriverHandleResults *);

        /////////////////////////////////////////////////////////////////////////////
        /// Exposure Commands
        /////////////////////////////////////////////////////////////////////////////
        int StartExposure(StartExposureParams2 *);
        int EndExposure(EndExposureParams *);
        int StartReadout(StartReadoutParams *);
        int ReadoutLine(ReadoutLineParams *, unsigned short *results, bool subtract);
        int DumpLines(DumpLinesParams *);
        int EndReadout(EndReadoutParams *);

        /////////////////////////////////////////////////////////////////////////////
        /// Temperature Commands
        /////////////////////////////////////////////////////////////////////////////
        int SetTemperatureRegulation(SetTemperatureRegulationParams *);
        int SetTemperatureRegulation(double temp, bool enable = true);
        int QueryTemperatureStatus(QueryTemperatureStatusResults *);
        int QueryTemperatureStatus(bool &enabled, double &ccdTemp, double &setpointT, double &power);

        /////////////////////////////////////////////////////////////////////////////
        /// External Control Comands
        /////////////////////////////////////////////////////////////////////////////
        int ActivateRelay(ActivateRelayParams *);
        void NSGuideCallback();
        void WEGuideCallback();
        int PulseOut(PulseOutParams *);
        int TxSerialBytes(TXSerialBytesParams *, TXSerialBytesResults *);
        int GetSerialStatus(GetSerialStatusResults *);

        /////////////////////////////////////////////////////////////////////////////
        /// General Purpose Commands
        /////////////////////////////////////////////////////////////////////////////
        int EstablishLink();
        int GetCcdInfo(GetCCDInfoParams *, void *);
        int GetExtendedCCDInfo();
        int QueryCommandStatus(QueryCommandStatusParams *, QueryCommandStatusResults *);
        int MiscellaneousControl(MiscellaneousControlParams *);
        int ReadOffset(ReadOffsetParams *, ReadOffsetResults *);
        int GetLinkStatus(GetLinkStatusResults *);
        char *GetErrorString(int err);
        int SetDriverControl(SetDriverControlParams *);
        int GetDriverControl(GetDriverControlParams *, GetDriverControlResults *);
        int UsbAdControl(USBADControlParams *);
        int QueryUsb(QueryUSBResults *);
        int RwUsbI2c(RWUSBI2CParams *);
        int BitIo(BitIOParams *, BitIOResults *);

        /////////////////////////////////////////////////////////////////////////////
        /// Camera Functions
        /////////////////////////////////////////////////////////////////////////////
        int getCCDSizeInfo(int ccd, int rm, int &frmW, int &frmH, double &pixW, double &pixH);
        bool IsFanControlAvailable();
        bool updateFrameProperties(INDI::CCDChip *targetChip);
        int StartExposure(INDI::CCDChip *targetChip, double duration);
        int AbortExposure(INDI::CCDChip *targetChip);
        int GetSelectedCCDChip(int &ccd_request);
        int getBinningMode(INDI::CCDChip *targetChip, int &binning);
        int getFrameType(INDI::CCDChip *targetChip, INDI::CCDChip::CCD_FRAME *frameType);
        int getShutterMode(INDI::CCDChip *targetChip, int &shutter);
        int readoutCCD(unsigned short left, unsigned short top, unsigned short width, unsigned short height,
                       unsigned short *buffer, INDI::CCDChip *targetChip);

        /////////////////////////////////////////////////////////////////////////////
        /// Filter Wheel Functions
        /////////////////////////////////////////////////////////////////////////////
        int CFW(CFWParams *, CFWResults *);
        int CFWConnect();
        int CFWDisconnect();
        int CFWInit(CFWResults *);
        int CFWQuery(CFWResults *);
        int CFWGoto(CFWResults *, int position);
        int CFWGotoMonitor(CFWResults *);

        /////////////////////////////////////////////////////////////////////////////
        /// Adaptive Optics Functions
        /////////////////////////////////////////////////////////////////////////////
        int AoTipTilt();
        int AoDelay(AODelayParams *);
        int AoCenter();
        // N.B. Not implemented in SBIGUDRV
        int AoSetFocus(AOSetFocusParams *aofc);

        /////////////////////////////////////////////////////////////////////////////
        /// Utility Functions
        /////////////////////////////////////////////////////////////////////////////
        bool grabImage(INDI::CCDChip *targetChip);
        bool setupParams();
        // SBIG's software interface to the Universal Driver Library function:
        int SBIGUnivDrvCommand(PAR_COMMAND, void *, void *);
        bool CheckLink();
        const char *GetCameraName();
        const char *GetCameraID();
        int getReadoutModes(INDI::CCDChip *targetChip, int &numModes, int &maxBinX, int &maxBinY);

        friend void ::ISGetProperties(const char *dev);
        friend void ::ISSnoopDevice(XMLEle *root);
        friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
        friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
        friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
        friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                                char *formats[], char *names[], int n);
};
