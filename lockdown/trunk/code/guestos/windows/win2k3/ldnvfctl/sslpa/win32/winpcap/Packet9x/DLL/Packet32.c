/*
 * Copyright (c) 1999, 2000
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <packet32.h>
#include <windows.h>
#include <windowsx.h>
#include <ntddndis.h>
#include <sys/timeb.h>
#ifdef __MINGW32__
#ifdef DBG
#include <assert.h>
#define _ASSERTE assert
#else
#define _ASSERTE
#endif /* DBG */
#else
#include <crtdbg.h>
#endif /* __MINGW32__ */


#ifdef __cplusplus
extern "C"
{		
#endif				

TCHAR szWindowTitle[] = TEXT ("PACKET.DLL");
LPADAPTER lpTheAdapter = NULL;

#if _DBG
#define ODS(_x) OutputDebugString(TEXT(_x))
#define ODSEx(_x, _y)
#else
#ifdef _DEBUG_TO_FILE
#include <stdio.h>
// Macro to print a debug string. The behavior differs depending on the debug level
#define ODS(_x) { \
	FILE *f; \
	f = fopen("winpcap_debug.txt", "a"); \
	fprintf(f, "%s", _x); \
	fclose(f); \
}
// Macro to print debug data with the printf convention. The behavior differs depending on */
#define ODSEx(_x, _y) { \
	FILE *f; \
	f = fopen("winpcap_debug.txt", "a"); \
	fprintf(f, _x, _y); \
	fclose(f); \
}

#else
#define ODS(_x)		
#define ODSEx(_x, _y)
#endif
#endif

typedef DWORD(CALLBACK* OPENVXDHANDLE)(HANDLE);

BOOLEAN StartPacketDriver (LPTSTR ServiceName);
BOOLEAN StopPacketDriver (void);
BOOLEAN PacketSetMaxLookaheadsize (LPADAPTER AdapterObject);

char PacketLibraryVersion[] = "2.3"; 

//---------------------------------------------------------------------------

PCHAR PacketGetVersion(){
	return PacketLibraryVersion;
}

//---------------------------------------------------------------------------
BOOL APIENTRY DllMain (HANDLE hModule,
					DWORD dwReason,
					LPVOID lpReserved)
{
    BOOL Status;

	ODS("\n************Packet32: DllMain************\n");
    switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			Status = StartPacketDriver (TEXT ("PACKET"));
			break;
		case DLL_PROCESS_DETACH:
			Status = StopPacketDriver ();
			break;
		default:
			Status = TRUE;
			break;
	}
	return Status;
}

//---------------------------------------------------------------------------
BOOL PacketDeviceIoControl (LPADAPTER lpAdapterObject,
			      LPPACKET lpPacket,
			      ULONG ulIoctl,
			      BOOLEAN bSync)
{
	BOOLEAN Result;
	DWORD Error;

	ODS ("Packet32: PacketDeviceIoControl\n");
	_ASSERTE (lpAdapterObject != NULL);
	_ASSERTE (lpPacket != NULL);
	lpPacket->OverLapped.Offset = 0;
	lpPacket->OverLapped.OffsetHigh = 0;
	lpPacket->ulBytesReceived		= 0;
	if (!ResetEvent (lpPacket->OverLapped.hEvent))
	{
		lpPacket->bIoComplete = FALSE;
		return FALSE;
	}

    Result = DeviceIoControl (lpAdapterObject->hFile,
				ulIoctl,
				lpPacket->Buffer,
				lpPacket->Length,
				lpPacket->Buffer,
				lpPacket->Length,
				&(lpPacket->ulBytesReceived), 
				&(lpPacket->OverLapped));
	Error=GetLastError () ;
    
	if (!Result && bSync)
	{
		if (Error == ERROR_IO_PENDING)
		{
			Result = GetOverlappedResult (lpAdapterObject->hFile,
					&(lpPacket->OverLapped),
					&(lpPacket->ulBytesReceived), 
					TRUE);
		}
		else
			ODS ("Packet32: unsupported API call return error!\n");
	}

	lpPacket->bIoComplete = Result;

	return Result;
}


