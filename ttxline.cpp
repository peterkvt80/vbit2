/** ***************************************************************************
 * Description       : Class for a single teletext line
 * Compiler          : C++
 *
 * Copyright (C) 2014, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *************************************************************************** **/

#include "ttxline.h"


TTXLine::TTXLine(std::string const& line, bool validateLine):
    m_textline(validate(line)),
    _nextLine(nullptr)

{
    if (!validateLine)
        m_textline=line;
}

TTXLine::TTXLine():m_textline("                                        "),
    _nextLine(nullptr)
{
}

TTXLine::~TTXLine()
{
    //std::cerr << "TTXLine dtor\n";
    if (_nextLine!=nullptr)
        delete _nextLine;
}

void TTXLine::Setm_textline(std::string const& val, bool validateLine)
{
    if (validateLine)
        m_textline = validate(val);
    else
        m_textline = val;
}

std::string TTXLine::validate(std::string const& val)
{
    char ch;
    int j=0;
    std::string str="                                        ";
    for (unsigned int i=0;i<val.length() && j<40;i++)
    {
        ch = val[i] & 0x7f; // 7-bit
        if (val[i] == 0x1b) // ascii escape
        {
            i++;
            ch = val[i] & 0x3f;
        }
        else if ((uint8_t)val[i] < 0x20) // other ascii control code
        {
            break;
        }

        if (ch < 0x20)
        {
            ch |= 0x80; // set high bit on control codes
        }

        str[j++]=ch;
    }
    return str;
}

bool TTXLine::IsBlank()
{
    if (m_textline.length()==0)
    {
        return true;
    }
    for (unsigned int i=0;i<m_textline.length();i++)
    {
        if (m_textline.at(i)!=' ')
        {
            return false;
        }
    }
    return true; // Yes, the line is blank
}

char TTXLine::GetCharAt(int xLoc)
{
    if (m_textline.length()<(uint16_t)xLoc)
    {
        // extend the line to 40 characters
        for (int i=xLoc;i<40;i++)
            m_textline+=" ";
    }
    return m_textline[xLoc];
}

std::string TTXLine::GetLine()
{
    // If the string is less than 40 characters we need to pad it or get weird render errors
    int len=m_textline.length();
    if (len>40)
    {
        return m_textline.substr(40);
    }
    if (len<40)
        for (int i=len;i<40;i++)
            m_textline+=" ";

    return m_textline;
}

void TTXLine::AppendLine(std::string  const& line)
{
    // Seek the end of the list
    TTXLine* p;
    for (p=this;p->_nextLine;p=p->_nextLine);
    p->_nextLine=new TTXLine(line,true);
}
