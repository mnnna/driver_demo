#include "Logger.h"


void Log(const char* sz_info, bool is_error, ULONG err_code)
{
	if (is_error) DbgPrintEx(77, 0, "[instCallBack Error]: %s    Err_code: %x\n", sz_info, err_code);
	else DbgPrintEx(77, 0, "[instCallBack Error]: %s\n", sz_info);
}
