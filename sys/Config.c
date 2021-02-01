#include "Driver.h"
#include "Config.tmh"

#define INI_USE_STACK		0
#define INI_MAX_LINE		1000
#define INI_ALLOW_REALLOC	1
#include "ini.h"

#pragma region https://gist.github.com/xebecnan/6d070c93fb69f40c3673

static wchar_t*
fromUTF8(
	const char* src,
	size_t src_length, /* = 0 */
	size_t* out_length /* = NULL */
)
{
	if (!src)
	{
		return NULL;
	}

	if (src_length == 0) { src_length = strlen(src); }
	int length = MultiByteToWideChar(CP_UTF8, 0, src, (int)src_length, 0, 0);
	wchar_t* output_buffer = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
	if (output_buffer)
	{
		MultiByteToWideChar(CP_UTF8, 0, src, (int)src_length, output_buffer, length);
		output_buffer[length] = L'\0';
	}
	if (out_length) { *out_length = length; }
	return output_buffer;
}

static char*
toUTF8(
	const wchar_t* src,
	size_t src_length, /* = 0 */
	size_t* out_length /* = NULL */
)
{
	if (!src)
	{
		return NULL;
	}

	if (src_length == 0) { src_length = wcslen(src); }
	int length = WideCharToMultiByte(CP_UTF8, 0, src, (int)src_length,
	                                 0, 0, NULL, NULL);
	char* output_buffer = (char*)malloc((length + 1) * sizeof(char));
	if (output_buffer)
	{
		WideCharToMultiByte(CP_UTF8, 0, src, (int)src_length,
		                    output_buffer, length, NULL, NULL);
		output_buffer[length] = '\0';
	}
	if (out_length) { *out_length = length; }
	return output_buffer;
}

#pragma endregion


static int inih_read_cfg_handler(void* user, const char* section, const char* name,
                                 const char* value)
{
	PDEVICE_CONTEXT pCtx = (PDEVICE_CONTEXT)user;

	size_t wideLen = 0, narrowLen = 0;
	PSTR instId = toUTF8(WdfMemoryGetBuffer(pCtx->InstanceId, &wideLen), wideLen, &narrowLen);

#define MATCH(n) (_stricmp(section, "Global") == 0 || _stricmp(section, instId) == 0) && _stricmp(name, n) == 0

	TraceVerbose(TRACE_CONFIG, "Checking for InstanceId \"%s\" vs section \"%s\"", instId, section);

	if (MATCH("HidDeviceMode"))
	{
		pCtx->Configuration.HidDeviceMode = (DS_HID_DEVICE_MODE)strtol(value, NULL, 10);
		TraceVerbose(TRACE_CONFIG, "Updating HidDeviceMode to 0x%04X", pCtx->Configuration.HidDeviceMode);
	}
	else if (MATCH("MuteDigitalPressureButtons"))
	{
		pCtx->Configuration.MuteDigitalPressureButtons = strtol(value, NULL, 10) > 0;
		TraceVerbose(TRACE_CONFIG, "Updating MuteDigitalPressureButtons to 0x%04X",
		         pCtx->Configuration.MuteDigitalPressureButtons);
	}
	else if (MATCH("DisableAutoPairing"))
	{
		pCtx->Configuration.DisableAutoPairing = strtol(value, NULL, 10) > 0;
		TraceVerbose(TRACE_CONFIG, "Updating DisableAutoPairing to 0x%04X", pCtx->Configuration.DisableAutoPairing);
	}
	else
	{
		free(instId);
		return 0; /* unknown section/name, error */
	}

	free(instId);
	return 1;
}

VOID DsConfig_Load(PDEVICE_CONTEXT Context)
{
	if (ini_parse(DS_DRIVER_CFG_FILE_PATH, inih_read_cfg_handler, Context) < 0)
	{
		TraceEvents(TRACE_LEVEL_WARNING,
		            TRACE_CONFIG,
		            "Failed to load configuration"
		);
	}
}
