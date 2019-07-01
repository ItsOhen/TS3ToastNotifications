/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) TeamSpeak Systems GmbH
 */
#include "stdafx.h"
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <inttypes.h>
#include <map>
#include <string>

#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 23

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

#define PLUGIN_NAME_W L"TS3_Toast_OnDisconnect"
#define PLUGIN_NAME "TS3_Toast_OnDisconnect"

#define PLUGIN_AUTHOR_W L"The one and only Ergon!"
#define PLUGIN_AUTHOR "The one and only Ergon!"

#define PLUGIN_DESC_W L"Send a Windows 10 \"Toast\" notification on user disconnect."
#define PLUGIN_DESC "Send a Windows 10 \"Toast\" notification on user disconnect."

#define PLUGIN_VERSION "1.1"

static char* pluginID = NULL;

#include "ToasterHelper.h"

// I got lazy and made the toaster a global aswell. Oh well..
Toaster* Toaster::_instance = 0;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = PLUGIN_NAME_W;
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = PLUGIN_NAME;  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return PLUGIN_NAME;
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return PLUGIN_VERSION;
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if (!result) {
		const wchar_t* name = PLUGIN_AUTHOR_W;
		if (wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = PLUGIN_AUTHOR;  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return PLUGIN_AUTHOR;
#endif
}

/* Plugin description */
const char* ts3plugin_description() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if (!result) {
		const wchar_t* name = PLUGIN_DESC_W;
		if (wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = PLUGIN_DESC;  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return PLUGIN_DESC;
#endif
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
	Toaster *toaster = Toaster::GetToaster();
	if(!SUCCEEDED(toaster->CheckShortcut()))
	{
		ts3Functions.logMessage("Unable to create a shortcut for some reason. Le fail!", LogLevel_CRITICAL, "Plugin", 0);
		return 1;
	}
	if(!SUCCEEDED(toaster->RegisterToaster()))
	{
		ts3Functions.logMessage("Unable to register with WRL for some reason. Le fail!", LogLevel_CRITICAL, "Plugin", 0);
		return 1;
	}

	return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    DPRINTF("%s: shutdown\n", PLUGIN_NAME);
	Toaster::GetToaster()->Shutdown();
}

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
	if (newStatus == STATUS_CONNECTION_ESTABLISHED) {
		/* Some example code following to show how to use the information query functions. */
		Toaster::GetToaster()->AddServer(serverConnectionHandlerID);
	}
	if (newStatus == STATUS_DISCONNECTED) {
		Toaster::GetToaster()->RemoveServer(serverConnectionHandlerID);
	}
}

void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {

}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
	anyID myClient = Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->GetClientID();
	uint64 myChannel = Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->GetCurrentChannel();
	if (newChannelID == myChannel || clientID == myClient)
	{
		if (clientID == myClient)
		{
			printf("You moved channel!\n");
			Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->SetCurrentChannel(newChannelID);
			Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->Shutdown();
			Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->AddClientsInChannel(newChannelID);
		}
		else
		{
			Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->AddClient(clientID);
		}
	}
	else if (oldChannelID == myChannel)
	{
		Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->RemoveClient(clientID);
	}
}

void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage) {
	if (!Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->GetNick(clientID).empty())
	{
		std::wstring nick = Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->GetNick(clientID);
		std::wstring channel;
		char *channelName;
		if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, oldChannelID, CHANNEL_NAME, &channelName) != ERROR_ok) {
			ts3Functions.logMessage("Error getting channel name", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		}
		else
		{
			channel = chartowstring(channelName);
			ts3Functions.freeMemory(channelName);
		}
		std::wstring msg = L"lost connection to the channel";

		Toaster::GetToaster()->SendToast(&channel, &nick, &msg);
		Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->RemoveClient(clientID);
	}
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
	printf("PLUGIN: onServerErrorEvent %llu %s %d %s\n", (long long unsigned int)serverConnectionHandlerID, errorMessage, error, (returnCode ? returnCode : ""));
	if(returnCode) {
		/* A plugin could now check the returnCode with previously (when calling a function) remembered returnCodes and react accordingly */
		/* In case of using a a plugin return code, the plugin can return:
		 * 0: Client will continue handling this error (print to chat tab)
		 * 1: Client will ignore this error, the plugin announces it has handled it */
		return 1;
	}
	return 0;  /* If no plugin return code was used, the return value of this function is ignored */
}

