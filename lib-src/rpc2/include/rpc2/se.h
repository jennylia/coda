/* BLURB lgpl

                           Coda File System
                              Release 5

          Copyright (c) 1987-1999 Carnegie Mellon University
                  Additional copyrights listed below

This  code  is  distributed "AS IS" without warranty of any kind under
the  terms of the  GNU  Library General Public Licence  Version 2,  as
shown in the file LICENSE. The technical and financial contributors to
Coda are listed in the file CREDITS.

                        Additional copyrights

#*/

/*
                         IBM COPYRIGHT NOTICE

                          Copyright (C) 1986
             International Business Machines Corporation
                         All Rights Reserved

This  file  contains  some  code identical to or derived from the 1986
version of the Andrew File System ("AFS"), which is owned by  the  IBM
Corporation.   This  code is provided "AS IS" and IBM does not warrant
that it is free of infringement of  any  intellectual  rights  of  any
third  party.    IBM  disclaims  liability of any kind for any damages
whatsoever resulting directly or indirectly from use of this  software
or  of  any  derivative work.  Carnegie Mellon University has obtained
permission to  modify,  distribute and sublicense this code,  which is
based on Version 2  of  AFS  and  does  not  contain  the features and
enhancements that are part of  Version 3 of  AFS.  Version 3 of AFS is
commercially   available   and  supported  by   Transarc  Corporation,
Pittsburgh, PA.

*/

#ifndef _SE_
#define _SE_

struct SE_Definition
    {
    long SideEffectType;	/* what kind of side effect am I? */
    long (*SE_Init)();		/* on both client & server side */
    long (*SE_Bind1)();		/* on client side */
    long (*SE_Bind2)();		/* on client side */
    long (*SE_Unbind)();	/* on client and server side */
    long (*SE_NewConnection)();	/* on server side */
    long (*SE_MakeRPC1)();	/* on client side */
    long (*SE_MakeRPC2)();	/* on client side */
    long (*SE_MultiRPC1)();	/* on client side */
    long (*SE_MultiRPC2)();	/* on client side */
    long (*SE_CreateMgrp)();	/* on client side */
    long (*SE_AddToMgrp)();	/* on client side */
    long (*SE_InitMulticast)();	/* on server side */
    long (*SE_DeleteMgrp)();	/* on client and server side */
    long (*SE_GetRequest)();	/* on server side */
    long (*SE_InitSideEffect)();	/* on server side */
    long (*SE_CheckSideEffect)();	/* on server side */
    long (*SE_SendResponse)();	/* on server side */
    long (*SE_PrintSEDescriptor)();	/* for debugging */
    long (*SE_SetDefaults)();	/* for initialization */
    long (*SE_GetSideEffectTime)(); 
    long (*SE_GetHostInfo)();	
    };


 /* Types of side effects: use these in the RPC2_Bind() call and in filling SE descriptors */
#define OMITSE   9999   /* useful in MultiRPC for omitting side effects on some conns */
#define SMARTFTP 1189



enum WhichWay {CLIENTTOSERVER=93, SERVERTOCLIENT=87};
enum FileInfoTag {FILEBYNAME=33, FILEBYINODE=58, FILEBYFD=67, FILEINVM=74};

