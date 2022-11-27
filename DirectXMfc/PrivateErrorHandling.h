#pragma once

#include<exception>

#define P_WRITE_LOG(msg) PrivateErrorHandling::WriteLog(msg, __FILE__, __LINE__)
#define P_THROW_ERROR(msg) PrivateErrorHandling::ThrowError(msg, __FILE__, __LINE__)
#define P_IGNORE_ERROR(msg) PrivateErrorHandling::IgnoreError(msg, __FILE__, __LINE__)
#define P_C_FUNC_BEGIN(func_name) const char* _pFuncName = func_name; try {
#define P_C_FUNC_END } catch( ... ) {P_WRITE_LOG(_pFuncName);}

#define P_ASSERT(cond) ASSERT(cond)
#define P_IS_TRUE(cond) if (cond) {} else P_THROW_ERROR(#cond)

class PrivateErrorHandling
{
public:
	static void WriteLog(const char* pMsg, const char* pFile, int srcLine);
	static void ThrowError(const char* pMsg, const char* pFile, int srcLine);
	static void IgnoreError(const char* pMsg, const char* pFile, int srcLine);
};