//---------------------------------------------------------------------------

LPADAPTER PacketOpenAdapter (LPTSTR AdapterName)
{
    LPPACKET	lpSupport;
	LPADAPTER	nAdapter;
	DWORD		error;
	BOOL		res;
	OPENVXDHANDLE OpenVxDHandle;
	DWORD		KernEvent;
	UINT		BytesReturned;
	struct _timeb time;

    ODSEx ("Packet32: PacketOpenAdapter, opening %s\n", AdapterName);
	
	nAdapter = (LPADAPTER) GlobalAllocPtr (GMEM_MOVEABLE | GMEM_ZEROINIT,sizeof (ADAPTER));
	if (nAdapter == NULL)
	{
		error=GetLastError();
		ODS ("Packet32: PacketOpenAdapter GlobalAlloc Failed\n");
		ODSEx("Error=%d\n",error);
		//set the error to the one on which we failed
		SetLastError(error);
		return NULL;
	}
	wsprintf (nAdapter->SymbolicLink,
		TEXT ("\\\\.\\%s"),
		TEXT ("NPF.VXD"));
	nAdapter->hFile = CreateFile (nAdapter->SymbolicLink,
		GENERIC_WRITE | GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE, 
		0);

	if (nAdapter->hFile == INVALID_HANDLE_VALUE)
	{
		error=GetLastError();
		ODS ("Packet32: PacketOpenAdapter Could not open adapter, 1\n");
		ODSEx("Error=%d\n",error);
		GlobalFreePtr (nAdapter);
		//set the error to the one on which we failed
		SetLastError(error);
		return NULL;
	}
	
	lpSupport=PacketAllocatePacket();
	
	PacketInitPacket(lpSupport,AdapterName,strlen(AdapterName));
	if (nAdapter && (nAdapter->hFile != INVALID_HANDLE_VALUE))
	{
		res=PacketDeviceIoControl(nAdapter,
			lpSupport,
			(ULONG) IOCTL_OPEN,
			TRUE);
		if (res==FALSE || ((char*)lpSupport->Buffer)[0]=='\0'){
			SetLastError(ERROR_FILE_NOT_FOUND);
			goto err;
		}
		PacketFreePacket(lpSupport);

		// Set the time zone
		_ftime(&time);
		if(DeviceIoControl(nAdapter->hFile,pBIOCSTIMEZONE,&time.timezone,2,NULL,0,&BytesReturned,NULL)==FALSE){
			error=GetLastError();
			ODS ("Packet32: PacketOpenAdapter Could not open adapter, 2\n");
			ODSEx("Error=%d\n",error);
			GlobalFreePtr (nAdapter);
			//set the error to the one on which we failed
			SetLastError(error);
			return NULL;	
		}
		
		// create the read event
		nAdapter->ReadEvent=CreateEvent(NULL, TRUE, FALSE, NULL);
		// obtain the pointer of OpenVxDHandle in KERNEL32.DLL.
		// It is not possible to reference statically this function, because it is present only in Win9x
		OpenVxDHandle = (OPENVXDHANDLE) GetProcAddress(GetModuleHandle("KERNEL32"),"OpenVxDHandle");
		// map the event to kernel mode
		KernEvent=(DWORD)(OpenVxDHandle)(nAdapter->ReadEvent);
		// pass the event to the driver
		if(DeviceIoControl(nAdapter->hFile,pBIOCEVNAME,&KernEvent,4,NULL,0,&BytesReturned,NULL)==FALSE){
			error=GetLastError();
			ODS("Packet32: PacketOpenAdapter Could not open adapter, 3\n");
			ODSEx("Error=%d\n",error);
			GlobalFreePtr (nAdapter);
			//set the error to the one on which we failed
			SetLastError(error);
			return NULL;	
		}

		//set the maximum lookhahead size allowable with this NIC driver
		PacketSetMaxLookaheadsize(nAdapter);

		//set the number of times a single write will be repeated
		PacketSetNumWrites(nAdapter,1);
		
		return nAdapter;
	}
err:
	error=GetLastError();
	ODS ("Packet32: PacketOpenAdapter Could not open adapter, 4\n");
	//set the error to the one on which we failed
	ODSEx("Error=%d\n",error);
	SetLastError(error);
    return NULL;
}

