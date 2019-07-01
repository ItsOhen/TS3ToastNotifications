#pragma once
#include "stdafx.h"
/* Toast helper class. Becouse apparantly you need to have a installed program or a shortcut on the start menu for toasts to work.. Great design! */
#include "DesktopNotificationManagerCompat.h"
#include "ts3_functions.h"
// Flag class as COM creatable
CoCreatableClass(NotificationActivator);

#include <vector>
#define ARR_FOR_I(x) for (size_t i = 0; x[i]; i++)
#ifdef DEBUG
#define DPRINTF(x, ...) wprintf(x, ...)
#else
#define DPRINTF(x, ...)
#endif


std::wstring chartowstring(const char* str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)strlen(str), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)strlen(str), &wstrTo[0], size_needed);
	return wstrTo;
}

class Server
{
public:
	Server() {};
	Server(uint64 serverID, uint64 channelID, anyID clientID) : _server(serverID), _channel(channelID), _client(clientID) {};
	~Server() {};

	uint64 GetCurrentChannel() { return _channel; }
	void SetCurrentChannel(uint64 channelID) { _channel = channelID; }
	anyID GetClientID() { return _client; }
	void SetClientID(unsigned short clientID) { _client = clientID; }
	uint64 GetServerID() { return _server; }
	uint64 GetServerID() const { return _server; }
	void SetServerID(uint64 server) { _server = server; }

	void AddClientsInChannel(uint64 channelID)
	{
		anyID *results;
		DPRINTF("Getting client list.\n");
		if (ts3Functions.getChannelClientList(_server, channelID, &results) != ERROR_ok) {
			ts3Functions.logMessage("Error getting channel list", LogLevel_ERROR, "Plugin", _server);
			return;
		}
		else
		{
			DPRINTF("Getting names for clients.\n");
			ARR_FOR_I(results)
			{
				AddClient(results[i]);
			}
		}
	}

	void RemoveClient(anyID client)
	{
		DPRINTF(L"Removing client: %i:%ws\n", client, _names[client].c_str());
		_names.erase(client);
	}
	void AddClient(anyID client)
	{
		char* nick;
		if (ts3Functions.getClientVariableAsString(_server, client, CLIENT_NICKNAME, &nick) != ERROR_ok) {
			ts3Functions.logMessage("Error getting client nick", LogLevel_ERROR, "Plugin", _server);
		}
		else
		{
			DPRINTF("Adding client: %i:%s\n", client, nick);
			// God damn i hate unicode..
			_names.insert({ client, std::wstring(chartowstring(nick)) });

			ts3Functions.freeMemory(nick);
		}
	}
	void EditClientName(anyID client, const char* nick)
	{
		// God damn i hate unicode..
		_names[client] = chartowstring(nick);
	}
	void Shutdown()
	{
		DPRINTF("Closing and cleaning.\n");
		for (auto nick : _names)
		{
			wprintf(L"%i : %ws\n", nick.first, nick.second.c_str());
		}
		_names.clear();
	}
	std::wstring GetNick(anyID clientID) { 
		if (_names.find(clientID) != _names.end()) 
			return _names[clientID]; 
		return std::wstring();
	}

private:
	uint64 _server, _channel;
	anyID _client;
	std::map<anyID, std::wstring> _names;
};

class Toaster
{
public:
	~Toaster() {};
	static Toaster* GetToaster() { if (_instance == 0) _instance = new Toaster(); return _instance; };
	HRESULT CheckShortcut();
	HRESULT CreateShortcut(const WCHAR* path);
	HRESULT GetRegistryInfo();
	HRESULT RegisterToaster();
	void SendToast(std::wstring *channel, std::wstring *nick, std::wstring *msg);

	void Shutdown()
	{
		_connections.clear();
	}

	void AddServer(uint64 server)
	{
		anyID myID;
		uint64 myChannelID;

		/* Get own clientID */
		if (ts3Functions.getClientID(server, &myID) != ERROR_ok) {
			ts3Functions.logMessage("Error querying client ID", LogLevel_ERROR, "Plugin", server);
			return;
		}
		if (ts3Functions.getChannelOfClient(server, myID, &myChannelID) != ERROR_ok) {
			ts3Functions.logMessage("Error querying channel ID", LogLevel_ERROR, "Plugin", server);
			return;
		}
		_connections.push_back(Server(server, myChannelID,myID));
		_connections.back().AddClientsInChannel(myChannelID);
	}
	void RemoveServer(uint64 server)
	{
		for (auto i = _connections.begin(); i != _connections.end(); ++i)
		{
			if (i->GetServerID() == server)
			{
				i->Shutdown();
				_connections.erase(i);
				break;
			}
		}
	}

	Server* GetServerHandle(uint64 server) {
		for (auto &s : _connections)
		{
			if (s.GetServerID() == server)
				return &s;
		}
		AddServer(server);
		return &_connections.back();
	}

private:
	WCHAR exePath[MAX_PATH];
	static Toaster* _instance;
	Toaster() { CoInitialize(nullptr); };
	std::vector<Server> _connections;
};

