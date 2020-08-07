#pragma once
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <cassert>
#include <string>
#include <vector>
#include <windows.h>
#include <memory>

#define SAFE_DELETE(obj) {if(obj) {delete obj;} obj = nullptr; }

#define OUT_DEBUG_INFO
#define ASSERT(cnd)            { assert(cnd); }
#define DEBUG_MSG(s)           OutputDebugStringA(s)
#define WARNING_MSG(s)         OutputDebugStringA(s)
#define ERROR_MSG(s)           { OutputDebugStringA(s); ASSERT(false); }
#define ASSERT_MSG(cnd, s)     {if(!(cnd)) ERROR_MSG(s) }
std::string formatString(const char *fmt, ...);

std::vector<char> ReadFile(const std::string& filename);

bool IsAnyMaskState(uint32_t mask, uint32_t state);
bool IsEachMaskState(uint32_t mask, uint32_t state);
