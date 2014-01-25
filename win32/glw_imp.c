/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
** GLW_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/
#include <assert.h>
#include <windows.h>
#include "../renderer/r_local.h"
#include "resource.h"
#include "glw_win.h"
#include "winquake.h"
#include "../vr/vr.h"

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL (void);

glwstate_t glw_state;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;

static qboolean VerifyDriver( void )
{
	char buffer[1024];

	strcpy( buffer, glGetString( GL_RENDERER ) );
	strlwr( buffer );
	if ( strcmp( buffer, "gdi generic" ) == 0 )
		if ( !glw_state.mcd_accelerated )
			return false;
	return true;
}

qboolean modType (char *name);

/*
** VID_CreateWindow
*/
#define	WINDOW_CLASS_NAME	"Quake II VR" // changed
#define	WINDOW_CLASS_NAME2	"Quake II VR - The Reckoning" // changed
#define	WINDOW_CLASS_NAME3	"Quake II VR - Ground Zero" // changed

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
	WNDCLASS		wc;
	RECT			r;
	cvar_t			*vid_xpos, *vid_ypos;
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)glw_state.wndproc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = glw_state.hInstance;

	if (modType("xatrix")) { // q2mp1
		wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON2));
		//wc.lpszClassName = WINDOW_CLASS_NAME2;
	}
	else if (modType("rogue"))  { // q2mp2
		wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON3));
		//wc.lpszClassName = WINDOW_CLASS_NAME3;
	}
	else {
		wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	}

    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (void *)COLOR_GRAYTEXT;
    wc.lpszMenuName  = 0;
	wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClass (&wc) )
		VID_Error (ERR_FATAL, "Couldn't register window class");

	if (fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		//stylebits = WS_POPUP|WS_VISIBLE;
		stylebits = WS_POPUP|WS_SYSMENU|WS_VISIBLE;
	}
	else
	{
		exstyle = 0;
		//stylebits = WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE;
		stylebits = WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_VISIBLE;
	}

	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

	AdjustWindowRect (&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	if (fullscreen)
	{
		x = 0;
		y = 0;
	}
	else
	{
		vid_xpos = Cvar_Get ("vid_xpos", "0", 0);
		vid_ypos = Cvar_Get ("vid_ypos", "0", 0);
		x = vid_xpos->value;
		y = vid_ypos->value;
	}

	glw_state.hWnd = CreateWindowEx (
		 exstyle, 
		 WINDOW_CLASS_NAME,
		 "Quake II VR",		//dghost changed
		 stylebits,
		 x, y, w, h,
		 NULL,
		 NULL,
		 glw_state.hInstance,
		 NULL);

	if (!glw_state.hWnd)
		VID_Error (ERR_FATAL, "Couldn't create window");
	
	ShowWindow( glw_state.hWnd, SW_SHOW );
	UpdateWindow( glw_state.hWnd );

	// init all the gl stuff for the window
	if (!GLimp_InitGL ())
	{
		VID_Printf( PRINT_ALL, "VID_CreateWindow() - GLimp_InitGL failed\n");
		return false;
	}

	SetForegroundWindow( glw_state.hWnd );
	SetFocus( glw_state.hWnd );

	// let the sound and input subsystems know about the new window
	VID_NewWindow (width, height);

	return true;
}


