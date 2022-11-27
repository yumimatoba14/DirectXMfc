#include "pch.h"
#include "PrivateErrorHandling.h"

void PrivateErrorHandling::WriteLog(const char* pMsg, const char* pFile, int srcLine)
{
}

void PrivateErrorHandling::ThrowError(const char* pMsg, const char* pFile, int srcLine)
{
	WriteLog(pMsg, pFile, srcLine);
	throw new std::exception(pMsg);
}

void PrivateErrorHandling::IgnoreError(const char* pMsg, const char* pFile, int srcLine)
{
	WriteLog(pMsg, pFile, srcLine);
}
