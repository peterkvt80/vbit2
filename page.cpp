 #include "page.h"

using namespace vbit;

Subpage::Subpage() :
    _subcode(0),
    _status(0),
    _cycleTime(1),
    _timedMode(false),
    _region(0),
    _lastPacket(0),
    _subpageChanged(true),
    _headerCRC(0),
    _subpageCRC(0)
{
    for (int i=0;i<=MAXROW;i++)
    {
        _lines[i]=nullptr; // delete rows
    }
}

Subpage::~Subpage()
{
    std::cerr << "Subpage dtor\n";
}

Page::Page() :
    _headerCRC(0),
    _pageCRC(0)
{
    ClearPage(); // initialises variables
    
    for (int i=0;i<6;i++)
    {
        SetFastextLink(i,0x8ff);
    }
}

Page::~Page()
{
    //std::cerr << "Page dtor\n";
}

void Page::ClearPage()
{
    _pageNumber = FIRSTPAGE;
    _pageCoding=CODING_7BIT_TEXT;
    _pageFunction=LOP;
    
    m_cycletimetype='C';
    m_cycletimeseconds=1; /* default to cycling carousels every page cycle */
    m_subcode=0;
    m_pagestatus=0; /* default to not sending page to ignore malformed/blank tti files */
    m_region=0;
    m_lastpacket=0;
    
    if (m_SubPage!=nullptr)
    {
        m_SubPage=nullptr; // delete subpages
    }
    
    _subpages.clear(); // empty subpage list
    
    for (int i=0;i<=MAXROW;i++)
    {
        m_pLine[i]=nullptr; // delete rows
        _pageChanged = true; // page content within scope of CRC was changed
    }
    
    for (int i=0;i<6;i++)
    {
        SetFastextLink(i,0x8ff); // clear links
    }
}

std::shared_ptr<TTXLine> Page::GetRow(unsigned int row)
{
    if (row>MAXROW)
    {
        return nullptr;
    }
    
    if (m_pLine[row]==nullptr && row>0 && row<26)
    {
        m_pLine[row].reset(new TTXLine("                                        ")); // return a blank row for X/1-X/25
    }
    return m_pLine[row];
}

void Page::SetRow(unsigned int rownumber, std::string line)
{
    unsigned int dc;
    
    // assert(rownumber<=MAXROW);
    if (rownumber>MAXROW) return;
    
    if (rownumber == 26 && line.length() >= 40)
    {
        dc = line.at(0) & 0x0F;
        if ((dc + 26) > m_lastpacket)
            m_lastpacket = dc + 26;
    }
    else if (rownumber < 26)
    {
        _pageChanged = true; // page content within scope of CRC was changed
        
        if (rownumber > m_lastpacket)
            m_lastpacket = rownumber;
    }

    if (m_pLine[rownumber]==nullptr)
    {
        m_pLine[rownumber].reset(new TTXLine(line,true)); // Didn't exist before
    }
    else
    {
        if (rownumber<26) // Ordinary line
        {
            m_pLine[rownumber]->Setm_textline(line, true);
        }
        else // Enhanced packet
        {
            // If the line already exists we want to add the packet rather than overwrite what is already there
            m_pLine[rownumber]->AppendLine(line);
        }
    }
}

void Page::RenumberSubpages()
{
    int count=0;
    unsigned int subcode;
    int code[4];
    if (this->m_SubPage == nullptr)
    {
        // A single page (no subpages).
        
        // Annex A.1 states that pages with no sub-pages should be coded Mxx-0000. This is the default when no subcode is specified in tti file.
        // Annex E.2 states that the subcode may be used to transmit a BCD time code, e.g for an alarm clock. Where a non zero subcode is specified in the tti file keep it.
        
        if (Special())
        {
            // "Special" pages (e.g. MOT, POP, GPOP, DRCS, GDRCS, MIP) should be coded sequentially in hexadecimal 0000-000F
            this->SetSubCode(0);
        }
    }
    else
    {
        // Page has subpages. Renumber according to Annex A.1.
        for (int i=0;i<4;i++) code[i]=0;
        for (std::shared_ptr<Page> p(this->getptr());p!=nullptr;p=p->m_SubPage)
        {
            if (Special())
            {
                // "Special" pages (e.g. MOT, POP, GPOP, DRCS, GDRCS, MIP) should be coded sequentially in hexadecimal 0000-000F
                subcode = count;
            }
            else
            {
                // Pages intended for display with sub-pages should have sub-pages coded sequentially from Mxx-0001 to
                // Mxx-0009 and then Mxx-0010 to Mxx-0019 and similarly using the decimal values of sub-code nibbles, from subcode 0001 up to 0079 as per ETS-300-706 Annex A.1.
                // Pages intended for display shouldn't have more than 79 subpages, however we should handle it sensibly if they do, therefore above 0079 the numbers jump to 01nn to 09nn, then 1nnn to 3nnn.

                // Increment the subcode is a baroque way
                code[3]++; // increment units
                if (code[3]>9) // if units > 9
                {
                    code[3]=0; // units = 0
                    code[2]++; // increment tens
                    if (code[2]>7) // if tens > 7
                    {
                        code[2]=0; // tens = 0
                        code[1]++; // increment 'hundreds'
                        if (code[1]>9) // if 'hundreds' > 9
                        {
                            code[1]=0; // 'hundreds' = 0
                            code[0]++; // increment 'thousands'
                            if (code[0]>3) // if 'thousands' > 3
                            {
                                code[0]=0; // overflow subcode
                                code[1]=0;
                                code[2]=0;
                                code[3]=0;
                            }
                        }
                    }
                }
                subcode=(code[0]<<12) + (code[1]<<8) + (code[2]<<4) + code[3];
            }
            
            if (p!=nullptr)
                p->SetSubCode(subcode); // modify the subcode
            count++;
        }
    }
}

