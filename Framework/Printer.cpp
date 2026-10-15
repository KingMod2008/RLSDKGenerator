#include "Printer.hpp"
#include "../Engine/Engine.hpp"
#include <array>
#include <cstdio>

namespace Printer
{
    static constexpr uint32_t MAX_FILL_LENGTH = 1024;

    void Empty(std::ostringstream& stream)
    {
        stream.str(std::string());
    }

    void FillRight(std::ostringstream& stream, char fill, uint32_t width)
    {
        if (width > MAX_FILL_LENGTH)
        {
            width = MAX_FILL_LENGTH;
        }

        stream << std::setfill(fill) << std::setw(width) << std::right;
    }

    void FillLeft(std::ostringstream& stream, char fill, uint32_t width)
    {
        if (width > MAX_FILL_LENGTH)
        {
            width = MAX_FILL_LENGTH;
        }

        stream << std::setfill(fill) << std::setw(width) << std::left;
    }

    void FillRight(std::ofstream& stream, char fill, uint32_t width)
    {
        if (width > MAX_FILL_LENGTH)
        {
            width = MAX_FILL_LENGTH;
        }

        stream << std::setfill(fill) << std::setw(width) << std::right;
    }

    void FillLeft(std::ofstream& stream, char fill, uint32_t width)
    {
        if (width > MAX_FILL_LENGTH)
        {
            width = MAX_FILL_LENGTH;
        }

        stream << std::setfill(fill) << std::setw(width) << std::left;
    }

    void ReplaceChars(std::string& baseStr, char oldChar, char newChar)
    {
        for (char& c : baseStr)
        {
            if (c == oldChar)
            {
                c = newChar;
            }
        }
    }

    void ReplaceString(std::string& baseStr, const std::string& strToReplace, const std::string& replaceWithStr)
    {
        if (!baseStr.empty() && !strToReplace.empty())
        {
            size_t replaceLength = strToReplace.length();
            size_t replacePos = baseStr.find(strToReplace);

            while (replacePos != std::string::npos)
            {
                baseStr.replace(replacePos, strToReplace.length(), replaceWithStr);
                replacePos = baseStr.find(strToReplace);
            }
        }
    }

    std::string Hex(uintptr_t decimal, uint32_t width)
    {
        static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";

        uint32_t digitCount = 0;
        uintptr_t tmp = decimal;

        do
        {
            tmp >>= 4;
            digitCount++;
        } while (tmp);

        uint32_t totalDigits = (width > digitCount ? width : digitCount);
        std::string result;
        result.reserve(static_cast<size_t>(2 + totalDigits));
        result += "0x";

        for (uint32_t i = digitCount; i < totalDigits; ++i)
        {
            result += '0';
        }

        std::array<char, sizeof(uintptr_t) * 2> buffer{};
        for (uint32_t i = 0; i < digitCount; ++i)
        {
            uint32_t shift = (digitCount - 1 - i) * 4;
            uint32_t index = static_cast<uint32_t>((decimal >> shift) & 0xF);
            buffer[i] = HEX_DIGITS[index];
        }

        result.append(buffer.data(), digitCount);
        return result;
    }

    std::string Hex(uintptr_t decimal, EWidthTypes width)
    {
        return Hex(decimal, static_cast<uint32_t>(width));
    }

    std::string Hex(void* pointer)
    {
        return Hex(reinterpret_cast<uintptr_t>(pointer), sizeof(uintptr_t));
    }

    std::string Decimal(uintptr_t hex, uint32_t width)
    {
        std::array<char, 32> buffer{};
        int count = std::snprintf(buffer.data(), buffer.size(), "%llu", static_cast<unsigned long long>(hex));

        if (count <= 0)
        {
            return "0";
        }

        uint32_t digits = static_cast<uint32_t>(count);
        uint32_t totalDigits = (width > digits ? width : digits);

        std::string result;
        result.reserve(totalDigits);

        for (uint32_t i = digits; i < totalDigits; ++i)
        {
            result += '0';
        }

        result.append(buffer.data(), digits);
        return result;
    }

    std::string Decimal(uintptr_t hex, EWidthTypes width)
    {
        return Decimal(hex, static_cast<uint32_t>(width));
    }

    std::string Precision(float value, uint32_t precision)
    {
        std::ostringstream stream;
        stream << std::setprecision(precision) << value;
        return stream.str();
    }

    std::string ToUpper(std::string str)
    {
        if (!str.empty())
        {
            std::transform(str.begin(), str.end(), str.begin(), toupper);
        }

        return str;
    }

    std::string ToLower(std::string str)
    {
        if (!str.empty())
        {
            std::transform(str.begin(), str.end(), str.begin(), tolower);
        }

        return str;
    }

    void Header(std::ostringstream& stream, const std::string& fileName, const std::string& fileExtension, bool bPragmaPush)
    {
        stream << "/*\n";
        stream << "#############################################################################################\n";
        stream << "# " << GConfig::GetGameNameLong() << " (" << GConfig::GetGameNameShort() + ") SDK " << GConfig::GetGameVersion() << "\n";
        stream << "# Generated with the " << GEngine::GetName() << " " << GEngine::GetVersion() << "\n";
        stream << "# ========================================================================================= #\n";
        stream << "# File: " << fileName << "." << fileExtension << "\n";
        stream << "# ========================================================================================= #\n";
        stream << "# Credits: " << GEngine::GetCredits() << "\n";
        stream << "# Links: " << GEngine::GetLinks() << "\n";
        stream << "#############################################################################################\n";
        stream << "*/\n";

        if ((fileName != "SdkHeaders") && (fileName != "SdkConstants") && (fileName != "GameDefines"))
        {
            if (fileExtension == "hpp")
            {
                stream << "#pragma once\n";

                if (GConfig::UsingConstants())
                {
                    stream << "#include \"../../SDK_HEADERS/SdkConstants.hpp\"\n";
                }
            }
            else if (fileExtension == "cpp")
            {
                stream << "#include \"../../SDK_HEADERS/SdkHeaders.hpp\"\n";
            }
        }

        if (bPragmaPush)
        {
            stream << "\n#ifdef _MSC_VER\n";
            stream << "#pragma pack(push, " + Hex(GConfig::GetFinalAlignment(), 1) + ")\n";
            stream << "#endif\n";
        }
    }

    void Header(std::ofstream& stream, const std::string& fileName, const std::string& fileExtension, bool bPragmaPush)
    {
        std::ostringstream sStream;
        Header(sStream, fileName, fileExtension, bPragmaPush);
        stream << sStream.str();
    }

    void Section(std::ostringstream& stream, const std::string& sectionName)
    {
        stream << "\n/*\n";
        stream << "# ========================================================================================= #\n";
        stream << "# " << sectionName << "\n";
        stream << "# ========================================================================================= #\n";
        stream << "*/\n\n";
    }

    void Section(std::ofstream& stream, const std::string& sectionName)
    {
        std::ostringstream sStream;
        Section(sStream, sectionName);
        stream << sStream.str();
    }

    void Footer(std::ostringstream& stream, bool bPragmaPop)
    {
        stream << "/*\n";
        stream << "# ========================================================================================= #\n";
        stream << "#\n";
        stream << "# ========================================================================================= #\n";
        stream << "*/\n";

        if (bPragmaPop)
        {
            stream << "\n#ifdef _MSC_VER\n";
            stream << "#pragma pack(pop)\n";
            stream << "#endif\n";
        }
    }

    void Footer(std::ofstream& stream, bool bPragmaPop)
    {
        std::ostringstream sStream;
        Footer(sStream, bPragmaPop);
        stream << sStream.str();
    }
}
