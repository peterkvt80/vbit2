#include "ttxline.h"

// create a blank line
TTXLine::TTXLine():
    _nextLine(nullptr)
{
    _line.fill(0x20); // initialise lines with all spaces
}

// create a new line from a 40 byte array
TTXLine::TTXLine(std::array<uint8_t, 40> line):
    _nextLine(nullptr)
{
    _line = line;
}

TTXLine::TTXLine(std::string const& line):
    _nextLine(nullptr)
{
    // convert text string to 40 byte ttxline representation, expanding escape codes etc
    char ch;
    int j=0;
    _line.fill(0x20); // spaces
    for (unsigned int i=0;i<line.length() && j<40;i++)
    {
        ch = line[i] & 0x7f; // 7-bit
        if (line[i] == 0x1b) // ascii escape
        {
            i++;
            ch = line[i] & 0x3f;
        }
        else if ((uint8_t)line[i] < 0x20) // other ascii control code
        {
            break;
        }

        if (ch < 0x20)
        {
            ch |= 0x80; // set high bit on control codes
        }

        _line[j++]=ch;
    }
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

void TTXLine::AppendLine(std::shared_ptr<TTXLine> line)
{
    // Seek the end of the list
    std::shared_ptr<TTXLine> p;
    for (p=this->getptr();p->_nextLine;p=p->_nextLine);
    p->_nextLine = line;
}
