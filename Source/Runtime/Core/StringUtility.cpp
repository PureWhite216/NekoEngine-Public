#include "StringUtility.h"
#include <cctype>
#include <windows.h>
#include <DbgHelp.h>
#include <iomanip>
#include <sstream>

namespace NekoEngine
{
    namespace StringUtility
    {
        String GetFilePathExtension(const String& FileName)
        {
            auto pos = FileName.find_last_of('.');
            if(pos != String::npos)
                return FileName.substr(pos + 1);
            return "";
        }

        String RemoveFilePathExtension(const String& FileName)
        {
            auto pos = FileName.find_last_of('.');
            if(pos != String::npos)
                return FileName.substr(0, pos);
            return FileName;
        }

        String GetFileName(const String& FilePath)
        {
            auto pos = FilePath.find_last_of('/');
            if(pos != String::npos)
                return FilePath.substr(pos + 1);

            pos = FilePath.find_last_of("\\");
            if(pos != String::npos)
                return FilePath.substr(pos + 1);

            return FilePath;
        }

        String ToLower(const String& text)
        {
            String lowerText = text;
            transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
            return lowerText;
        }

        String GetFileLocation(const String& FilePath)
        {
            auto pos = FilePath.find_last_of('/');
            if(pos != String::npos)
                return FilePath.substr(0, pos + 1);

            pos = FilePath.find_last_of("\\");
            if(pos != String::npos)
                return FilePath.substr(0, pos + 1);

            return FilePath;
        }

        String RemoveName(const String& FilePath)
        {
            auto pos = FilePath.find_last_of('/');
            if(pos != String::npos)
                return FilePath.substr(0, pos + 1);

            pos = FilePath.find_last_of("\\");
            if(pos != String::npos)
                return FilePath.substr(0, pos + 1);

            return FilePath;
        }

        bool IsHiddenFile(const String& path)
        {
            if(path != ".." && path != "." && path[0] == '.')
            {
                return true;
            }

            return false;
        }

        std::vector<String> SplitString(const String& string, const String& delimiters)
        {
            size_t start = 0;
            size_t end   = string.find_first_of(delimiters);

            std::vector<String> result;

            while(end <= String::npos)
            {
                String token = string.substr(start, end - start);
                if(!token.empty())
                    result.push_back(token);

                if(end == String::npos)
                    break;

                start = end + 1;
                end   = string.find_first_of(delimiters, start);
            }

            return result;
        }

        std::vector<String> SplitString(const String& string, const char delimiter)
        {
            return SplitString(string, String(1, delimiter));
        }

        std::vector<String> Tokenize(const String& string)
        {
            return SplitString(string, " \t\n");
        }

        std::vector<String> GetLines(const String& string)
        {
            return SplitString(string, "\n");
        }

        const char* FindToken(const char* str, const String& token)
        {
            const char* t = str;
            while((t = strstr(t, token.c_str())))
            {
                bool left  = str == t || isspace(t[-1]);
                bool right = !t[token.size()] || isspace(t[token.size()]);
                if(left && right)
                    return t;

                t += token.size();
            }
            return nullptr;
        }

        const char* FindToken(const String& string, const String& token)
        {
            return FindToken(string.c_str(), token);
        }

        int32_t FindStringPosition(const String& string, const String& search, uint32_t offset)
        {
            const char* str   = string.c_str() + offset;
            const char* found = strstr(str, search.c_str());
            if(found == nullptr)
                return -1;
            return (int32_t)(found - str) + offset;
        }

        String StringRange(const String& string, uint32_t start, uint32_t length)
        {
            return string.substr(start, length);
        }

        String RemoveStringRange(const String& string, uint32_t start, uint32_t length)
        {
            String result = string;
            return result.erase(start, length);
        }

        String GetBlock(const char* str, const char** outPosition)
        {
            const char* end = strstr(str, "}");
            if(!end)
                return String(str);

            if(outPosition)
                *outPosition = end;
            const uint32_t length = static_cast<uint32_t>(end - str + 1);
            return String(str, length);
        }