void Page::SetPageNumber(int page)
{
    if ((page<0x100) || (page>0x8ff))
    {
        page = 0x8FF;
    }
    _pageNumber=page;
}

void Page::SetFastextLink(int link, int value)
{
    if (link<0 || link>5 || value>0x8ff)
    {
        m_fastextlinks[link].page=0x8ff; // When no particular page is specified
        m_fastextlinks[link].subpage=0x3f7f;
        return;
    }
    m_fastextlinks[link].page=value;
    m_fastextlinks[link].subpage=0x3f7f;
}

void Page::SetPageFunctionInt(int pageFunction)
{
    switch (pageFunction)
    {
        default: // treat page functions we don't know as level one pages
        case 0:
        {
            _pageFunction = LOP;
            break;
        }
        case 2:
        {
            _pageFunction = GPOP;
            break;
        }
        case 3:
        {
            _pageFunction = POP;
            break;
        }
        case 4:
        {
            _pageFunction = GDRCS;
            break;
        }
        case 5:
        {
            _pageFunction = DRCS;
            break;
        }
        case 6:
        {
            _pageFunction = MOT;
            break;
        }
        case 7:
        {
            _pageFunction = MIP;
            break;
        }
        case 8:
        {
            _pageFunction = BTT;
            break;
        }
        case 9:
        {
            _pageFunction = AIT;
            break;
        }
        case 10:
        {
            _pageFunction = MPT;
            break;
        }
        case 11:
        {
            _pageFunction = MPT_EX;
            break;
        }
    }
}

PageCoding Page::ReturnPageCoding(int pageCoding)
{
    switch (pageCoding)
    {
        default: // treat codings we don't know yet as normal text.
        case 0:
            return CODING_7BIT_TEXT;
        case 1:
            return CODING_8BIT_DATA;
        case 2:
            return CODING_13_TRIPLETS;
        case 3:
            return CODING_HAMMING_8_4;
        case 4:
            return CODING_HAMMING_7BIT_GROUPS;
        case 5:
            return CODING_PER_PACKET;
    }
}

bool Page::HasHeaderChanged(uint16_t crc)
{
    if (_headerCRC != crc)
    {
        // update stored CRC and signal change
        _headerCRC = crc;
        return true;
    }
    
    return false; // no change
}

/** MOVING STUFF TO SUBPAGE CLASS HERE **/

void Subpage::SetRow(unsigned int rownumber, std::string line)
{
    unsigned int dc;
    
    // assert(rownumber<=MAXROW);
    if (rownumber>MAXROW) return;
    
    if (rownumber == 26 && line.length() >= 40)
    {
        dc = line.at(0) & 0x0F;
        if ((dc + 26) > _lastPacket)
            _lastPacket = dc + 26;
    }
    else if (rownumber < 26)
    {
        _subpageChanged = true; // page content within scope of CRC was changed
        
        if (rownumber > _lastPacket)
            _lastPacket = rownumber;
    }

    if (_lines[rownumber]==nullptr)
    {
        _lines[rownumber].reset(new TTXLine(line,true)); // Didn't exist before
    }
    else
    {
        if (rownumber<26) // Ordinary line
        {
            _lines[rownumber]->Setm_textline(line, true);
        }
        else // Enhanced packet
        {
            // If the line already exists we want to add the packet rather than overwrite what is already there
            _lines[rownumber]->AppendLine(line);
        }
    }
}

void Subpage::SetFastextLink(uint8_t link, uint16_t page, uint16_t subpage)
{
    if (link>5 || page<0x100 || page>0x8ff)
    {
        _fastextLinks[link].page = 0x8ff;
        _fastextLinks[link].subpage = 0x3f7f;
    }
    else
    {
        _fastextLinks[link].page = page;
        _fastextLinks[link].subpage = subpage & 0x3f7f;
    }
}

bool Subpage::HasHeaderChanged(uint16_t crc)
{
    if (_headerCRC != crc)
    {
        // update stored CRC and signal change
        _headerCRC = crc;
        return true;
    }
    
    return false; // no change
}
