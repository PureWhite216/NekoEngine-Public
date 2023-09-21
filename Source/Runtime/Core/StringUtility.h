#pragma once
#include "Core.h"
namespace NekoEngine
{
    namespace StringUtility
    {
        template <typename T>
        static String ToString(const T& input)
        {
            return std::to_string(input);
        }

        String GetFilePathExtension(const String& FileName);
        String GetFileName(const String& FilePath);
        String GetFileLocation(const String& FilePath);

        String RemoveName(const String& FilePath);
        String RemoveFilePathExtension(const String& FileName);
        String ToLower(const String& text);

        String& BackSlashesToSlashes(String& string);
        String& SlashesToBackSlashes(String& string);
        String& RemoveSpaces(String& string);
        String& RemoveCharacter(String& string, const char character);

        String Demangle(const String& string);

        bool IsHiddenFile(const String& path);

        std::vector<String> SplitString(const String& string, const String& delimiters);
        std::vector<String> SplitString(const String& string, const char delimiter);
        std::vector<String> Tokenize(const String& string);
        std::vector<String> GetLines(const String& string);

        const char* FindToken(const char* str, const String& token);
        const char* FindToken(const String& string, const String& token);
        int32_t FindStringPosition(const String& string, const String& search, uint32_t offset = 0);
        String StringRange(const String& string, uint32_t start, uint32_t length);
        String RemoveStringRange(const String& string, uint32_t start, uint32_t length);

        String GetBlock(const char* str, const char** outPosition = nullptr);
        String GetBlock(const String& string, uint32_t offset = 0);

        String GetStatement(const char* str, const char** outPosition = nullptr);

        bool StringContains(const String& string, const String& chars);
        bool StartsWith(const String& string, const String& start);
        int32_t NextInt(const String& string);

        bool StringEquals(const String& string1, const String& string2);
        String StringReplace(String str, char ch1, char ch2);
        String StringReplace(String str, char ch);

        String BytesToString(uint64_t bytes);
    }
} // NekoEngine

