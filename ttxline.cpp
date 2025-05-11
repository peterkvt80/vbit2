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
    //std::cerr << "TTXLine dtor " << m_textline << std::endl;
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
    std::shared_ptr<TTXLine> p;
    for (p=this->getptr();p->_nextLine;p=p->_nextLine);
    p->_nextLine.reset(new TTXLine(line,true));
}
