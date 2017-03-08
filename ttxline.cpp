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
	if (_nextLine)
		delete _nextLine;
}

/** Set m_textline
 * \param val New value to set
 */
void TTXLine::Setm_textline(std::string const& val, bool validateLine)
{
	if (validateLine)
    m_textline = validate(val);
	else
    m_textline = val;
}

/** Validate - given a string, it validates it to ensure that the
 *  string follows the required format. 7 bit transmission ready.
 * It de-escapes and/or remaps characters as detailed in the MRG tti format spec.
 * It also treats \r very carefully.
 * If the string ends in \r or \r\n then it is treated as a terminator and is discarded.
 */
std::string TTXLine::validate(std::string const& val)
{
    char ch;
    int j=0;
    std::string str="                                       ";
    str.resize(80);
    // std::cout << "Validating length= " << val.length() << std::endl;
    for (unsigned int i=0;i<val.length() && i<80;i++)
    {
        ch=val[i] & 0x7f;   // Convert to 7 bits
        if (ch==0x1b) // escape?
        {
            i++;
            ch=val[i] & 0x3f;
        }
        // std::cout << val[i] << std::endl;

        // If we use null terminated strings anywhere it will go wrong.
        // add the parity bit to nulls now so that they make it through the string handling
        if (ch==0x00) // null?
        {
            ch=0x80; // Black text.
        }
        str[j++]=ch;
    }
    // short line? Remove the text terminator.
    if (str[j-1]=='\n') j--;
    if (str[j-1]=='\r') j--;
    str.resize(j);
    // std::cout << "Validating done " << std::endl;
    return str;
}

/** GetMappedLine - returns a string with text file-safe mappings applied.
 * It is more or less the reverse of validate.
 * It escapes characters as detailed in the MRG tti format spec.
 */
std::string TTXLine::GetMappedline()
{
    char ch;
    int j=0;
    std::string str;
    str.resize(40);

    for (unsigned int i=0;i<40;i++)
    {
        ch=m_textline[i] & 0x7f;   // Strip bit 7 just in case
        if (ch<' ') ch |= 0x80;
        str[j++]=ch;
    }
    return str;
}

/** GetMappedLine7bit - returns a string with text file-safe mappings applied.
 * Escape to 7 bit (required by Javascript Droidfax)
 */
std::string TTXLine::GetMappedline7bit()
{
    char ch;
    int j=0;
    std::string str;
    str.resize(80);

    for (unsigned int i=0;i<40;i++)
    {
        ch=m_textline[i] & 0x7f;   // Strip bit 7
        if (ch<' ')
        {
            str[j++]=0x1b;  // <ESC>
            ch |= 0x40;     // Move control code up
        }
        str[j++]=ch;
    }
    str.resize(j);
    return str;
}


bool TTXLine::IsDoubleHeight()
{
    for (unsigned int i=0;i<m_textline.length();i++)
        if (m_textline[i]=='\r' || m_textline[i]==0x10)
            return true;
    return false;
}

bool TTXLine::IsBlank()
{
    for (unsigned int i=0;i<40;i++)
        if (m_textline[i]!=' ')
            return false;
    return true; // Yes, the line is blank
}

char TTXLine::SetCharAt(int x,int code)
{
    char c=m_textline[x];
    m_textline[x]=code & 0x7f;
    return c;
}

char TTXLine::GetCharAt(int xLoc)
{
    if (m_textline.length()<(uint16_t)xLoc)
    {
        // extend the line to 40 characters
        std::cerr << "[TTXLine::SetCharAt] oops, need to extend this line" << std::endl;
    }
    return m_textline[xLoc];
}


bool TTXLine::IsAlphaMode(int loc)
{
    // TODO: Not sure what cursor we should use for graphics mode capital letters
    bool result=true;
    if (loc>39) return true;
    for (int i=0;i<loc;i++)
    {
        switch (m_textline[i])
        {
        case ttxCodeAlphaBlack:;
        case ttxCodeAlphaRed:;
        case ttxCodeAlphaGreen:;
        case ttxCodeAlphaYellow:;
        case ttxCodeAlphaBlue:;
        case ttxCodeAlphaMagenta:;
        case ttxCodeAlphaCyan:;
        case ttxCodeAlphaWhite:;
            result=true;
            break;
        case ttxCodeGraphicsRed:;
        case ttxCodeGraphicsGreen:;
        case ttxCodeGraphicsYellow:;
        case ttxCodeGraphicsBlue:;
        case ttxCodeGraphicsMagenta:;
        case ttxCodeGraphicsCyan:;
        case ttxCodeGraphicsWhite:;
            result=false;
            break;
        default:; // Do nothing
        }
        // This is probably a mistake or it needs a third state: "Alpha in a graphics region".
        //if (m_textLine[i]>='A' && m_textLine[i]<'Z')
        //    result=false;
    }
    return result;
}

std::string TTXLine::GetLine()
{
    if (this==nullptr)
    {
        std::cerr << "[TTXLine::GetLine] this can never happen" << std::endl;
        return "***This is a null object U dun goofed  ***";
    }
    // If the string is less than 40 characters we need to pad it or get weird render errors
    int len=m_textline.length();
    if (len>40)
    {
        // std::cerr << "[TTXLine::GetLine] len=" << len << std::endl;
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
	std::cerr << "[TTXLine::AppendLine] called" << std::endl;
	TTXLine* p;
	for (p=this;p->_nextLine;p=p->_nextLine);
	p->_nextLine=new TTXLine(line,false);
}

void TTXLine::Dump()
{
	for (int i=0;i<40;i++)
	{
		std::cerr << std::hex << std::setw(2) << (int)(m_textline[i] & 0xff) << " ";
	}
	std::cerr << std::dec << std::endl;
}