//---------------------------------------------------------------------------
/* Function to set the working mode of the driver
mode	working mode
*/

BOOLEAN PacketSetMode(LPADAPTER AdapterObject,int mode)
{
	int BytesReturned;
		
    return DeviceIoControl(AdapterObject->hFile,pBIOCSMODE,&mode,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------
/* Function to get the the read event*/

HANDLE PacketGetReadEvent(LPADAPTER AdapterObject)
{
    return AdapterObject->ReadEvent;
}

//---------------------------------------------------------------------------
/* Function to set the the number of times a write will be repeated*/

BOOLEAN PacketSetNumWrites(LPADAPTER AdapterObject,int nwrites)
{
	AdapterObject->NumWrites=nwrites;
	return TRUE;
}

//---------------------------------------------------------------------------
/* This function is used to set the read timeout
timeout		value of timeout(milliseconds). 0 means infinite.
*/

BOOLEAN PacketSetReadTimeout(LPADAPTER AdapterObject,int timeout)
{
	int BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSRTIMEOUT,&timeout,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------
/* This function allows to set the dimension of the packet buffer in the driver
parameters:
dim		dimension of the buffer (kilobytes)
*/

BOOLEAN PacketSetBuff(LPADAPTER AdapterObject,int dim)
{
	int BytesReturned;
    return DeviceIoControl(AdapterObject->hFile,pBIOCSETBUFFERSIZE,&dim,4,NULL,0,&BytesReturned,NULL);
}

//---------------------------------------------------------------------------
/* Function to set the minimum amount of bytes that will be copied by the driver*/

BOOLEAN PacketSetMinToCopy(LPADAPTER AdapterObject,int nbytes)
{
	// not yet implemented in Windows 9x
    return TRUE;
}

//---------------------------------------------------------------------------
/* Function to set a bpf filter in the driver
parameters:
fp		the pointer to the beginning of the filtering program
*/

BOOLEAN PacketSetBpf(LPADAPTER AdapterObject,struct bpf_program *fp)
{
	int BytesReturned;

    return DeviceIoControl(AdapterObject->hFile,pBIOCSETF,(char*)fp->bf_insns,fp->bf_len*sizeof(struct bpf_insn),NULL,0,&BytesReturned,NULL);
}



//---------------------------------------------------------------------------

BOOLEAN PacketGetStats(LPADAPTER AdapterObject,struct bpf_stat *s)
{
	int BytesReturned;

    return DeviceIoControl(AdapterObject->hFile,pBIOCGSTATS,NULL,0,s,sizeof(struct bpf_stat),&BytesReturned,NULL);
}


//---------------------------------------------------------------------------
VOID PacketCloseAdapter (LPADAPTER lpAdapter)
{

	ODS ("Packet32: PacketCloseAdapter\n");

	// close the capture handle
	CloseHandle (lpAdapter->hFile);

	// close the read event
	CloseHandle (lpAdapter->ReadEvent);

	GlobalFreePtr (lpAdapter);
	lpAdapter = NULL;

}

//---------------------------------------------------------------------------
LPPACKET PacketAllocatePacket (void)
{
	LPPACKET lpPacket;

    lpPacket = (LPPACKET) GlobalAllocPtr (
					   GMEM_MOVEABLE | GMEM_ZEROINIT,
					   sizeof (PACKET));
    if (lpPacket == NULL)
	{
		ODS ("Packet32: PacketAllocatePacket: GlobalAlloc Failed\n");
		return NULL;
	}
    lpPacket->OverLapped.hEvent = CreateEvent (NULL,
						FALSE,
						FALSE,
						NULL);
    if (lpPacket->OverLapped.hEvent == NULL)
    {
		ODS ("Packet32: PacketAllocatePacket: CreateEvent Failed\n");
		GlobalFreePtr (lpPacket);
		return NULL;
    }
	lpPacket->Buffer=NULL;
	lpPacket->Length=0;
	lpPacket->ulBytesReceived	= 0;
	lpPacket->bIoComplete		= FALSE;

    return lpPacket;
}

//---------------------------------------------------------------------------
VOID PacketFreePacket (LPPACKET lpPacket)
{
	CloseHandle (lpPacket->OverLapped.hEvent);
    GlobalFreePtr (lpPacket);
}

//---------------------------------------------------------------------------
VOID PacketInitPacket (LPPACKET lpPacket,
				       PVOID Buffer,
				       UINT Length)
{
	lpPacket->Buffer = Buffer;
    lpPacket->Length = Length;
}

//---------------------------------------------------------------------------
BOOLEAN PacketSendPacket (LPADAPTER AdapterObject,
						LPPACKET lpPacket,
						BOOLEAN Sync)
{
	int i;
	for(i=0;i<AdapterObject->NumWrites;i++){
		if(PacketDeviceIoControl (AdapterObject,lpPacket,(ULONG) IOCTL_PROTOCOL_WRITE,Sync)==FALSE)
			return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
BOOLEAN PacketReceivePacket(LPADAPTER AdapterObject,
		       LPPACKET lpPacket,
		       BOOLEAN Sync)
{
	return PacketDeviceIoControl(AdapterObject,
								lpPacket,
								(ULONG) IOCTL_PROTOCOL_READ,
								Sync);

}


//---------------------------------------------------------------------------
BOOLEAN PacketWaitPacket (LPADAPTER AdapterObject,
		       LPPACKET lpPacket)
{
	lpPacket->bIoComplete =  GetOverlappedResult( AdapterObject->hFile,
													&lpPacket->OverLapped,
													&lpPacket->ulBytesReceived,
												    TRUE );

	return lpPacket->bIoComplete;

}

//---------------------------------------------------------------------------
BOOLEAN PacketResetAdapter (LPADAPTER AdapterObject)
{
    UINT BytesReturned;
    DeviceIoControl (
		      AdapterObject->hFile,
		      (DWORD) IOCTL_PROTOCOL_RESET,
		      NULL,
		      0,
		      NULL,
		      0,
		      &BytesReturned,
		      NULL);
    return TRUE;
}

//---------------------------------------------------------------------------
BOOLEAN PacketRequest (LPADAPTER AdapterObject,
		    BOOLEAN Set,
		    PPACKET_OID_DATA OidData)
{
    UINT BytesReturned;
    BOOLEAN Result;
	OVERLAPPED Overlap;

	ODS ("Packet32: PacketRequest\n");
	_ASSERTE (AdapterObject != NULL);
	_ASSERTE (OidData != NULL);
	_ASSERTE (OidData->Data != NULL);
	Overlap.Offset = 0;
	Overlap.OffsetHigh = 0;
    Overlap.hEvent = CreateEvent (NULL,
						FALSE,
						FALSE,
						NULL);
    if (Overlap.hEvent == NULL)
    {
		ODS ("Packet32: PacketRequestPacket: CreateEvent Failed\n");
		return FALSE;
    }
	if (!ResetEvent(Overlap.hEvent))
	{
		ODS ("Packet32: PacketRequestPacket: ResetEvent Failed\n");
		CloseHandle(Overlap.hEvent);
		return FALSE;
	}
    Result = DeviceIoControl (
			       AdapterObject->hFile,
	    (DWORD) Set ? pBIOCSETOID : pBIOCQUERYOID,
			       OidData,
			     sizeof (PACKET_OID_DATA) - 1 + OidData->Length,
			       OidData,
			     sizeof (PACKET_OID_DATA) - 1 + OidData->Length,
			       &BytesReturned,
			       &Overlap);
	if (!Result)
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			Result = GetOverlappedResult(AdapterObject->hFile,
										&Overlap,
										&BytesReturned,
										TRUE);
		}
		else
		{
			ODS("Packet32: Unssupported API call return error!\n");
		}
	}
	if (BytesReturned == 0)
	{
		// There was an ndis error
		ODS ("Packet32: Ndis returned error to OID\n");
		Result = FALSE;
	}

	CloseHandle(Overlap.hEvent);
	return Result;
}

//---------------------------------------------------------------------------
BOOLEAN PacketSetHwFilter (LPADAPTER AdapterObject,
				      ULONG Filter)
{
	BOOLEAN Status;
    ULONG IoCtlBufferLength = (sizeof (PACKET_OID_DATA) + sizeof (ULONG) - 1);
    PPACKET_OID_DATA OidData;

	ODS ("Packet32: PacketSetFilter\n");
	_ASSERTE (AdapterObject != NULL);
    OidData = GlobalAllocPtr (
			       GMEM_MOVEABLE | GMEM_ZEROINIT,
			       IoCtlBufferLength
      );
    if (OidData == NULL)
	{
		return FALSE;
	}
    OidData->Oid = OID_GEN_CURRENT_PACKET_FILTER;
    OidData->Length = sizeof (ULONG);
    *((PULONG) OidData->Data) = Filter;
    Status = PacketRequest (
			     AdapterObject,
			     TRUE,
			     OidData
      );
    GlobalFreePtr (OidData);
    return Status;
}
//---------------------------------------------------------------------------
BOOLEAN PacketGetNetType (LPADAPTER AdapterObject,NetType *type)
{
	BOOLEAN Status;
    ULONG IoCtlBufferLength = (sizeof (PACKET_OID_DATA) + sizeof (ULONG) - 1);
    PPACKET_OID_DATA OidData;

	ODS ("Packet32: PacketSetFilter\n");
	_ASSERTE (AdapterObject != NULL);
    OidData = GlobalAllocPtr (
			       GMEM_MOVEABLE | GMEM_ZEROINIT,
			       IoCtlBufferLength
      );
    if (OidData == NULL)
	{
		return FALSE;
	}
	//get the link-layer type
    OidData->Oid = OID_GEN_MEDIA_IN_USE;
    OidData->Length = sizeof (ULONG);
    Status = PacketRequest(AdapterObject,FALSE,OidData);
	if(Status==FALSE)return FALSE;

    type->LinkType=*((UINT*)OidData->Data);

	//get the link-layer speed
    OidData->Oid = OID_GEN_LINK_SPEED;
    OidData->Length = sizeof (ULONG);
    Status = PacketRequest(AdapterObject,FALSE,OidData);
	type->LinkSpeed=*((UINT*)OidData->Data)*100;
    GlobalFreePtr (OidData);
    return Status;
}

//---------------------------------------------------------------------------
BOOLEAN PacketSetMaxLookaheadsize (LPADAPTER AdapterObject)
{
	BOOLEAN Status;
    ULONG IoCtlBufferLength = (sizeof (PACKET_OID_DATA) + sizeof (ULONG) - 1);
    PPACKET_OID_DATA OidData;

	ODS ("Packet32: PacketSetFilter\n");
	_ASSERTE (AdapterObject != NULL);
    OidData = GlobalAllocPtr (
			       GMEM_MOVEABLE | GMEM_ZEROINIT,
			       IoCtlBufferLength
      );
    if (OidData == NULL)
	{
		return FALSE;
	}
	//set the size of the lookahead buffer to the maximum available by the the NIC driver
    OidData->Oid=OID_GEN_MAXIMUM_LOOKAHEAD;
    OidData->Length=sizeof(ULONG);
    Status=PacketRequest(AdapterObject,FALSE,OidData);
    OidData->Oid=OID_GEN_CURRENT_LOOKAHEAD;
    Status=PacketRequest(AdapterObject,TRUE,OidData);
    GlobalFreePtr(OidData);
    return Status;
}

//---------------------------------------------------------------------------

BOOLEAN StartPacketDriver (LPTSTR lpstrServiceName)
{

    ODS ("Packet32: StartPacketDriver\n"); 

	return TRUE;
}

//---------------------------------------------------------------------------
BOOLEAN StopPacketDriver(void)
{
	
    ODS ("Packet32: StopPacketDriver\n"); 

	return TRUE;
}

//---------------------------------------------------------------------------

BOOLEAN PacketGetAdapterNames (PTSTR pStr,
				PULONG BufferSize)
{
    ULONG Result,i;
    LONG		Status;
	char		*TpStr;
	char		*TTpStr,*DpStr;
	LPADAPTER	adapter;
    PPACKET_OID_DATA  OidData;
    HKEY		Key,Key1;
	ULONG		BSize;
	ULONG		dim;
	char		NdisName[32];

    OidData=GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,256);
    if (OidData == NULL) {
        return FALSE;
    }

    Status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM",0,KEY_READ,&Key);
    Status=RegOpenKeyEx(Key,"CurrentControlSet",0,KEY_READ,&Key);
    Status=RegOpenKeyEx(Key,"Services",0,KEY_READ,&Key);
    Status=RegOpenKeyEx(Key,"class",0,KEY_READ,&Key);
    Status=RegOpenKeyEx(Key,"net",0,KEY_READ,&Key);
    if (Status != ERROR_SUCCESS) return FALSE;

	TpStr=pStr;
	BSize=*BufferSize;
	i=0;
	while((Result=RegEnumKey(Key,i,NdisName,32))==ERROR_SUCCESS)
	{
		Status=RegOpenKeyEx(Key,NdisName,0,KEY_READ,&Key1);
		Status=RegOpenKeyEx(Key1,"NDIS",0,KEY_READ,&Key1);
		dim=BSize;
        Status=RegQueryValueEx(Key1,"LOGDRIVERNAME",NULL,NULL,(LPBYTE)TpStr,&dim);
		i++;
		if(Status!=ERROR_SUCCESS) continue;
		BSize-=dim;
		TpStr+=dim;
	}
	
	TpStr[0]=0;
	*BufferSize-=BSize;

	if(Result==259){  //259 means OK
		i=0;
	
		(*BufferSize)++;
		TpStr=pStr;
		DpStr=pStr+*BufferSize;	
		
		while(*TpStr!=0){
			
			ODSEx("Found adapter: %s\n", TpStr);

			adapter=PacketOpenAdapter(TpStr);
			if(adapter==NULL){
				strcpy(DpStr,"Unknown");
				DpStr+=7;
				*DpStr++=0;
				
				while(*TpStr!=0){
					TpStr++;
				}
				
				TpStr++;
				continue;
			}
			
			OidData->Oid = OID_GEN_VENDOR_DESCRIPTION;
			OidData->Length = 256;
			Status = PacketRequest(adapter,FALSE,OidData);
			if(Status==0){
				strcpy(DpStr,"Unknown");

				ODSEx("Adapter description: %s\n", DpStr);
				
				DpStr+=7;
				*DpStr++=0;
				
				while(*TpStr!=0){
					TpStr++;
				}
				
				TpStr++;
				continue;
			}

			TTpStr=(char*)(OidData->Data);

			ODSEx("Adapter description: %s\n", TTpStr);

			while(*TTpStr!=0){
				*DpStr++=*TTpStr++;
			}
			*DpStr++=*TTpStr++;
			
			PacketCloseAdapter(adapter);
			
			while(*TpStr!=0){
				TpStr++;
			}
			
			TpStr++;
		}
		*DpStr=0;

		return TRUE;
	}
	
	return FALSE;
}

//---------------------------------------------------------------------------

BOOLEAN PacketGetNetInfo(LPTSTR AdapterName, PULONG netp, PULONG maskp)
{
	struct hostent* h;
	char szBuff[80];
	
    if(gethostname(szBuff, 79)) 
	{
		if(WSAGetLastError()==WSANOTINITIALISED){
			WORD wVersionRequested;
			WSADATA wsaData;

			wVersionRequested = MAKEWORD( 1, 1); 
			if(WSAStartup( wVersionRequested, &wsaData )!=0) return FALSE;

			if(gethostname(szBuff, 79))
			{
				return FALSE;
			}
			h=gethostbyname(szBuff);
			*netp=((h->h_addr_list[0][0]<<24))+
				((h->h_addr_list[0][1]<<16))+
				((h->h_addr_list[0][2]<<8))+
				((h->h_addr_list[0][3]));
			if (((*netp)&0x80000000)==0) *maskp=0xFF000000;
			else if (((*netp)&0xC0000000)==0x80000000) *maskp=0xFFFF0000;
			else if (((*netp)&0xE0000000)==0xC0000000) *maskp=0xFFFFFF00;
			else return FALSE;
			(*netp)&=*maskp;
			return TRUE;
			
		}
		else
		{
			return FALSE;
		}
	}
	
	h=gethostbyname(szBuff);
	*netp=((h->h_addr_list[0][0]<<24))+
		((h->h_addr_list[0][1]<<16))+
		((h->h_addr_list[0][2]<<8))+
		((h->h_addr_list[0][3]));
	if (((*netp)&0x80000000)==0) *maskp=0xFF000000;
	else if (((*netp)&0xC0000000)==0x80000000) *maskp=0xFFFF0000;
	else if (((*netp)&0xE0000000)==0xC0000000) *maskp=0xFFFFFF00;
	else return FALSE;
	(*netp)&=*maskp;
	
	return TRUE;
	
}

//---------------------------------------------------------------------------

/* Convert a ASCII dotted-quad to a 32-bit IP address.
   Doesn't check to make sure it's valid. */

ULONG inet_addrU(const char *cp)
{
	ULONG val, part;
	WCHAR c;
	int i;

	val = 0;
	for (i = 0; i < 4; i++) {
		part = 0;
		while ((c = *cp++) != '\0' && c != '.' && c != ',') {
			if (c < '0' || c > '9')
				return -1;
			part = part*10 + (c - '0');
		}
		if (part > 255)
			return -1;	
		val = val | (part << i*8);
		if (i == 3) {
			if (c != '\0' && c != ',')
				return -1;	// extra gunk at end of string 
		} else {
			if (c == '\0' || c == ',')
				return -1;	// string ends early 
		}
	}
	return val;
}

//---------------------------------------------------------------------------

BOOLEAN PacketGetNetInfoEx(LPTSTR AdapterName, npf_if_addr* buffer, PLONG NEntries)
{
	HKEY	InterfaceKey,CycleKey,NdisKey;
	LONG	status;
	TCHAR	String[1024+1];
	DWORD	RegType;
	ULONG	BufLen;
	struct	sockaddr_in *TmpAddr, *TmpBroad;
	LONG	naddrs,nmasks;
	ULONG	StringPos;
	char	CurAdapName[256];
	char	NdisName[256];
	ULONG	IIndex;
	ULONG	Result;

	// Reach the class\net registry key
	status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Services\\class\\net",0,KEY_READ,&InterfaceKey);
    if (status != ERROR_SUCCESS) return FALSE;

	// Scan the subkeys to determine the index of the current adapter
	IIndex=0;
	while((Result=RegEnumKey(InterfaceKey,IIndex,NdisName,sizeof NdisName))==ERROR_SUCCESS)
	{
		status=RegOpenKeyEx(InterfaceKey,NdisName,0,KEY_READ,&CycleKey);
		status=RegOpenKeyEx(CycleKey,"NDIS",0,KEY_READ,&NdisKey);
		BufLen=256;
        status=RegQueryValueEx(NdisKey,"LOGDRIVERNAME",NULL,NULL,CurAdapName,&BufLen);
		RegCloseKey(CycleKey);
		RegCloseKey(NdisKey);
		if(!strcmp(AdapterName, CurAdapName)) 
			break;
		IIndex++;
	}

	RegCloseKey(InterfaceKey);

	// Reach the Enum\Network\MSTCP registry key and open the key at position IIndex
	status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Enum\\Network\\MSTCP",0,KEY_READ,&InterfaceKey);
	status=RegEnumKey(InterfaceKey,IIndex,NdisName,sizeof NdisName);
	status=RegOpenKeyEx(InterfaceKey,NdisName,0,KEY_READ,&CycleKey);

    if (status != ERROR_SUCCESS) return FALSE;

	BufLen=sizeof NdisName;
    status=RegQueryValueEx(CycleKey,"Driver",NULL,NULL,NdisName,&BufLen);

	RegCloseKey(InterfaceKey);
	RegCloseKey(CycleKey);

	// Now go to the just obtained NetTrans Entry
	status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SYSTEM\\CurrentControlSet\\Services\\class",0,KEY_READ,&InterfaceKey);
	status=RegOpenKeyEx(InterfaceKey,NdisName,0,KEY_READ,&CycleKey);
	
	if (status != ERROR_SUCCESS) return FALSE;
	
	BufLen = sizeof String;
	// Open the key with the addresses
	status = RegQueryValueEx(CycleKey,"IPAddress",NULL,&RegType,String,&BufLen);
	if (status != ERROR_SUCCESS){
		RegCloseKey(InterfaceKey);
		RegCloseKey(CycleKey);
		return FALSE;
	}
	
	// scan the key to obtain the addresses
	StringPos = 0;
	for(naddrs = 0;naddrs < *NEntries;naddrs++){
		TmpAddr = (struct sockaddr_in *) &(buffer[naddrs].IPAddress);
		
		if((TmpAddr->sin_addr.S_un.S_addr = inet_addrU(String + StringPos))!= -1){
			TmpAddr->sin_family = AF_INET;
			
			TmpBroad = (struct sockaddr_in *) &(buffer[naddrs].Broadcast);
			TmpBroad->sin_family = AF_INET;
			// Don't know where to find the broadcast adrr under Win9x, default to 255.255.255.255
			TmpBroad->sin_addr.S_un.S_addr = 0xffffffff;

			while(*(String + StringPos) != '\0' && *(String + StringPos) != ',')StringPos++;
			StringPos++;
			
			if(*(String + StringPos) == 0 || StringPos >= BufLen)
				break;
		}
		else break;
	}
	
	BufLen = sizeof String;
	// Open the key with the addresses
	status = RegQueryValueEx(CycleKey,"IPMask",NULL,&RegType,String,&BufLen);
	if (status != ERROR_SUCCESS){
		RegCloseKey(InterfaceKey);
		RegCloseKey(CycleKey);
		return FALSE;
	}
	
	// scan the key to obtain the masks
	StringPos = 0;
	for(nmasks = 0;nmasks <* NEntries;nmasks++){
		TmpAddr = (struct sockaddr_in *) &(buffer[nmasks].SubnetMask);
		
		if((TmpAddr->sin_addr.S_un.S_addr = inet_addrU(String + StringPos))!= -1){
			TmpAddr->sin_family = AF_INET;
			
			while(*(String + StringPos) != '\0' && *(String + StringPos) != ',')StringPos++;
			StringPos++;
			
			if(*(String + StringPos) == 0 || StringPos >= BufLen)
				break;
		}
		else break;
	}		
	
	RegCloseKey(InterfaceKey);
	RegCloseKey(CycleKey);

	// The number of masks MUST be equal to the number of adresses
	if(nmasks != naddrs){
		return FALSE;
	}		
	
	*NEntries = naddrs + 1;

	return TRUE;
}

//---------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif				