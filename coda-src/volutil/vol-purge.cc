#ifndef _BLURB_
#define _BLURB_
/*

            Coda: an Experimental Distributed File System
                             Release 4.0

          Copyright (c) 1987-1996 Carnegie Mellon University
                         All Rights Reserved

Permission  to  use, copy, modify and distribute this software and its
documentation is hereby granted,  provided  that  both  the  copyright
notice  and  this  permission  notice  appear  in  all  copies  of the
software, derivative works or  modified  versions,  and  any  portions
thereof, and that both notices appear in supporting documentation, and
that credit is given to Carnegie Mellon University  in  all  documents
and publicity pertaining to direct or indirect use of this code or its
derivatives.

CODA IS AN EXPERIMENTAL SOFTWARE SYSTEM AND IS  KNOWN  TO  HAVE  BUGS,
SOME  OF  WHICH MAY HAVE SERIOUS CONSEQUENCES.  CARNEGIE MELLON ALLOWS
FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.   CARNEGIE  MELLON
DISCLAIMS  ANY  LIABILITY  OF  ANY  KIND  FOR  ANY  DAMAGES WHATSOEVER
RESULTING DIRECTLY OR INDIRECTLY FROM THE USE OF THIS SOFTWARE  OR  OF
ANY DERIVATIVE WORK.

Carnegie  Mellon  encourages  users  of  this  software  to return any
improvements or extensions that  they  make,  and  to  grant  Carnegie
Mellon the rights to redistribute these changes without encumbrance.
*/

static char *rcsid = "$Header: /afs/cs/project/coda-src/cvs/coda/coda-src/volutil/vol-purge.cc,v 4.5 1998/10/29 15:29:04 braam Exp $";
#endif /*_BLURB_*/


/*

                         IBM COPYRIGHT NOTICE

                          Copyright (C) 1986
             International Business Machines Corporation
                         All Rights Reserved

This  file  contains  some  code identical to or derived from the 1986
version of the Andrew File System ("AFS"), which is owned by  the  IBM
Corporation.    This  code is provded "AS IS" and IBM does not warrant
that it is free of infringement of  any  intellectual  rights  of  any
third  party.    IBM  disclaims  liability of any kind for any damages
whatsoever resulting directly or indirectly from use of this  software
or  of  any  derivative work.  Carnegie Mellon University has obtained
permission to distribute this code, which is based on Version 2 of AFS
and  does  not  contain the features and enhancements that are part of
Version 3 of AFS.  Version 3 of  AFS  is  commercially  available  and
supported by Transarc Corporation, Pittsburgh, PA.

*/

#ifdef __cplusplus
extern "C" {
#endif __cplusplus

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <unistd.h>
#include <stdlib.h>
#include <lwp.h>
#include <lock.h>
#include <util.h>
#include <rvmlib.h>
#include <volutil.h>
#include <vice.h>

#ifdef __cplusplus
}
#endif __cplusplus

#include <cvnode.h>
#include <volume.h>
#include <viceinode.h>
#include <partition.h>
#include <srvsignal.h>
#include <vutil.h>
#include <recov.h>


/*
  S_VolPurge: Purge the requested volume
*/
long int S_VolPurge(RPC2_Handle rpcid, RPC2_Unsigned formal_purgeId, 
		    RPC2_String formal_purgeName) 
{
    Error error = 0;
    Error error2 = 0;
    Volume *vp = NULL;
    int status = 0;
    int rc = 0;
    ProgramType *pt;
    int	AlreadyOffline = 0;

    /* To keep C++ 2.0 happy */
    char *purgeName = (char *)formal_purgeName;
    VolumeId purgeId = (VolumeId)formal_purgeId;

    VLog(69, "Checking lwp rock in S_VolPurge");
    CODA_ASSERT(LWP_GetRock(FSTAG, (char **)&pt) == LWP_SUCCESS);

    VLog(9, "Entering S_VolPurge: purgeId = %x, purgeName = %s",
					    purgeId, purgeName);
    rc = VInitVolUtil(volumeUtility);
    if (rc != 0) {
	VLog(0, "S_VolPurge: returned %ld from VInitVolUtil; aborting", rc);
	return rc;
    }

    vp = VGetVolume(&error, purgeId);	/* Does this need a transaction? */
    if (error){
	if (error == VOFFLINE){
	    VLog(9, "VolPurge: Volume %x was already offline", V_id(vp));
	    AlreadyOffline = 1;
	} else if (error == VNOVOL){
	    /* volume is not attached or is shutting down */
	    vp = VAttachVolume(&error2, purgeId, V_UPDATE);
	    if (error2) {
		VLog(0, "Unable to attach volume %x; not purged", purgeId);
		rc = VNOVOL;
	    }
	    AlreadyOffline = 1;
	} else {
	    if (vp)
		VPutVolume(vp);
	    VLog(0, "VolPurge: GetVolume %x  returns error %d", purgeId, error);
	    rc = error;
	    goto exit;
	}
    }

    CODA_ASSERT(vp != NULL);
    if (strcmp(V_name(vp), purgeName) != 0) {
	VLog(0, "The name you specified (%s) does not match the internal name (%s) for volume %x; not purged",
	   (int) purgeName, (int) V_name(vp), purgeId);
	VPutVolume(vp);
	status = VNOVOL;
	goto exit;
    }

    if (!AlreadyOffline){
	/* force the volume offline */
	VLog(9, "VolPurge: Forcing Volume %x offline", V_id(vp));
	*pt = fileServer;
	VOffline(vp, "Volume being Purged");
	*pt = volumeUtility;
	vp = VGetVolume(&error, purgeId);
	CODA_ASSERT(error == VOFFLINE);
    }

    if (status != 0) {
	VLog(0, "S_VolPurge: Transaction aborted!");
	VDisconnectFS();
	return status;
    }

    /* By this time the volume is attached and is offline */
    CODA_ASSERT(V_inUse(vp) == 0);
    CODA_ASSERT(DeleteVolume(vp) == 0);  /* Remove the volume from rvm and vm */
    vp->shuttingDown = 1;

    /* Don't need to call VPutVolume since all vm traces have been removed. */
    VListVolumes();			/* Create updated /vice/vol/VolumeList */

    PrintVolumesInHashTable();
 exit:
    VDisconnectFS();
    VLog(0, "purge: volume %x (%s) purged", purgeId, purgeName);
    return(status?status:rc);
}
