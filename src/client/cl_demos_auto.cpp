// Copyright (C) 2015 ent (entdark)
//
// cl_demos_auto.cpp - autorecording demos routine
//
// credits: 
// - teh aka dumbledore aka teh_1337: autorecording demos
// - sil: saving LastDemo
// - CaNaBiS: formatting with %

#include "client.h"

cvar_t	*cl_autoDemo;
cvar_t	*cl_autoDemoFormat;

#define MAX_TIMESTAMPS 256
#define DEFAULT_NAME "LastDemo/LastDemo_recording"
#define DEFAULT_NAME_LAST "LastDemo/LastDemo"

static struct {
	char				demoName[MAX_OSPATH];
	char				customName[MAX_QPATH];
	int					timeStamps[MAX_TIMESTAMPS];
	char				mod[MAX_QPATH];
	char				ext[16];
} demoAuto;

char *demoAutoFormat(const char* name) {	
	const	char *format;
	qboolean haveTag = qfalse;
	static char	outBuf[512];
	int			outIndex = 0;
	int			outLeft = sizeof(outBuf) - 1;
	
	int t = 0;
	char timeStamps[MAX_QPATH] = "";
	qtime_t ct;

	char playerName[MAX_QPATH], *mapName = COM_SkipPath(Info_ValueForKey((cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO]), "mapname"));
	Q_strncpyz(playerName, Info_ValueForKey((cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS+cl.snap.ps.clientNum]), "n"), sizeof(playerName));
	Q_CleanStr(playerName, qtrue);
	Com_RealTime(&ct);
	
	format = cl_autoDemoFormat->string;
	if (!format || !format[0]) {
		if (!name || !name[0]) {
			format = "%t";
		} else {
			format = "%n_%t";
		}
	}

	while (*format && outLeft  > 0) {
		if (haveTag) {
			char ch = *format++;
			haveTag = qfalse;
			switch (ch) {
			case 'd':		//date
				Com_sprintf( outBuf + outIndex, outLeft, "%d-%02d-%02d-%02d%02d%02d",
								1900+ct.tm_year, ct.tm_mon+1,ct.tm_mday,
								ct.tm_hour, ct.tm_min, ct.tm_sec);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 'm':		//map
				Com_sprintf( outBuf + outIndex, outLeft, "%s", mapName);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 'n':		//custom demo name
				Com_sprintf( outBuf + outIndex, outLeft, "%s", name);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 'p':		//current player name
				Com_sprintf( outBuf + outIndex, outLeft, "%s", playerName);
				outIndex += strlen( outBuf + outIndex );
				break;
			case 't':		//timestamp
				while (demoAuto.timeStamps[t] && t < MAX_TIMESTAMPS) {
					int min = demoAuto.timeStamps[t] / 60000;
					int sec = (demoAuto.timeStamps[t] / 1000) % 60;
					if (t == 0) {
						Com_sprintf(timeStamps, sizeof(timeStamps), "%02d%02d", min, sec);
					} else {
						Com_sprintf(timeStamps, sizeof(timeStamps), "%s_%02d%02d", timeStamps, min, sec);
					}
					t++;
				}
				Com_sprintf( outBuf + outIndex, outLeft, "%s", timeStamps);
				outIndex += strlen( outBuf + outIndex );
				break;
			case '%':
				outBuf[outIndex++] = '%';
				break;
			default:
				continue;
			}
			outLeft = sizeof(outBuf) - outIndex - 1;
			continue;
		}
		if (*format == '%') {
			haveTag = qtrue;
			format++;
			continue;
		}
		outBuf[outIndex++] = *format++;
		outLeft = sizeof(outBuf) - outIndex - 1;
	}
	outBuf[ outIndex ] = 0;
	return outBuf;
}

// Standard naming for screenshots/demos
static char *demoAutoGenerateDefaultFilename(void) {
	qtime_t ct;
	const char *pszServerInfo = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	
	Com_RealTime(&ct);
	return va("%d-%02d-%02d-%02d%02d%02d-%s",
								1900+ct.tm_year, ct.tm_mon+1,ct.tm_mday,
								ct.tm_hour, ct.tm_min, ct.tm_sec,
								COM_SkipPath(Info_ValueForKey(pszServerInfo, "mapname")));
}