struct SFTP_Descriptor
    {
    enum WhichWay TransmissionDirection; /* IN */
    char hashmark;		/* IN: 0 for non-verbose transfer */
    long SeekOffset;		/* IN:  >= 0; position to seek to before first read or write */
    long BytesTransferred;	/* OUT: value after RPC2_CheckSideEffect() meaningful */
    long ByteQuota;		/* IN: maximum number of data bytes  to be sent or received.
				    A value of -1 implies infinity. 
				    Transfer is terminated and QuotaExceeded set if this
				    limit would be exceeded.
				    EnforceQuota in SFTP_Initializer must be  specified as 1
				    at RPC initialization for the quota enforcement to take place.
				    
				    NOTE:  (2/6/1994, Satya) The semantics is being slightly changed
				    here to support partial file transfer; it used to be the case
				    that hitting ByteQuota was an error reported as RPC2_SEFAIL1.
				    But no one seems to be relying on this, so changing the error
				    return to a success return seems fair game.
				*/
    long QuotaExceeded;		/* OUT: set to 1 if transfer terminated due to ByteQuota limit
					0 otherwise */
    enum FileInfoTag Tag;	/* IN */
    union
	{
	struct FileInfoByName
	    {
	    long ProtectionBits;	/* Unix mode bits to be set for created files */
	    char LocalFileName[256];
	    }
	    ByName;	/* if (Tag == FILEBYNAME); standard Unix open() */
	    
	struct FileInfoByInode
	    {
	    long Device;		/* device on which file  resides */
	    long Inode;		/* inode number of file (inode MUST exist already)*/
	    }
	    ByInode;	/* if (Tag == FILEBYINODE); ITC inode-open */

	struct FileInfoByFD
	    {
	    long fd;		/* fd of already-open file (not automatically closed!) */
	    }
	    ByFD;	/* if (Tag == FILEBYFD); user gives already-open file */

	struct FileInfoByAddr
	    {
	    /*  Describes buffer allocated by user in VM.
		When file used as source:
		    - user sets vmfile.SeqLen to actual file length.
		    - SFTP ignores vmfile.MaxSeqLen
		When used as sink:
		    - user sets vmfile.MaxSeqLen
		    - SFTP sets vmfile.SeqLen to length of received file.
		    - SFTP returns RPC2_SEFAIL3 if file bigger than MaxSeqLen.
	    */
	    RPC2_BoundedBS vmfile;
	    long vmfilep; /* for internal use by SFTP as file pointer */
	    }
	    ByAddr;     /* if (Tag == FILEINVM); file resides in VM */
	}
	FileInfo;	/* everything is IN */
    };


enum SE_Status {SE_NOTSTARTED=33, SE_INPROGRESS=24, SE_SUCCESS=57, SE_FAILURE=36};


typedef
    struct SE_SideEffectDescriptor
	{
	enum SE_Status LocalStatus;
	enum SE_Status RemoteStatus;
	long Tag;	/* only SMARTFTP or OMITSE */
	union
	    {
	    /* nothing for OMITSE */
	    struct SFTP_Descriptor SmartFTPD;
	    }
	    Value;

       /* this is a callback function, which is called whenever a block of
	* data is succesfully transferred (sink side only for now). */
        void (*XferCB) (void *userp, unsigned int offset);
        void *userp;
	}
	SE_Descriptor;



typedef struct SFTPI
    {
    long PacketSize;	/* bytes in data packet */
    long WindowSize;	/* max number of outstanding unacknowledged packets */
    long RetryCount;	
    long RetryInterval;	/* in milliseconds */
    long SendAhead;	/* number of packets to read and send ahead */
    long AckPoint;	/* when to send ack */
    long EnforceQuota;  /* 0 ==> don't */
    long DoPiggy;		/* FALSE ==> don't piggyback small files */
    long DupThreshold;	/* Duplicates allowed before spontaneous Ack is sent */
    long MaxPackets; 	/* Memory usage throttle; SFTP will not use more than
    			this many packets in total; -1 (default) says no limit;
			Caveat user: packet starvation can cause mysterious
			RPC2_SEFAIL2s */
    RPC2_PortIdent Port;	/* initialization required on server side */
    } SFTP_Initializer;



/*
Flag options in RPC2_CheckSEStatus(): OR these together as needed
*/
#define	SE_AWAITLOCALSTATUS	1
#define SE_AWAITREMOTESTATUS	2

extern struct SE_Definition *SE_DefSpecs;	/* array */
extern long SE_DefCount; /* how many are there? */
extern void SE_SetDefaults();
extern char *SE_ErrorMsg();

/*
  Statistics
*/

/* I'm not sure where these should go. -JJK */
struct sftpStats
    {
    unsigned long
	Total,		/* Packets Sent (Received) */
	Starts,		/* Starts Sent (Received) */
	Datas,		/* Datas Sent (Received) */
	DataRetries,	/* Data Retries Sent (Received) */
	Acks,		/* Acks Sent (Received) */
	Naks,		/* Naks Sent (Received) */
	Busies,		/* Busies Sent (Received) */
	Bytes,		/* Bytes Sent (Received) */
 	Timeouts;	/* Timeouts when Sending (Receiving) */
};

extern struct sftpStats sftp_Sent, sftp_MSent;
extern struct sftpStats sftp_Recvd, sftp_MRecvd;
#endif
