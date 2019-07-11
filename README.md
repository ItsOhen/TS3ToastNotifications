# TS3ToastNotifications

Changelog:
1.1:
	Added multiserver support
	Added toasts for channel kicks, server kicks, server bans and pokes
	Changed toast style and formatting. Now follows pattern: (channel) (user) (action)
	
You can either use the ts3_plugin installer by grabbing the .ts3_plugin file, or use the dll and manually put it in your teamspeak 3 plugins folder.
Or if you are a paranoid EVE player, as you should be, grab the Source code and build it yourself!

Note:
	Will only work on Windows 10 with Anniversary Update ( Build 14393 ) or newer.
	If you run full-screen application, windows game center may block your notification,
		to fix this issue, open windows settings and go into Focus assist, or open the start menu and search for Focus assist. and set that to Off ( Show all notifications )

Build notes:
	Requires: Windows 10 SDK, 10.0.17763.0 or newer
	Use preprocessor option DEBUG for verbose output in console. ( start TS3 with -console flag )
	If you build without the sln, make sure to include RuntimeObject.lib; with your linker.