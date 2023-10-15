/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2023 Audiokinetic Inc.
*******************************************************************************/

/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

  Version: <VERSION>  Build: <BUILDNUMBER>
  Copyright (c) <COPYRIGHTYEAR> Audiokinetic Inc.
*******************************************************************************/
//////////////////////////////////////////////////////////////////////
// 
// AkFilePackage.h
//
// This class represents a file package that was created with the 
// AkFilePackager utility app (located in ($WWISESDK)/samples/FilePackager/). 
// It holds a system file handle and a look-up table (CAkFilePackageLUT).
//
// CAkFilePackage objects can be chained together using the ListFilePackages
// typedef defined below.
//
//////////////////////////////////////////////////////////////////////

#include "AkFilePackage.h"

// Destroy file package and free memory / destroy pool.
void CAkFilePackage::Destroy()
{
	// Cache memory pointer because memory pool is destroyed _after_ deleting this.
	void * pToRelease	= m_pToRelease;

	// Call destructor.
	this->~CAkFilePackage();

	// Free memory.
	ClearMemory(pToRelease);
}

void CAkFilePackage::ClearMemory(
	void *		in_pMemToRelease	// Memory block to free before destroying pool.
	)
{
	if (in_pMemToRelease)
	{
		AkFree(AkMemID_FilePackage, in_pMemToRelease);
	}
}