void ts3plugin_onConnectionInfoEvent(uint64 serverConnectionHandlerID, anyID clientID) {
	printf("ts3plugin_onConnectionInfoEvent: { clientID: %i } }\n", clientID);
}

void ts3plugin_onMessageListEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, uint64 timestamp, int flagRead) {
}

/* Called when client custom nickname changed */
void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* displayName, const char* uniqueClientIdentifier) {
	Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->EditClientName(clientID, displayName);
}


void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
	if (oldChannelID == Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->GetCurrentChannel())
	{
		std::wstring channel, nick;
		char *kicked_nick;
		char *channelName;

		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, clientID, CLIENT_NICKNAME, &kicked_nick) != ERROR_ok) {
			ts3Functions.logMessage("Error getting client nick", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		}
		else
		{
			nick = (chartowstring(kicked_nick));
			ts3Functions.freeMemory(kicked_nick);
		}
		if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, oldChannelID, CHANNEL_NAME, &channelName) != ERROR_ok) {
			ts3Functions.logMessage("Error getting channel name", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		}
		else
		{
			channel = chartowstring(channelName);
			ts3Functions.freeMemory(channelName);
		}
		char buf[1024] = { 0 };
		sprintf_s(buf, "was kicked from the channel. (%s) by %s", kickMessage, kickerName);
		std::wstring msg(chartowstring(buf));

		Toaster::GetToaster()->SendToast(&channel, &nick, &msg);
	}
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
	if (oldChannelID == Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->GetCurrentChannel())
	{
		std::wstring channel, nick;
		char *kicked_nick;
		char *channelName;

		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, clientID, CLIENT_NICKNAME, &kicked_nick) != ERROR_ok) {
			ts3Functions.logMessage("Error getting client nick", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		}
		else
		{
			nick = (chartowstring(kicked_nick));
			ts3Functions.freeMemory(kicked_nick);
		}
		if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, oldChannelID, CHANNEL_NAME, &channelName) != ERROR_ok) {
			ts3Functions.logMessage("Error getting channel name", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		}
		else
		{
			channel = chartowstring(channelName);
			ts3Functions.freeMemory(channelName);
		}
		char buf[1024] = { 0 };
		sprintf_s(buf, "was kicked from the server. (%s) by %s", kickMessage, kickerName);
		std::wstring msg(chartowstring(buf));

		Toaster::GetToaster()->SendToast(&channel, &nick, &msg);
	}
}

void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time, const char* kickMessage) {
	if (oldChannelID == Toaster::GetToaster()->GetServerHandle(serverConnectionHandlerID)->GetCurrentChannel())
	{
		std::wstring channel, nick;
		char *kicked_nick;
		char *channelName;

		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, clientID, CLIENT_NICKNAME, &kicked_nick) != ERROR_ok) {
			ts3Functions.logMessage("Error getting client nick", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		}
		else
		{
			nick = (chartowstring(kicked_nick));
			ts3Functions.freeMemory(kicked_nick);
		}
		if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, oldChannelID, CHANNEL_NAME, &channelName) != ERROR_ok) {
			ts3Functions.logMessage("Error getting channel name", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		}
		else
		{
			channel = chartowstring(channelName);
			ts3Functions.freeMemory(channelName);
		}
		char buf[1024] = { 0 };
		sprintf_s(buf, "was banned from the server. (%s) by %s", kickMessage, kickerName);
		std::wstring msg(chartowstring(buf));

		Toaster::GetToaster()->SendToast(&channel, &nick, &msg);
	}
}

int ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored) {
	std::wstring from(chartowstring(pokerName));
	std::wstring msg(chartowstring(message));
	std::wstring poked(L"");

	Toaster::GetToaster()->SendToast(&from, &msg, &poked);

	return 0;  /* 0 = handle normally, 1 = client will ignore the poke */
}