qboolean VR_CreateWindow(int xPos, int yPos, int width, int height)
{
	WNDCLASS		wc;
	RECT			r;
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)glw_state.wndproc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = glw_state.hInstance;

	if (modType("xatrix")) { // q2mp1
		wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON2));
		//wc.lpszClassName = WINDOW_CLASS_NAME2;
	}
	else if (modType("rogue"))  { // q2mp2
		wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON3));
		//wc.lpszClassName = WINDOW_CLASS_NAME3;
	}
	else {
		wc.hIcon         = LoadIcon(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	}

    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (void *)COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WINDOW_CLASS_NAME;

	if (!RegisterClass (&wc) )
		VID_Error (ERR_FATAL, "Couldn't register window class");

	exstyle = WS_EX_TOPMOST;
	//stylebits = WS_POPUP|WS_VISIBLE;
	stylebits = WS_POPUP|WS_SYSMENU|WS_VISIBLE;

	r.left = xPos;
	r.top = yPos;
	r.right  = xPos + width;
	r.bottom = yPos + height;

	AdjustWindowRect (&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	x = r.left;
	y = r.top;

	Cvar_SetInteger("vid_xpos",x);
	Cvar_SetInteger("vid_ypos",y);


	glw_state.hWnd = CreateWindowEx (
		exstyle, 
		WINDOW_CLASS_NAME,
		"Quake II VR",		//dghost changed
		stylebits,
		x, y, w, h,
		NULL,
		NULL,
		glw_state.hInstance,
		NULL);

	if (!glw_state.hWnd)
		VID_Error (ERR_FATAL, "Couldn't create window");

	ShowWindow( glw_state.hWnd, SW_SHOW );
	UpdateWindow( glw_state.hWnd );

	// init all the gl stuff for the window
	if (!GLimp_InitGL ())
	{
		VID_Printf( PRINT_ALL, "VID_CreateWindow() - GLimp_InitGL failed\n");
		return false;
	}

	SetForegroundWindow( glw_state.hWnd );
	SetFocus( glw_state.hWnd );

	// let the sound and input subsystems know about the new window
	VID_NewWindow (width, height);

	return true;
}

/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode ( int *pwidth, int *pheight, qboolean fullscreen )
{
	int width, height;
	const char *win_fs[] = { "W", "FS" };

	width = Cvar_VariableInteger("r_width");
	height = Cvar_VariableInteger("r_height");

	VID_Printf( PRINT_ALL, "Initializing OpenGL display\n");

	VID_Printf (PRINT_ALL, "...setting mode %dx%d %s\n", width, height, win_fs[fullscreen] );

	// destroy the existing window
	if (glw_state.hWnd)
	{
		GLimp_Shutdown ();
	}

	// do a CDS if needed
	if ( fullscreen )
	{
		DEVMODE dm;
		DISPLAY_DEVICE  targetMonitor;
		ZeroMemory(&targetMonitor, sizeof(targetMonitor));
		targetMonitor.cb = sizeof(targetMonitor);


		VID_Printf( PRINT_ALL, "...attempting fullscreen\n" );

		memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );
		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		// Added refresh rate control
		if ( r_displayrefresh->value != 0 )
		{
			dm.dmDisplayFrequency = r_displayrefresh->integer;
			dm.dmFields |= DM_DISPLAYFREQUENCY;
			VID_Printf( PRINT_ALL, "...using r_displayrefresh of %d\n", (int)r_displayrefresh->value );
		}

		if ( r_bitdepth->value != 0 )
		{
			dm.dmBitsPerPel = r_bitdepth->value;
			dm.dmFields |= DM_BITSPERPEL;
			VID_Printf( PRINT_ALL, "...using r_bitdepth of %d\n", ( int ) r_bitdepth->value );
		}
		else
		{
			HDC hdc = GetDC( NULL );
			int bitspixel = GetDeviceCaps( hdc, BITSPIXEL );

			VID_Printf( PRINT_ALL, "...using desktop display depth of %d\n", bitspixel );

			ReleaseDC( 0, hdc );
		}

		if ((int) vr_enabled->value == HMD_RIFT)
		{
			qboolean found = false;
			qboolean isPrimaryDisplay = false;
			UINT i;
			DISPLAY_DEVICE displayDevice , primaryDisplay;
			ZeroMemory(&displayDevice, sizeof(displayDevice));
			displayDevice.cb = sizeof(displayDevice);
			ZeroMemory(&primaryDisplay, sizeof(primaryDisplay));
			primaryDisplay.cb = sizeof(primaryDisplay);
			VID_Printf(PRINT_ALL, "...looking for HMD\n");
			for (i = 0; EnumDisplayDevices(NULL, i, &displayDevice, 0); i++) {

				if (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE || displayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
				{
					size_t length = strlen(displayDevice.DeviceName);
					VID_Printf( PRINT_ALL,"...found display device: %s - %s\n",displayDevice.DeviceName,displayDevice.DeviceString);
					if (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
						primaryDisplay = displayDevice;
					if (!strncmp(vrConfig.deviceName,displayDevice.DeviceName,length))
					{
						found = true;
						targetMonitor = displayDevice;
						if (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
							isPrimaryDisplay = true;

					}


				}
			}
			if (found)
			{
				VID_Printf( PRINT_ALL,"...found HMD on %s display '%s'\n",isPrimaryDisplay?"primary":"secondary",targetMonitor.DeviceName);
			} else
			{
				targetMonitor = primaryDisplay;
				VID_Printf( PRINT_ALL,"...unable to locate HMD, using primary display %s\n",targetMonitor.DeviceName);
			}
		}	
		
		VID_Printf( PRINT_ALL, "...calling CDS: " );

		if (vr_enabled->value && ChangeDisplaySettingsEx(targetMonitor.DeviceName, &dm, NULL, CDS_FULLSCREEN,NULL ) == DISP_CHANGE_SUCCESSFUL)
		{
			DEVMODE settings;
			ZeroMemory(&settings, sizeof(settings));
			settings.dmSize = sizeof(settings);

			*pwidth = width;
			*pheight = height;

			glState.fullscreen = true;

			VID_Printf( PRINT_ALL, "%s ok!\n" ,targetMonitor.DeviceName);
			if (!EnumDisplaySettingsEx(targetMonitor.DeviceName,
				ENUM_CURRENT_SETTINGS,
				&settings,
				EDS_ROTATEDMODE))
			{
				VID_Printf( PRINT_ALL,"Error enumerating display settings!\n");
			}
			if ( !VR_CreateWindow (settings.dmPosition.x,settings.dmPosition.y,width, height) )
				return rserr_invalid_mode;

			return rserr_ok;

		}
		else if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL  )
		{
			*pwidth = width;
			*pheight = height;

			glState.fullscreen = true;

			VID_Printf( PRINT_ALL, "ok\n" );

			if ( !VID_CreateWindow (width, height, true) )
				return rserr_invalid_mode;

			return rserr_ok;
		}
		else
		{
			*pwidth = width;
			*pheight = height;

			VID_Printf( PRINT_ALL, "failed\n" );

			VID_Printf( PRINT_ALL, "...calling CDS assuming dual monitors:" );

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if ( r_bitdepth->value != 0 )
			{
				dm.dmBitsPerPel = r_bitdepth->value;
				dm.dmFields |= DM_BITSPERPEL;
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system 
			*/
			if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
			{
				VID_Printf( PRINT_ALL, " failed\n" );

				VID_Printf( PRINT_ALL, "...setting windowed mode\n" );

				ChangeDisplaySettings( 0, 0 );

				*pwidth = width;
				*pheight = height;
				glState.fullscreen = false;
				if ( !VID_CreateWindow (width, height, false) )
					return rserr_invalid_mode;
				return rserr_invalid_fullscreen;
			}
			else
			{
				VID_Printf( PRINT_ALL, " ok\n" );
				if ( !VID_CreateWindow (width, height, true) )
					return rserr_invalid_mode;

				glState.fullscreen = true;
				return rserr_ok;
			}
		}
	}
	else
	{
		VID_Printf( PRINT_ALL, "...setting windowed mode\n" );

		ChangeDisplaySettings( 0, 0 );

		*pwidth = width;
		*pheight = height;
		glState.fullscreen = false;
		if ( !VID_CreateWindow (width, height, false) )
			return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
WORD original_ramp[3][256]; //Knightmare added
WORD gamma_ramp[3][256];


void GLimp_Shutdown( void )
{
	//Knightmare- added Vic's hardware gamma ramp
	if ( !r_ignorehwgamma->value )
	{
	
			SetDeviceGammaRamp (glw_state.hDC, original_ramp);
	}
	//end Knightmare

	if ( wglMakeCurrent && !wglMakeCurrent( NULL, NULL ) )
		VID_Printf( PRINT_ALL, "ref_gl::R_Shutdown() - wglMakeCurrent failed\n");
	if ( glw_state.hGLRC )
	{
		if (  wglDeleteContext && !wglDeleteContext( glw_state.hGLRC ) )
			VID_Printf( PRINT_ALL, "ref_gl::R_Shutdown() - wglDeleteContext failed\n");
		glw_state.hGLRC = NULL;
	}
	if (glw_state.hDC)
	{
		if ( !ReleaseDC( glw_state.hWnd, glw_state.hDC ) )
			VID_Printf( PRINT_ALL, "ref_gl::R_Shutdown() - ReleaseDC failed\n" );
		glw_state.hDC   = NULL;
	}
	if (glw_state.hWnd)
	{	//Knightmare- remove leftover button on taskbar
		ShowWindow (glw_state.hWnd, SW_HIDE);
		//end Knightmare
		DestroyWindow (	glw_state.hWnd );
		glw_state.hWnd = NULL;
	}

	if ( glw_state.log_fp )
	{
		fclose( glw_state.log_fp );
		glw_state.log_fp = 0;
	}

	UnregisterClass (WINDOW_CLASS_NAME, glw_state.hInstance);

	if ( glState.fullscreen )
	{
		ChangeDisplaySettings( 0, 0 );
		glState.fullscreen = false;
	}
}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
*/
qboolean GLimp_Init( void *hinstance, void *wndproc )
{


	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	glw_state.allowdisplaydepthchange = true;

	glw_state.hInstance = ( HINSTANCE ) hinstance;
	glw_state.wndproc = wndproc;

	return true;
}

qboolean GLimp_InitGL (void)
{
    PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		32,								// 32-bit color depth // was 24
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		//Knightmare 12/24/2001- stencil buffer
		24,								// 24-bit z-buffer, was 32	
		8,								// 8-bit stencil buffer
		//end Knightmare
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
    };
    int  pixelformat;
	cvar_t *stereo;
	
	stereo = Cvar_Get( "cl_stereo", "0", 0 );

	/*
	** set PFD_STEREO if necessary
	*/
	if ( stereo->value != 0 )
	{
		VID_Printf( PRINT_ALL, "...attempting to use stereo\n" );
		pfd.dwFlags |= PFD_STEREO;
		glState.stereo_enabled = true;
	}
	else
	{
		glState.stereo_enabled = false;
	}

	/*
	** figure out if we're running on a minidriver or not
	*/

	Com_Printf("Using current OpenGL driver: %s\n",gl_driver->string);
	/*
	** Get a DC for the specified window
	*/
	if ( glw_state.hDC != NULL )
		VID_Printf( PRINT_ALL, "GLimp_Init() - non-NULL DC exists\n" );

    if ( ( glw_state.hDC = GetDC( glw_state.hWnd ) ) == NULL )
	{
		VID_Printf( PRINT_ALL, "GLimp_Init() - GetDC failed\n" );
		return false;
	}


	if ( ( pixelformat = ChoosePixelFormat( glw_state.hDC, &pfd)) == 0 )
	{
		VID_Printf (PRINT_ALL, "GLimp_Init() - ChoosePixelFormat failed\n");
		return false;
	}
	if ( SetPixelFormat( glw_state.hDC, pixelformat, &pfd) == FALSE )
	{
		VID_Printf (PRINT_ALL, "GLimp_Init() - SetPixelFormat failed\n");
		return false;
	}
	DescribePixelFormat( glw_state.hDC, pixelformat, sizeof( pfd ), &pfd );

	if ( !( pfd.dwFlags & PFD_GENERIC_ACCELERATED ) )
	{
		extern cvar_t *gl_allow_software;

		if ( gl_allow_software->value )
			glw_state.mcd_accelerated = true;
		else
			glw_state.mcd_accelerated = false;
	}
	else
	{
		glw_state.mcd_accelerated = true;
	}

	/*
	** report if stereo is desired but unavailable
	*/
	if ( !( pfd.dwFlags & PFD_STEREO ) && ( stereo->value != 0 ) ) 
	{
		VID_Printf( PRINT_ALL, "...failed to select stereo pixel format\n" );
		Cvar_SetValue( "cl_stereo", 0 );
		glState.stereo_enabled = false;
	}

	/*
	** startup the OpenGL subsystem by creating a context and making
	** it current
	*/
	if ( ( glw_state.hGLRC = wglCreateContext( glw_state.hDC ) ) == 0 )
	{
		VID_Printf (PRINT_ALL, "GLimp_Init() - wglCreateContext failed\n");

		goto fail;
	}

    if ( !wglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) )
	{
		VID_Printf (PRINT_ALL, "GLimp_Init() - wglMakeCurrent failed\n");

		goto fail;
	}

	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK)
	{
		VID_Printf( PRINT_ALL, "GLimp_Init() - glewInit failed\n");
		goto fail;
	}

	if ( !VerifyDriver() )
	{
		VID_Printf( PRINT_ALL, "GLimp_Init() - no hardware acceleration detected.\nPlease install drivers provided by your video card/GPU vendor.\n" );
		goto fail;
	}

	/*
	** print out PFD specifics 
	*/
	VID_Printf( PRINT_ALL, "PIXELFORMAT: color(%d-bits) Z(%d-bit)\n", ( int ) pfd.cColorBits, ( int ) pfd.cDepthBits );

	//Knightmare- Vic's hardware gamma stuff
	if ( !r_ignorehwgamma->value )
	{
			glState.gammaRamp = GetDeviceGammaRamp ( glw_state.hDC, original_ramp );
	}
	else
	{
		glState.gammaRamp = false;
	}

	if (glState.gammaRamp)
		vid_gamma->modified = true;

	// moved these to GL_SetDefaultState
	//glState.blend = false;
	//glState.alphaTest = false;
	//end Knightmare

	//Knightmare- 12/24/2001- stecil buffer
	{
		char buffer[1024];

		strcpy( buffer, glGetString( GL_RENDERER ) );
		strlwr( buffer );
		if (pfd.cStencilBits) {
			VID_Printf( PRINT_ALL, "... Using stencil buffer\n" );
			glConfig.have_stencil = true; // Stencil shadows - MrG
		}
	}
	//if (pfd.cStencilBits)
	//	glConfig.have_stencil = true;
	//end Knightmare

	/*	Moved to GL_SetDefaultState in r_glstate.c
	// Vertex arrays
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_COLOR_ARRAY);

	glTexCoordPointer (2, GL_FLOAT, sizeof(texCoordArray[0][0]), texCoordArray[0][0]);
	glVertexPointer (3, GL_FLOAT, sizeof(vertexArray[0]), vertexArray[0]);
	glColorPointer (4, GL_FLOAT, sizeof(colorArray[0]), colorArray[0]);
	//glState.activetmu[0] = true;
	// end vertex arrays
	*/

	return true;

fail:
	if ( glw_state.hGLRC )
	{
		wglDeleteContext( glw_state.hGLRC );
		glw_state.hGLRC = NULL;
	}

	if ( glw_state.hDC )
	{
		ReleaseDC( glw_state.hWnd, glw_state.hDC );
		glw_state.hDC = NULL;
	}
	return false;
}

//Knightmare- added Vic's hardware gammaramp
void UpdateGammaRamp (void)
{
	int i, o;

	if (!glState.gammaRamp)
		return;
	
	memcpy (gamma_ramp, original_ramp, sizeof(original_ramp));

	for (o = 0; o < 3; o++) 
	{
		for (i = 0; i < 256; i++) 
		{
			signed int v;

			v = 255 * pow ((i+0.5)/255.5, vid_gamma->value ) + 0.5;
			if (v > 255) v = 255;
			if (v < 0) v = 0;
			gamma_ramp[o][i] = ((WORD)v) << 8;
		}
	}
	SetDeviceGammaRamp ( glw_state.hDC, gamma_ramp );
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_separation )
{
	if ( r_bitdepth->modified )
	{
		if ( r_bitdepth->value != 0 && !glw_state.allowdisplaydepthchange )
		{
			Cvar_SetValue( "r_bitdepth", 0 );
			VID_Printf( PRINT_ALL, "r_bitdepth requires Win95 OSR2.x or WinNT 4.x\n" );
		}
		r_bitdepth->modified = false;
	}

	if ( vr_enabled->value )
	{
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
	} 
	else if ( camera_separation < 0 && glState.stereo_enabled )
	{
		glDrawBuffer( GL_BACK_LEFT );
	}
	else if ( camera_separation > 0 && glState.stereo_enabled )
	{
		glDrawBuffer( GL_BACK_RIGHT );
	}
	else
	{
		glDrawBuffer( GL_BACK );
	}
}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
extern void R_VR_Present(void);
void GLimp_EndFrame (void)
{
	int		err;
	err = glGetError();
//	assert( err == GL_NO_ERROR );

	if (err != GL_NO_ERROR)	// Output error code instead
		VID_Printf (PRINT_DEVELOPER, "OpenGL Error %i\n", err);

	if (vr_enabled->value)
		R_VR_Present();

	if ( !SwapBuffers( glw_state.hDC ) )
		VID_Error (ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n");

	R_FrameFence();
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
	if ( active )
	{
		SetForegroundWindow( glw_state.hWnd );
		ShowWindow( glw_state.hWnd, SW_RESTORE );
	}
	else
	{
		if ( vid_fullscreen->value )
			ShowWindow( glw_state.hWnd, SW_MINIMIZE );
	}
}
