#pragma once
#include <string>
#include <memory>
#include <Windows.h>
#include <windows.ui.notifications.h>
#include <wrl.h>
#include <NotificationActivationCallback.h>
#include <Psapi.h>
#include <ShObjIdl.h>
#include <propvarutil.h>
#include <ShlObj_core.h>
#include <propkey.h>

using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace Microsoft::WRL;

#define GUUID_CLSID "90813707-B56D-47EF-A033-6E1CED1AEAA8"
#define AUMID L"TS3.TS3ToastHelper"

// The UUID CLSID must be unique to your app. Create a new GUID if copying this code.
class DECLSPEC_UUID(GUUID_CLSID) NotificationActivator WrlSealed WrlFinal
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, INotificationActivationCallback>
{
public:
	virtual HRESULT STDMETHODCALLTYPE Activate(
		_In_ LPCWSTR appUserModelId,
		_In_ LPCWSTR invokedArgs,
		_In_reads_(dataCount) const NOTIFICATION_USER_INPUT_DATA* data,
		ULONG dataCount) override
	{
		// TODO: Handle activation
		return S_OK;
	}
};