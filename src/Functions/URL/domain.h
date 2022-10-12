#pragma once

#include "protocol.h"
#include <base/find_symbols.h>
#include <cstring>
#include <Common/StringUtils/StringUtils.h>

namespace DB
{

inline std::string_view checkAndReturnHost(const Pos & pos, const Pos & dot_pos, const Pos & start_of_host)
{
    if (!dot_pos || start_of_host >= pos || pos - dot_pos == 1)
        return std::string_view{};

    auto after_dot = *(dot_pos + 1);
    if (after_dot == ':' || after_dot == '/' || after_dot == '?' || after_dot == '#')
        return std::string_view{};

    return std::string_view(start_of_host, pos - start_of_host);
}

/// Extracts host from given url.
///
/// @return empty string view if the host is not valid (i.e. it does not have dot, or there no symbol after dot).
inline std::string_view getURLHost(const char * data, size_t size)
{
    Pos pos = data;
    Pos end = data + size;

    if (*pos == '/' && *(pos + 1) == '/')
    {
        pos += 2;
    }
    else
    {
        Pos scheme_end = data + std::min(size, 16UL);
        for (++pos; pos < scheme_end; ++pos)
        {
            if (!isAlphaNumericASCII(*pos))
            {
                switch (*pos)
                {
                case '.':
                case '-':
                case '+':
                    break;
                case ' ': /// restricted symbols
                case '\t':
                case '<':
                case '>':
                case '%':
                case '{':
                case '}':
                case '|':
                case '\\':
                case '^':
                case '~':
                case '[':
                case ']':
                case ';':
                case '=':
                case '&':
                    return std::string_view{};
                default:
                    goto exloop;
                }
            }
        }
exloop: if ((scheme_end - pos) > 2 && *pos == ':' && *(pos + 1) == '/' && *(pos + 2) == '/')
            pos += 3;
        else
            pos = data;
    }

    Pos dot_pos = nullptr;
    Pos colon_pos = nullptr;
    bool exist_at = false;
    const auto * start_of_host = pos;
    for (; pos < end; ++pos)
    {
        switch (*pos)
        {
        case '.':
            if (exist_at || colon_pos == nullptr)
                dot_pos = pos;
            break;
        case ':':
            if (exist_at) goto done;
            colon_pos = colon_pos ? colon_pos : pos;
            break;
        case '/':
        case '?':
        case '#':
            goto done;
        case '@': /// myemail@gmail.com
            exist_at = true;
            start_of_host = pos + 1;
            break;
        case ' ': /// restricted symbols in whole URL
        case '\t':
        case '<':
        case '>':
        case '%':
        case '{':
        case '}':
        case '|':
        case '\\':
        case '^':
        case '~':
        case '[':
        case ']':
        case ';':
        case '=':
        case '&':
            return std::string_view{};
        }
    }

done:
    if (!exist_at)
        pos = colon_pos ? colon_pos : pos;
    return checkAndReturnHost(pos, dot_pos, start_of_host);
}

template <bool without_www>
struct ExtractDomain
{
    static size_t getReserveLengthForElement() { return 15; }

    static void execute(Pos data, size_t size, Pos & res_data, size_t & res_size)
    {
        std::string_view host = getURLHost(data, size);

        if (host.empty())
        {
            res_data = data;
            res_size = 0;
        }
        else
        {
            if (without_www && host.size() > 4 && !strncmp(host.data(), "www.", 4))
                host = { host.data() + 4, host.size() - 4 };

            res_data = host.data();
            res_size = host.size();
        }
    }
};

}
