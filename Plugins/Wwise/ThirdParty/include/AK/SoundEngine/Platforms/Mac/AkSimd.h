/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Apache License Usage

Alternatively, this file may be used under the Apache License, Version 2.0 (the 
"Apache License"); you may not use this file except in compliance with the 
Apache License. You may obtain a copy of the Apache License at 
http://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for
the specific language governing permissions and limitations under the License.

  Version: v2021.1.13  Build: 8036
  Copyright (c) 2006-2023 Audiokinetic Inc.
*******************************************************************************/

// AkSimd.h

/// \file 
/// AKSIMD - Mac implementation

#ifndef _AK_SIMD_PLATFORM_H_
#define _AK_SIMD_PLATFORM_H_

#include <AK/SoundEngine/Common/AkTypes.h>

#undef AKSIMD_GETELEMENT_V4F32
#define AKSIMD_GETELEMENT_V4F32( __vName, __num__ )            ((float*)&(__vName))[(__num__)]                            ///< Retrieve scalar element from vector.

#undef AKSIMD_GETELEMENT_V2F32
#define AKSIMD_GETELEMENT_V2F32( __vName, __num__ )            ((float*)&(__vName))[(__num__)]                            ///< Retrieve scalar element from vector.

#undef AKSIMD_GETELEMENT_V4I32
#define AKSIMD_GETELEMENT_V4I32( __vName, __num__ )            ((int*)&(__vName))[(__num__)]                            ///< Retrieve scalar element from vector.

#ifdef AK_CPU_ARM_NEON
#define AK_IOS_ARM_NEON
#include <AK/SoundEngine/Platforms/arm_neon/AkSimd.h>
#elif defined AK_CPU_X86_64 || defined AK_CPU_X86
#include <AK/SoundEngine/Platforms/SSE/AkSimd.h>
#else
#include <AK/SoundEngine/Platforms/Generic/AkSimd.h>
#endif

#endif //_AK_SIMD_PLATFORM_H_
