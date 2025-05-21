 #include "page.h"

using namespace vbit;

Page::Page() :
    _carouselPage(nullptr)
{
    ClearPage(); // initialises variables
}

Page::~Page()
{
    //std::cerr << "Page dtor\n";
}

void Page::AppendSubpage(std::shared_ptr<Subpage> s)
{
    _subpages.push_back(s);
    if (_carouselPage == nullptr)
        StepNextSubpage();
}

void Page::ClearPage()
{
    _pageNumber = 0; // an invalid page number
    _pageCoding=CODING_7BIT_TEXT;
    _pageFunction=LOP;
    _carouselPage=nullptr;
    
    _subpages.clear(); // empty subpage list
    _iter=_subpages.begin(); // reset iterator
}

void Page::RenumberSubpages()
{
    int count=0;
    unsigned int subcode;
    int code[4];
    if (_subpages.size() == 1)
    {
        // A single page
        
        // Annex A.1 states that pages with no sub-pages should be coded Mxx-0000. This is the default when no subcode is specified in tti file.
        // Annex E.2 states that the subcode may be used to transmit a BCD time code, e.g for an alarm clock. Where a non zero subcode is specified in the tti file keep it.
        
        if (Special())
        {
            // "Special" pages (e.g. MOT, POP, GPOP, DRCS, GDRCS, MIP) should be coded sequentially in hexadecimal 0000-000F
            _subpages.front()->SetSubCode(0);
        }
    }
    else if (_subpages.size() > 1)
    {
        // Page has subpages. Renumber according to Annex A.1.
        for (int i=0;i<4;i++) code[i]=0;
        std::list<std::shared_ptr<Subpage>>::iterator it;
        for (it = _subpages.begin(); it != _subpages.end(); ++it)
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
            
            (*it)->SetSubCode(subcode); // modify the subcode
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

bool Page::IsCarousel()
{
    if (_subpages.size() > 1) // has multiple subpages
    {
        return true;
    }
    
    if (std::shared_ptr<Subpage> s = GetSubpage())
    {
        if (s->GetTimedMode() && s->GetSubpageStatus() & PAGESTATUS_C9_INTERRUPTED)
        {
            // interrupted sequence flag is set, and page is in timed mode, so treat as a 1 page carousel
            return true;
        }
    }
    
    return false;
}

void Page::StepNextSubpageNoLoop()
{
    if (_subpages.empty())
    {
        _carouselPage==nullptr;
    }
    else
    {
        if (_carouselPage==nullptr)
        {
            _iter=_subpages.begin();
            _carouselPage = *_iter;
        }
        else
        {
            ++_iter;
            _carouselPage = *_iter;
        }
        if (_iter == _subpages.end())
        {
            _carouselPage = nullptr;
        }
    }
}

void Page::StepNextSubpage()
{
    if (_subpages.empty())
    {
        _carouselPage==nullptr;
    }
    else
    {
        if (_carouselPage==nullptr)
        {
            _iter=_subpages.begin();
            _carouselPage = *_iter;
        }
        else
        {
            ++_iter;
            _carouselPage = *_iter;
        }
        if (_iter == _subpages.end())
        {
            _iter = _subpages.begin();
            _carouselPage = *_iter;
        }
    }
}

std::shared_ptr<TTXLine> Page::GetTxRow(uint8_t row)
{
    // Return a line or nullptr if the row does not exist
    std::shared_ptr<TTXLine> line=nullptr;
    
    if (_carouselPage)
        line=GetSubpage()->GetRow(row);
    
    if (line!=nullptr) // Found a line
    {
        return line;
    }
    // No more lines? return NULL.
    return nullptr;
}

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
    
    for (int i=0;i<6;i++)
    {
        SetFastextLink(i,0x8ff,0x3f7f);
    }
}

Subpage::~Subpage()
{
    //std::cerr << "Subpage dtor\n";
}

std::shared_ptr<TTXLine> Subpage::GetRow(unsigned int row)
{
    if (row>MAXROW)
    {
        return nullptr;
    }
    
    if (_lines[row]==nullptr && row>0 && row<26)
    {
        _lines[row].reset(new TTXLine("                                        ")); // return a blank row for X/1-X/25
    }
    return _lines[row];
}

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