void demoAutoSave_f(void) {
	int t = 0;

	if (cls.state != CA_ACTIVE) {
		Com_Printf ("You must be in a level to save the demo.\n");
		return;
	}

	if (strstr(cl_autoDemoFormat->string, "%t"))
		while (demoAuto.timeStamps[t] && t < MAX_TIMESTAMPS)
			t++;
	demoAuto.timeStamps[t] = cl.serverTime - atoi(cl.gameState.stringData + cl.gameState.stringOffsets[CS_LEVEL_START_TIME]);

	if (!(Cmd_Argc() < 2)) {
		Q_strncpyz(demoAuto.customName, Cmd_Argv( 1 ), sizeof(demoAuto.customName));
	}
	Com_sprintf(demoAuto.demoName, sizeof(demoAuto.demoName), "%s", demoAutoFormat(demoAuto.customName));
	Com_Printf(S_COLOR_WHITE "Demo will be saved into " S_COLOR_GREEN "%s%s\n", demoAuto.demoName, demoAuto.ext);
}

void demoAutoSaveLast_f(void) {
	if (Cmd_Argc() < 2
		&& FS_CopyFile(
			va("%s/demos/%s%s", demoAuto.mod, DEFAULT_NAME_LAST, demoAuto.ext),
			va("%s/demos/%s%s", demoAuto.mod, demoAutoGenerateDefaultFilename(), demoAuto.ext)
		)) {
		Com_Printf(S_COLOR_GREEN "LastDemo successfully saved\n");
	} else if (
		FS_CopyFile(
			va("%s/demos/%s%s", demoAuto.mod, DEFAULT_NAME_LAST, demoAuto.ext),
			va("%s/demos/%s%s", demoAuto.mod, Cmd_Argv( 1 ), demoAuto.ext)
		)) {
		Com_Printf(S_COLOR_GREEN "LastDemo successfully saved into %s%s\n", Cmd_Argv( 1 ), demoAuto.ext);
	} else {
		Com_Printf(S_COLOR_RED "LastDemo has failed to save\n");
	}
}

extern void CL_StopRecord_f( void );
void demoAutoComplete(void) {
	char newName[MAX_OSPATH];
	CL_StopRecord_f();
	//if we are not manually saving, then temporarily store a demo in LastDemo folder
	if (!demoAuto.demoName[0]
		&& FS_CopyFile(
			va("%s/demos/%s%s", demoAuto.mod, DEFAULT_NAME, demoAuto.ext),
			va("%s/demos/%s%s", demoAuto.mod, DEFAULT_NAME_LAST, demoAuto.ext)
		)) {
		Com_Printf(S_COLOR_GREEN "Demo temporarily saved into %s%s\n", DEFAULT_NAME, demoAuto.ext);
	} else if (
		FS_CopyFile(
			va("%s/demos/%s%s", demoAuto.mod, DEFAULT_NAME, demoAuto.ext),
			va("%s/demos/%s%s", demoAuto.mod, demoAuto.demoName, demoAuto.ext), newName, sizeof(newName)
		)) {
		Com_Printf(S_COLOR_GREEN "Demo successfully saved into %s\n", ((Q_stricmp(newName, "")) ? newName : demoAuto.demoName));
	} else {
		Com_Printf(S_COLOR_RED "Demo has failed to save\n");
	}
}

// Dynamically names a demo and sets up the recording
void demoAutoRecord(void) {
	//mod resetting allowed in init only
	memset(&demoAuto, 0, sizeof(demoAuto)-sizeof(demoAuto.mod));
	Cbuf_AddText(va("record %s\n", DEFAULT_NAME));
}

void demoAutoInit(void) {
	cvar_t *fs_game;
	memset(&demoAuto, 0, sizeof(demoAuto));
	fs_game = Cvar_FindVar ("fs_game" );
	if (fs_game && (Q_stricmp(fs_game->string, ""))) {
		Q_strncpyz(demoAuto.mod, fs_game->string, sizeof(demoAuto.mod));
	} else {
		Q_strncpyz(demoAuto.mod, "base", sizeof(demoAuto.mod));
	}

	if ( clc.mvNetProtocol ) {
		Com_sprintf(demoAuto.ext, sizeof(demoAuto.ext), ".dm_mv1");
	} else {
		Com_sprintf(demoAuto.ext, sizeof(demoAuto.ext), ".dm_%d", MV_GetCurrentProtocol());
	}
}
