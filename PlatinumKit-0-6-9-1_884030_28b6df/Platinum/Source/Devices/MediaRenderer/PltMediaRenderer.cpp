/*****************************************************************
|
|   Platinum - AV Media Renderer Device
|
| Copyright (c) 2004-2010, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| licensing@plutinosoft.com
|
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc.,
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltMediaRenderer.h"

//cloud-aa
#include "PltMicroMediaController.h"
#include "PltFileMediaServer.h"
#include "PltXbox360.h"
#include "PltVersion.h"

/* cloud-aa*/
//#define HAS_RENDERER 1
#define HAS_SERVER 1
//#define SIMULATE_XBOX_360 1
//#define SIMULATE_PS3 1
//#define BROADCAST_EXTRA 1

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "PltService.h"

NPT_SET_LOCAL_LOGGER("platinum.media.renderer")

/*----------------------------------------------------------------------
|   external references
+---------------------------------------------------------------------*/
extern NPT_UInt8 RDR_ConnectionManagerSCPD[];
extern NPT_UInt8 RDR_AVTransportSCPD[];
extern NPT_UInt8 RDR_RenderingControlSCPD[];

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::PLT_MediaRenderer
+---------------------------------------------------------------------*/
PLT_MediaRenderer::PLT_MediaRenderer(const char*  friendly_name,
                                     bool         show_ip     /* = false */,
                                     const char*  uuid        /* = NULL */,
                                     unsigned int port        /* = 0 */,
                                     bool         port_rebind /* = false */) :
    PLT_DeviceHost("/",
                   uuid,
                   "urn:schemas-upnp-org:device:MediaRenderer:1",
                   friendly_name,
                   show_ip,
                   port,
                   port_rebind),
    m_Delegate(NULL)
{
    m_ModelDescription = "Plutinosoft AV Media Renderer Device";
    m_ModelName        = "AV Renderer Device";
    m_ModelURL         = "http://www.plutinosoft.com/platinum";
    m_DlnaDoc          = "DMR-1.50";
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::~PLT_MediaRenderer
+---------------------------------------------------------------------*/
PLT_MediaRenderer::~PLT_MediaRenderer()
{
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::SetupServices
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::SetupServices()
{
    PLT_Service* service;

    {
        /* AVTransport */
        service = new PLT_Service(
            this,
            "urn:schemas-upnp-org:service:AVTransport:1",
            "urn:upnp-org:serviceId:AVTransport",
            "AVTransport",
            "urn:schemas-upnp-org:metadata-1-0/AVT/");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) RDR_AVTransportSCPD));
        NPT_CHECK_FATAL(AddService(service));

        service->SetStateVariableRate("LastChange", NPT_TimeInterval(0.2f));
        service->SetStateVariable("A_ARG_TYPE_InstanceID", "0");

        // GetCurrentTransportActions
        service->SetStateVariable("CurrentTransportActions", "Play,Pause,Stop,Seek,Next,Previous");

        // GetDeviceCapabilities
        service->SetStateVariable("PossiblePlaybackStorageMedia", "NONE,NETWORK,HDD,CD-DA,UNKNOWN");
        service->SetStateVariable("PossibleRecordStorageMedia", "NOT_IMPLEMENTED");
        service->SetStateVariable("PossibleRecordQualityModes", "NOT_IMPLEMENTED");

        // GetMediaInfo
        service->SetStateVariable("NumberOfTracks", "0");
        service->SetStateVariable("CurrentMediaDuration", "00:00:00");
        service->SetStateVariable("AVTransportURI", "");
        service->SetStateVariable("AVTransportURIMetadata", "");;
        service->SetStateVariable("NextAVTransportURI", "NOT_IMPLEMENTED");
        service->SetStateVariable("NextAVTransportURIMetadata", "NOT_IMPLEMENTED");
        service->SetStateVariable("PlaybackStorageMedium", "NONE");
        service->SetStateVariable("RecordStorageMedium", "NOT_IMPLEMENTED");
		service->SetStateVariable("RecordMediumWriteStatus", "NOT_IMPLEMENTED");

        // GetPositionInfo
        service->SetStateVariable("CurrentTrack", "0");
        service->SetStateVariable("CurrentTrackDuration", "00:00:00");
        service->SetStateVariable("CurrentTrackMetadata", "");
        service->SetStateVariable("CurrentTrackURI", "");
        service->SetStateVariable("RelativeTimePosition", "00:00:00");
        service->SetStateVariable("AbsoluteTimePosition", "00:00:00");
        service->SetStateVariable("RelativeCounterPosition", "2147483647"); // means NOT_IMPLEMENTED
        service->SetStateVariable("AbsoluteCounterPosition", "2147483647"); // means NOT_IMPLEMENTED

        // disable indirect eventing for certain state variables
        PLT_StateVariable* var;
        var = service->FindStateVariable("RelativeTimePosition");
        if (var) var->DisableIndirectEventing();
        var = service->FindStateVariable("AbsoluteTimePosition");
        if (var) var->DisableIndirectEventing();
        var = service->FindStateVariable("RelativeCounterPosition");
        if (var) var->DisableIndirectEventing();
        var = service->FindStateVariable("AbsoluteCounterPosition");
        if (var) var->DisableIndirectEventing();

        // GetTransportInfo
        service->SetStateVariable("TransportState", "NO_MEDIA_PRESENT");
        service->SetStateVariable("TransportStatus", "OK");
        service->SetStateVariable("TransportPlaySpeed", "1");

        // GetTransportSettings
        service->SetStateVariable("CurrentPlayMode", "NORMAL");
        service->SetStateVariable("CurrentRecordQualityMode", "NOT_IMPLEMENTED");
    }

    {
        /* ConnectionManager */
        service = new PLT_Service(
            this,
            "urn:schemas-upnp-org:service:ConnectionManager:1",
            "urn:upnp-org:serviceId:ConnectionManager",
            "ConnectionManager");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) RDR_ConnectionManagerSCPD));
        NPT_CHECK_FATAL(AddService(service));

        service->SetStateVariable("CurrentConnectionIDs", "0");

        // put all supported mime types here instead
        //service->SetStateVariable("SinkProtocolInfo", "http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_PRO,http-get:*:video/x-ms-asf:DLNA.ORG_PN=MPEG4_P2_ASF_SP_G726,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_FULL,http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_MED,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_BASE,http-get:*:audio/L16;rate=44100;channels=1:DLNA.ORG_PN=LPCM,http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_PAL,http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_NTSC,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVHIGH_PRO,http-get:*:audio/L16;rate=44100;channels=2:DLNA.ORG_PN=LPCM,http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM,http-get:*:video/x-ms-asf:DLNA.ORG_PN=VC1_ASF_AP_L1_WMA,http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMDRM_WMABASE,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVHIGH_FULL,http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMAFULL,http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMABASE,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPLL_BASE,http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_NTSC_XAC3,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMDRM_WMVSPLL_BASE,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPML_BASE,http-get:*:video/x-ms-asf:DLNA.ORG_PN=MPEG4_P2_ASF_ASP_L5_SO_G726,http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_LRG,http-get:*:audio/mpeg:DLNA.ORG_PN=MP3,http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_PAL_XAC3,http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMAPRO,http-get:*:video/mpeg:DLNA.ORG_PN=MPEG1,http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_TN,http-get:*:video/x-ms-asf:DLNA.ORG_PN=MPEG4_P2_ASF_ASP_L4_SO_G726,http-get:*:audio/L16;rate=48000;channels=2:DLNA.ORG_PN=LPCM,http-get:*:audio/mpeg:DLNA.ORG_PN=MP3X,http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPML_MP3,http-get:*:video/x-ms-wmv:*");
         service->SetStateVariable("SinkProtocolInfo"
        ,"http-get:*:*:*"
        ",xbmc-get:*:*:*"
        ",http-get:*:audio/mpegurl:*"
        ",http-get:*:audio/mpeg:*"
        ",http-get:*:audio/mpeg3:*"
        ",http-get:*:audio/mp3:*"
        ",http-get:*:audio/mp4:*"
        ",http-get:*:audio/basic:*"
        ",http-get:*:audio/midi:*"
        ",http-get:*:audio/ulaw:*"
        ",http-get:*:audio/ogg:*"
        ",http-get:*:audio/DVI4:*"
        ",http-get:*:audio/G722:*"
        ",http-get:*:audio/G723:*"
        ",http-get:*:audio/G726-16:*"
        ",http-get:*:audio/G726-24:*"
        ",http-get:*:audio/G726-32:*"
        ",http-get:*:audio/G726-40:*"
        ",http-get:*:audio/G728:*"
        ",http-get:*:audio/G729:*"
        ",http-get:*:audio/G729D:*"
        ",http-get:*:audio/G729E:*"
        ",http-get:*:audio/GSM:*"
        ",http-get:*:audio/GSM-EFR:*"
        ",http-get:*:audio/L8:*"
        ",http-get:*:audio/L16:*"
        ",http-get:*:audio/LPC:*"
        ",http-get:*:audio/MPA:*"
        ",http-get:*:audio/PCMA:*"
        ",http-get:*:audio/PCMU:*"
        ",http-get:*:audio/QCELP:*"
        ",http-get:*:audio/RED:*"
        ",http-get:*:audio/VDVI:*"
        ",http-get:*:audio/ac3:*"
        ",http-get:*:audio/vorbis:*"
        ",http-get:*:audio/speex:*"
        ",http-get:*:audio/x-aiff:*"
        ",http-get:*:audio/x-pn-realaudio:*"
        ",http-get:*:audio/x-realaudio:*"
        ",http-get:*:audio/x-wav:*"
        ",http-get:*:audio/x-ms-wma:*"
        ",http-get:*:audio/x-mpegurl:*"
        ",http-get:*:application/x-shockwave-flash:*"
        ",http-get:*:application/ogg:*"
        ",http-get:*:application/sdp:*"
        ",http-get:*:image/gif:*"
        ",http-get:*:image/jpeg:*"
        ",http-get:*:image/ief:*"
        ",http-get:*:image/png:*"
        ",http-get:*:image/tiff:*"
        ",http-get:*:video/avi:*"
        ",http-get:*:video/mpeg:*"
        ",http-get:*:video/fli:*"
        ",http-get:*:video/flv:*"
        ",http-get:*:video/quicktime:*"
        ",http-get:*:video/vnd.vivo:*"
        ",http-get:*:video/vc1:*"
        ",http-get:*:video/ogg:*"
        ",http-get:*:video/mp4:*"
        ",http-get:*:video/BT656:*"
        ",http-get:*:video/CelB:*"
        ",http-get:*:video/JPEG:*"
        ",http-get:*:video/H261:*"
        ",http-get:*:video/H263:*"
        ",http-get:*:video/H263-1998:*"
        ",http-get:*:video/H263-2000:*"
        ",http-get:*:video/MPV:*"
        ",http-get:*:video/MP2T:*"
        ",http-get:*:video/MP1S:*"
        ",http-get:*:video/MP2P:*"
        ",http-get:*:video/BMPEG:*"
        ",http-get:*:video/x-ms-wmv:*"
        ",http-get:*:video/x-ms-avi:*"
        ",http-get:*:video/x-flv:*"
        ",http-get:*:video/x-fli:*"
        ",http-get:*:video/x-ms-asf:*"
        ",http-get:*:video/x-ms-asx:*"
        ",http-get:*:video/x-ms-wmx:*"
        ",http-get:*:video/x-ms-wvx:*"
        ",http-get:*:video/x-msvideo:*"
        );
        service->SetStateVariable("SourceProtocolInfo", "");
    }

    {
        /* RenderingControl */
        service = new PLT_Service(
            this,
            "urn:schemas-upnp-org:service:RenderingControl:1",
            "urn:upnp-org:serviceId:RenderingControl",
            "RenderingControl",
            "urn:schemas-upnp-org:metadata-1-0/RCS/");
        NPT_CHECK_FATAL(service->SetSCPDXML((const char*) RDR_RenderingControlSCPD));
        NPT_CHECK_FATAL(AddService(service));

        service->SetStateVariableRate("LastChange", NPT_TimeInterval(0.2f));

        service->SetStateVariable("Mute", "0");
        service->SetStateVariableExtraAttribute("Mute", "Channel", "Master");
        service->SetStateVariable("Volume", "100");
        service->SetStateVariableExtraAttribute("Volume", "Channel", "Master");
        service->SetStateVariable("VolumeDB", "0");
        service->SetStateVariableExtraAttribute("VolumeDB", "Channel", "Master");

        service->SetStateVariable("PresetNameList", "FactoryDefaults");
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnAction
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnAction(PLT_ActionReference&          action,
                            const PLT_HttpRequestContext& context)
{
    NPT_COMPILER_UNUSED(context);

    /* parse the action name */
    NPT_String name = action->GetActionDesc().GetName();

    // since all actions take an instance ID and we only support 1 instance
    // verify that the Instance ID is 0 and return an error here now if not
    NPT_String serviceType = action->GetActionDesc().GetService()->GetServiceType();
    if (serviceType.Compare("urn:schemas-upnp-org:service:AVTransport:1", true) == 0) {
        if (NPT_FAILED(action->VerifyArgumentValue("InstanceID", "0"))) {
            action->SetError(718, "Not valid InstanceID");
            return NPT_FAILURE;
        }
    }
	serviceType = action->GetActionDesc().GetService()->GetServiceType();
	if (serviceType.Compare("urn:schemas-upnp-org:service:RenderingControl:1", true) == 0) {
		if (NPT_FAILED(action->VerifyArgumentValue("InstanceID", "0"))) {
			action->SetError(702, "Not valid InstanceID");
			return NPT_FAILURE;
		}
	}

	/* Is it a ConnectionManager Service Action ? */
	if (name.Compare("GetCurrentConnectionInfo", true) == 0) {
		return OnGetCurrentConnectionInfo(action);
	}

	/* Is it a AVTransport Service Action ? */
    if (name.Compare("Next", true) == 0) {
        return OnNext(action);
    }
    if (name.Compare("Pause", true) == 0) {
        return OnPause(action);
    }
    if (name.Compare("Play", true) == 0) {
        return OnPlay(action);
    }
    if (name.Compare("Previous", true) == 0) {
        return OnPrevious(action);
    }
    if (name.Compare("Seek", true) == 0) {
        return OnSeek(action);
    }
    if (name.Compare("Stop", true) == 0) {
        return OnStop(action);
    }
    if (name.Compare("SetAVTransportURI", true) == 0) {
        return OnSetAVTransportURI(action);
    }
    if (name.Compare("SetPlayMode", true) == 0) {
        return OnSetPlayMode(action);
    }

    /* Is it a RendererControl Service Action ? */
    if (name.Compare("SetVolume", true) == 0) {
          return OnSetVolume(action);
    }
	if (name.Compare("SetVolumeDB", true) == 0) {
		return OnSetVolumeDB(action);
    }
	if (name.Compare("GetVolumeDBRange", true) == 0) {
		return OnGetVolumeDBRange(action);

	}
    if (name.Compare("SetMute", true) == 0) {
          return OnSetMute(action);
    }

    // other actions rely on state variables
    NPT_CHECK_LABEL_WARNING(action->SetArgumentsOutFromStateVariable(), failure);
    return NPT_SUCCESS;

failure:
    action->SetError(401,"No Such Action.");
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnGetCurrentConnectionInfo
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnGetCurrentConnectionInfo(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnGetCurrentConnectionInfo(action);
    }

    if (NPT_FAILED(action->VerifyArgumentValue("ConnectionID", "0"))) {
        action->SetError(706,"No Such Connection.");
        return NPT_FAILURE;
    }

    if (NPT_FAILED(action->SetArgumentValue("RcsID", "0"))){
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("AVTransportID", "0"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentOutFromStateVariable("ProtocolInfo"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("PeerConnectionManager", "/"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("PeerConnectionID", "-1"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("Direction", "Input"))) {
        return NPT_FAILURE;
    }
    if (NPT_FAILED(action->SetArgumentValue("Status", "Unknown"))) {
        return NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnNext
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnNext(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnNext(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnPause
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnPause(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnPause(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnPlay
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnPlay(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnPlay(action);
    }

    NPT_String uri, meta;
    PLT_Service* service;

	//NPT_Cardinal max_items = 4;
	//PLT_ThreadTask *task;

    // look for value set previously by SetAVTransportURI
    NPT_CHECK_SEVERE(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", service));
    NPT_CHECK_SEVERE(service->GetStateVariableValue("AVTransportURI", uri));
    NPT_CHECK_SEVERE(service->GetStateVariableValue("AVTransportURIMetaData", meta));


    {
    	NPT_AutoLock lock(m_state);
    	service->SetStateVariable("TransportState", "TRANSITIONING");
    	service->SetStateVariable("TransportStatus", "OK");
    }

	// transcoder process takes over here. it shall put the output to the media server's
	// media folder which can then be served by the media server when the media renderer
	// requests it via a media controller that is instantiated here to connect(control)
	// the transport of av between the renderer and the transcoder.

	//PLT_TaskManager manager(max_items);
	//manager.StartTask(task,0,true) ;
	int child_pid;

    char cmd[200];
	char vcodec[] = "copy";
	char acodec[] = "copy";
	char fext[] = "flv"; //asf
	char fname[] = "/home/ted/Music/transcoded.flv";
	//ffmpeg -i bbb.m4v -vf "movie=aa-logo.png [wm];[in][wm] overlay=10:H-h-10 [out]" -f m4v output.m4v
    //char* arg_list[] = {"ffmpeg",//"-v",//"0","-i",uri,"-vcodec",vcodec,"-acodec",acodec,//"-b",//"600k",//"-s",//"hd720","-f",fext,"-y",fname,NULL};
	char* arg_list[] = {
	        "ffmpeg",
	        //"-v",
	        //"0",
	        "-i",
	        uri,
	        "-vcodec",
	        "libx264",
	        //"-coder",
	        //"0",
	        //"-bf",
	        //"0",
	        //"-flags2",
	        //"fast",
	        //"-wpredp",
	        //"0",
	        "-vpre",
	        "lossless_ultrafast",
	        "-vpre",
	        "baseline",
	        "-crf",
	        "25",
	        "-acodec",
	        "libmp3lame",
	        "-ar",
	        "44100",
	        "-deblockalpha",
	        "0",
	        "-deblockbeta",
	        "0",
	        "-r",
	        "25",
	        "-vb",
	        "2000000",
	        "-vf",
	        "movie=/home/ted/Logo/aa-logo.png[wm];[in][wm]overlay=10:H-h-10[out]",
	        "-f",
	        fext,
	        "-y",
	        fname,
	        NULL
	    };
	printf("\n\n****%s****\n\n",arg_list);


    //int n=sprintf (cmd, "ffmpeg.exe  -i %s -vcodec %s -acodec %s  -f %s -y %s", uri,vcodec,acodec,fext,fname);
	//int n=sprintf (cmd, "ffmpeg.exe  -i %s -i aa.png -i pv.png -filter_complex 'overlay=10:H-h-10,overlay=W-w-10:H-h-10' -vcodec %s -b:v 200k -acodec %s -f %s -y %s", uri,vcodec,acodec,fext,fname);

    /* Spawn a child process running the “ls” command.  Ignore the
    returned child process ID.  */

    /* Duplicate this process.  */
    child_pid = fork ();
    if (child_pid != 0){
        /* This is the parent process.  */
        // setup Neptune logging
    //NPT_LogManager::GetDefault().Configure("plist:.level=INFO;.handlers=ConsoleHandler;.ConsoleHandler.colors=off;.ConsoleHandler.filter=63");

    // Create upnp engine
    PLT_UPnP upnp;

#ifdef SIMULATE_XBOX_360
    // override default headers
    NPT_HttpClient::m_UserAgentHeader = "Xbox/2.0.8955.0 UPnP/1.0 Xbox/2.0.8955.0";
    NPT_HttpServer::m_ServerHeader    = "Xbox/2.0.8955.0 UPnP/1.0 Xbox/2.0.8955.0";
#endif

#ifdef SIMULATE_PS3
    // TODO: We need a way to add an extra header to all HTTP requests
    //X-AV-Client-Info: av=5.0; cn="Sony Computer Entertainment Inc."; mn="PLAYSTATION 3"; mv="1.0";
#endif

    // Create control point
    PLT_CtrlPointReference ctrlPoint(new PLT_CtrlPoint());

    // Create controller
    PLT_MicroMediaController controller(ctrlPoint);

#ifdef HAS_SERVER
    // create device
    PLT_DeviceHostReference server(
        new PLT_FileMediaServer("/home/ted/Music/",
                                "Embedded Media Server"));

    server->m_ModelDescription = "Embedded File Media Server";
    server->m_ModelURL = "http://www.plutinosoft.com/";
    server->m_ModelNumber = "1.0";
    server->m_ModelName = "Platinum File Media Server";
    server->m_Manufacturer = "Plutinosoft";
    server->m_ManufacturerURL = "http://www.plutinosoft.com/";

    // add device
    upnp.AddDevice(server);
	NPT_String uuid = server->GetUUID();
	//printf("****** surver name is:  %s\n", uuid);

    // remove device uuid from ctrlpoint
    //ctrlPoint->IgnoreUUID(server->GetUUID());
#endif


#ifdef HAS_RENDERER
	//create a device
	PLT_DeviceHostReference renderer(
        new PLT_MediaRenderer("Platinum Media Based \C5bo Test Transcoder",
                              false,
                              "e6572b54-f3c7-2d91-2fb5-b757f2537e21"));
    upnp.AddDevice(renderer);

#endif


    // add control point to upnp engine and start it
    upnp.AddCtrlPoint(ctrlPoint);
    upnp.Start();

#ifdef BROADCAST_EXTRA
    // tell control point to perform extra broadcast discover every 6 secs
    // in case our device doesn't support multicast
    ctrlPoint->Discover(NPT_HttpUrl("255.255.255.255", 1900, "*"), "upnp:rootdevice", 1, NPT_TimeInterval(6.0));
    ctrlPoint->Discover(NPT_HttpUrl("239.255.255.250", 1900, "*"), "upnp:rootdevice", 1, NPT_TimeInterval(6.0));
#endif

#ifdef SIMULATE_XBOX_360
    // create device
    PLT_DeviceHostReference xbox(new PLT_Xbox360("30848576-1775-2000-0000-00125a8fefad"));
    xbox->SetByeByeFirst(false);
    xbox->m_SerialNumber = "308485761776";

    // add device
    upnp.AddDevice(xbox);
    ctrlPoint->IgnoreUUID(xbox->GetUUID());

    // xbox issues a search for the content directory service
    // 10 secs after announcing itself to make sure
    // it got detected and inspected first

    ctrlPoint->Search(
        NPT_HttpUrl("239.255.255.250", 1900, "*"),
        "urn:schemas-microsoft-com:service:MSContentDirectory:1", 2, 10000, NPT_TimeInterval(10, 0));
    ctrlPoint->Search(
        NPT_HttpUrl("239.255.255.250", 1900, "*"),
        "urn:schemas-upnp-org:service:ContentDirectory:1", 2, 10000, NPT_TimeInterval(10, 0));

#endif

    // start to process commands
    //controller.ProcessCommandLoop();
	//upnp.m_Devices

    // stop everything
    //upnp.Stop();



	//controller.DoEmbeddedServerCheck();



	const char embeddedServerName[]="Embedded Media Server";
	//PC as a renderer
	//const char mediaRendererName[]="XBMC: Media Renderer (192.168.11.3)";
	//mina laptop
	//const char mediaRendererName[]="XBMC (ted-HP-2230s)";
	//Raspbmc
	const char mediaRendererName[]="XBMC (raspbmc)";



	controller.SetEmbeddedServer(embeddedServerName);
	controller.SetMediaRenderer(mediaRendererName);


	sleep(25);

	controller.HandleCmd_open();


	sleep(1);
	controller.HandleCmd_play();

	 NPT_AutoLock lock(m_state);
	 service->SetStateVariable("TransportState", "PLAYING");
	 service->SetStateVariable("TransportStatus", "OK");
	 service->SetStateVariable("AVTransportURI", uri);
	 service->SetStateVariable("AVTransportURIMetaData", meta);



	//wait till play finished
	char buf[256];
	while (gets(buf)) {
        if (*buf == 'q'){
            kill( child_pid, SIGKILL );
            break;
        }
	}



	// Wait until child process exits.
    //int child_status;
    wait();


    upnp.Stop();

    return NPT_SUCCESS;

    }else {
        /* Now execute PROGRAM, searching for it in the path.  */
        execvp ("ffmpeg", arg_list);
        /* The execvp function returns only if an error occurs.  */
        fprintf (stderr, "an error occurred in execvp\n");

        abort ();
   }

}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnPrevious
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnPrevious(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnPrevious(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSeek
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSeek(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnSeek(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnStop
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnStop(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnStop(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetAVTransportURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetAVTransportURI(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnSetAVTransportURI(action);
    }

    // default implementation is using state variable
    NPT_String uri;
    NPT_CHECK_WARNING(action->GetArgumentValue("CurrentURI", uri));

    NPT_String metadata;
    NPT_CHECK_WARNING(action->GetArgumentValue("CurrentURIMetaData", metadata));

    PLT_Service* serviceAVT;
    NPT_CHECK_WARNING(FindServiceByType("urn:schemas-upnp-org:service:AVTransport:1", serviceAVT));

    // update service state variables
    serviceAVT->SetStateVariable("AVTransportURI", uri);
    serviceAVT->SetStateVariable("AVTransportURIMetaData", metadata);

    serviceAVT->SetStateVariable("TransportState", "STOPPED");
    serviceAVT->SetStateVariable("TransportStatus", "OK");
    serviceAVT->SetStateVariable("TransportPlaySpeed", "1");


    NPT_CHECK_SEVERE(action->SetArgumentsOutFromStateVariable());


    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetPlayMode
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetPlayMode(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnSetPlayMode(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetVolume
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetVolume(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnSetVolume(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetVolumeDB
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetVolumeDB(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnSetVolumeDB(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnGetVolumeDBRange
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnGetVolumeDBRange(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnGetVolumeDBRange(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   PLT_MediaRenderer::OnSetMute
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaRenderer::OnSetMute(PLT_ActionReference& action)
{
    if (m_Delegate) {
        return m_Delegate->OnSetMute(action);
    }
    return NPT_ERROR_NOT_IMPLEMENTED;
}