HRESULT Toaster::CheckShortcut()
{
	//const WCHAR path[MAX_PATH] = L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\TeamSpeak 3 Toaster Helper.lnk";
	wchar_t shortcutPath[MAX_PATH];
	DWORD charWritten = GetEnvironmentVariable(L"APPDATA", shortcutPath, MAX_PATH);
	HRESULT hr = charWritten > 0 ? S_OK : E_INVALIDARG;
	if (SUCCEEDED(hr))
	{
		errno_t concatError = wcscat_s(shortcutPath, ARRAYSIZE(shortcutPath), L"\\Microsoft\\Windows\\Start Menu\\Programs\\TeamSpeak 3 Toaster Helper.lnk");
		hr = concatError == 0 ? S_OK : E_INVALIDARG;
		if (SUCCEEDED(hr))
		{
			DWORD attributes = GetFileAttributes(shortcutPath);
			bool fileExists = attributes < 0xFFFFFFF;
			if (!fileExists)
			{
				DPRINTF("Shortcut doesn't exist, creating new.\n");
				return CreateShortcut(shortcutPath);
			}
		}
	}
	return S_OK;
}

HRESULT Toaster::CreateShortcut(const WCHAR* path)
{
	HRESULT hr = GetRegistryInfo();
	if (!SUCCEEDED(hr))
	{
		DPRINTF("Failed to read registry\n");
		return E_FAIL;
	}
	ComPtr<IShellLink> shellLink;
	hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
	if (SUCCEEDED(hr))
	{
		shellLink->SetPath(exePath);
		shellLink->SetArguments(L"");
		ComPtr<IPropertyStore> propertyStore;
		hr = shellLink.As(&propertyStore);
		PROPVARIANT appIdPropVar;
		InitPropVariantFromString(AUMID, &appIdPropVar);
		propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
		hr = propertyStore->Commit();
		if (SUCCEEDED(hr))
		{
			DPRINTF("Commit worked.\n");
			ComPtr<IPersistFile> persistFile;
			hr = shellLink.As(&persistFile);
			DPRINTF("EXE path is: %ws.\n", exePath);
			shellLink->SetIconLocation(exePath, 0);
			hr = persistFile->Save(path, TRUE);
			if (!SUCCEEDED(hr))
			{
				printf("Persist file save failed!\n");
			}
		}
		PropVariantClear(&appIdPropVar);
	}
	return hr;
}

HRESULT Toaster::GetRegistryInfo()
{
	HKEY pKey;
	WCHAR path[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\TeamSpeak 3 Client";

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE, &pKey) != ERROR_SUCCESS)
	{
		return E_FAIL;
	}
	else
	{
		DWORD value_length = MAX_PATH;
		RegQueryValueEx(pKey, L"DisplayIcon", NULL, NULL, (LPBYTE)&exePath, &value_length);
		// Ugly hack to strip unwanted parts.. But it works!
		exePath[wcslen(exePath) - 2] = '\0';
	}
	return S_OK;
}

HRESULT Toaster::RegisterToaster()
{
	HRESULT hr;
	hr = DesktopNotificationManagerCompat::RegisterAumidAndComServer(AUMID, __uuidof(NotificationActivator));
	if (!SUCCEEDED(hr))
		return hr;
	hr = DesktopNotificationManagerCompat::RegisterActivator();
	if (!SUCCEEDED(hr))
		return hr;

	// Reduntant, i know, but i'm thinking about adding notification callbacks, they would go here aswell. So just let the optimizer deal with this for now.
	return S_OK;
}

void Toaster::SendToast(std::wstring *channel, std::wstring *nick, std::wstring *msg)
{
	ComPtr<IXmlDocument> doc;
	// I was originally thinking of using the IXMLDom framework already in place. But it's a fucking bitch to work with. So here! Fancy solutions!
	// NEVER FORGET TO INITIALIZE! THIS FUCKING THING RIGHT HERE TOOK ME LIKE 30MIN TO FIND!
	WCHAR buf[1024] = { 0 };
	wsprintf(buf, L"<toast><visual><binding template = 'ToastGeneric'><text>%ws</text><text>%ws</text><text>%ws</text></binding></visual><audio silent = \"true\"/></toast>", channel->c_str(), nick->c_str(), msg->c_str());
	

	DPRINTF(L"%ws", buf);

	HRESULT hr = DesktopNotificationManagerCompat::CreateXmlDocumentFromString(buf, &doc);

	if (SUCCEEDED(hr))
	{
		ComPtr<IToastNotifier> notifier;
		hr = DesktopNotificationManagerCompat::CreateToastNotifier(&notifier);
		if (SUCCEEDED(hr))
		{
			ComPtr<IToastNotification> toast;
			hr = DesktopNotificationManagerCompat::CreateToastNotification(doc.Get(), &toast);
			if (SUCCEEDED(hr))
			{
				hr = notifier->Show(toast.Get());
			}
		}
	}
}