        String GetBlock(const String& string, uint32_t offset)
        {
            const char* str = string.c_str() + offset;
            return GetBlock(str);
        }

        String GetStatement(const char* str, const char** outPosition)
        {
            const char* end = strstr(str, ";");
            if(!end)
                return String(str);

            if(outPosition)
                *outPosition = end;
            const uint32_t length = static_cast<uint32_t>(end - str + 1);
            return String(str, length);
        }

        bool StringContains(const String& string, const String& chars)
        {
            return string.find(chars) != String::npos;
        }

        bool StartsWith(const String& string, const String& start)
        {
            return string.find(start) == 0;
        }

        int32_t NextInt(const String& string)
        {
            for(uint32_t i = 0; i < string.size(); i++)
            {
                if(isdigit(string[i]))
                    return atoi(&string[i]);
            }
            return -1;
        }

        bool StringEquals(const String& string1, const String& string2)
        {
            return strcmp(string1.c_str(), string2.c_str()) == 0;
        }

        String StringReplace(String str, char ch1, char ch2)
        {
            for(int i = 0; i < str.length(); ++i)
            {
                if(str[i] == ch1)
                    str[i] = ch2;
            }

            return str;
        }

        String StringReplace(String str, char ch)
        {
            for(int i = 0; i < str.length(); ++i)
            {
                if(str[i] == ch)
                {
                    str = String(str).substr(0, i) + String(str).substr(i + 1, str.length());
                }
            }

            return str;
        }

        String& BackSlashesToSlashes(String& string)
        {
            size_t len = string.length();
            for(size_t i = 0; i < len; i++)
            {
                if(string[i] == '\\')
                {
                    string[i] = '/';
                }
            }
            return string;
        }

        String& SlashesToBackSlashes(String& string)
        {
            size_t len = string.length();
            for(size_t i = 0; i < len; i++)
            {
                if(string[i] == '/')
                {
                    string[i] = '\\';
                }
            }
            return string;
        }

        String& RemoveSpaces(String& string)
        {
            String::iterator endIterator = std::remove(string.begin(), string.end(), ' ');
            string.erase(endIterator, string.end());
            string.erase(std::remove_if(string.begin(),
                                        string.end(),
                                        [](unsigned char x)
                                        {
                                            return isspace(x);
                                        }),
                         string.end());

            return string;
        }

        String& RemoveCharacter(String& string, const char character)
        {
            String::iterator endIterator = std::remove(string.begin(), string.end(), character);
            string.erase(endIterator, string.end());
            string.erase(std::remove_if(string.begin(),
                                        string.end(),
                                        [](unsigned char x)
                                        {
                                            return isspace(x);
                                        }),
                         string.end());

            return string;
        }

        String Demangle(const String& string)
        {
            if(string.empty())
                return {};

            char undecorated_name[1024] = {};
            if(!UnDecorateSymbolName(
                   string.c_str(), undecorated_name, sizeof(undecorated_name),
                   UNDNAME_COMPLETE))
            {
                return string;
            }
            else
            {
                return String(undecorated_name);
            }
        }

        String BytesToString(uint64_t bytes)
        {
            static const float gb = 1024 * 1024 * 1024;
            static const float mb = 1024 * 1024;
            static const float kb = 1024;

            std::stringstream result;
            if(bytes > gb)
                result << std::fixed << std::setprecision(2) << std::to_string((float)bytes / gb) << " gb";
            else if(bytes > mb)
                result << std::fixed << std::setprecision(2) << std::to_string((float)bytes / mb) << " mb";
            else if(bytes > kb)
                result << std::fixed << std::setprecision(2) << std::to_string((float)bytes / kb) << " kb";
            else
                result << std::fixed << std::setprecision(2) << std::to_string((float)bytes) << " bytes";

            return result.str();
        }
    }
} // NekoEngine