#ifndef _FRONTENDAMB_LV_WRAPPER_DLL_EXPORT_H_
#define _FRONTENDAMB_LV_WRAPPER_DLL_EXPORT_H_
/*******************************************************************************
* ALMA - Atacama Large Millimeter Array
* (c) Associated Universities Inc., 2022
*
*This library is free software; you can redistribute it and/or
*modify it under the terms of the GNU Lesser General Public
*License as published by the Free Software Foundation; either
*version 2.1 of the License, or (at your option) any later version.
*
*This library is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*Lesser General Public License for more details.
*
*You should have received a copy of the GNU Lesser General Public
*License along with this library; if not, write to the Free Software
*Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*
*/

/************************************************************************
 * Macros for exporting functions from a Windows DLL.
 * 
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
     #define LINKAGE extern "C"
#else
     #define LINKAGE
#endif


#ifdef BUILD_FRONTENDAMB
    #define DLL_API LINKAGE __declspec(dllexport)
#else
    #define DLL_API LINKAGE __declspec(dllimport)
#endif


#define DLL_CALL __cdecl


#endif /*_FRONTENDAMB_LV_WRAPPER_DLL_EXPORT_H_*/
