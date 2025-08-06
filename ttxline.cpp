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
    // Seek through list
    std::shared_ptr<TTXLine> p=this->getptr();
    for (p=this->getptr();;p=p->_nextLine)
    {
        if ((p->GetCharAt(0)&0xF) == (line->GetCharAt(0)&0xF))
        {
            // line with same designation code already exists
            p->_line = line->_line; // overwrite it with our new line data but leave _nextLine unmodified
            return;
        }
        else if ((p->GetCharAt(0)&0xF) > (line->GetCharAt(0)&0xF))
        {
            // existing line has higher designation code than we are trying to add
            std::array<uint8_t, 40> tmp = p->_line; // preserve original line data
            line->_nextLine = p->_nextLine; // move original _nextLine pointer to our new line
            p->_line = line->_line; // move our new line data to original line
            line->_line = tmp; // move original line data into our new line
            p->_nextLine = line; // finally update _nextLine pointer to our line which now contains the original line
        }
        
        if (p->_nextLine == nullptr) // reached the end of the list
            break;
    }
    
    p->_nextLine = line; // append line to end of chain
}

std::shared_ptr<TTXLine> TTXLine::RemoveLine(uint8_t designationCode)
{
    // returns new pointer to linked list for this line
    if ((_line[0]&0xF) == (designationCode&0xF)) // the root line matches the designation code
    {
        if (_nextLine == nullptr) // this is the only line
            return nullptr;
        else
            return _nextLine; // return the next line as the new root
    }
    
    // seek through list for a specific designation code
    std::shared_ptr<TTXLine> p=this->getptr();
    for (p=this->getptr();p->_nextLine!=nullptr;p=p->_nextLine)
    {
        if ((p->_nextLine->GetCharAt(0)&0xF) == (designationCode&0xF))
        {
            // next line matches designationCode
            p->_nextLine = p->_nextLine->_nextLine; // cut line out of list
            
            if (p->_nextLine == nullptr)
                break; // this is now the end of the list so break out of loop
        }
    }
    
    return this->getptr(); // give back the same list
}

std::shared_ptr<TTXLine> TTXLine::LocateLine(uint8_t designationCode)
{
    // seek through list for a specific designation code
    std::shared_ptr<TTXLine> p=this->getptr();
    for (p=this->getptr();;p=p->_nextLine)
    {
        if ((p->GetCharAt(0)&0xF) == (designationCode&0xF))
        {
            return p; // return the line
        }
        
        if (p->_nextLine == nullptr) // reached the end of the list
            break;
    }
    return nullptr;
}
