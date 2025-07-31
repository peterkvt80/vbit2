#include "ttxline.h"

TTXLine::TTXLine(std::string const& line, bool validateLine):
    _nextLine(nullptr)
{
    SetLine(line, validateLine);
}

// create a blank line
TTXLine::TTXLine():
    _nextLine(nullptr)
{
    SetLine("                                        ");
}

// create a copy of an existing line
TTXLine::TTXLine(std::shared_ptr<TTXLine> line):
    _nextLine(nullptr)
{
    _line = line->GetLine();
    
    // recurse down appended lines
    if (line->GetNextLine())
        _nextLine = std::shared_ptr<TTXLine>(new TTXLine(line->GetNextLine()));
}

TTXLine::~TTXLine()
{
    //std::cerr << "TTXLine dtor " << m_textline << std::endl;
}

void TTXLine::SetLine(std::string const& val, bool validateLine)
{
    // convert string to ttxline
    char ch;
    int j=0;
    _line.fill(0x20); // spaces
    for (unsigned int i=0;i<val.length() && j<40;i++)
    {
        ch = val[i];
        if (validateLine)
        {
            ch &= 0x7f; // 7-bit
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
        }

        _line[j++]=ch;
    }
}

bool TTXLine::IsBlank()
{
    for (unsigned int i=0;i<_line.size();i++)
    {
        if (_line[i]!=0x20)
        {
            return false;
        }
    }
    return true; // Yes, the line is blank
}

void TTXLine::AppendLine(std::string  const& line)
{
    // Seek the end of the list
    std::shared_ptr<TTXLine> p;
    for (p=this->getptr();p->_nextLine;p=p->_nextLine);
    p->_nextLine.reset(new TTXLine(line,true));
}
