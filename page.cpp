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
    s->SetMagazine(_pageNumber >> 8); // tell subpage what magazine it is in for fastext
    
    _subpages.push_back(s);
    if (_carouselPage == nullptr)
        StepFirstSubpage();
}

void Page::InsertSubpage(std::shared_ptr<Subpage> s)
{
    s->SetMagazine(_pageNumber >> 8); // tell subpage what magazine it is in for fastext
    
    for (std::list<std::shared_ptr<Subpage>>::iterator it=_subpages.begin();it!=_subpages.end();++it)
    {
        // find first subpage with a higher subcode
        std::shared_ptr<Subpage> ptr = *it;
        if (ptr->GetSubCode() > s->GetSubCode())
        {
            _subpages.insert(it,s);
            return;
        }
    }
    
    // if we are here we ran to the end of the list without a match
    _subpages.push_back(s);
    if (_carouselPage == nullptr)
        StepFirstSubpage();
}

void Page::RemoveSubpage(std::shared_ptr<Subpage> s)
{
    _subpages.remove(s);
    
    if (_subpages.empty())
        _carouselPage = nullptr;
    else
    {
        _iter=_subpages.begin();
        _carouselPage = *_iter;
    }
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

void Page::SetPageCodingInt(int pageCoding)
{
    if (pageCoding != _pageCoding)
    {
        _pageCoding = ReturnPageCoding(pageCoding);
        for (auto it = _subpages.begin(); it != _subpages.end(); ++it)
        {
            (*it)->SetSubpageChanged(); // page coding changed so CRC needs recalculating for each subpage
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

void Page::StepFirstSubpage()
{
    if (_subpages.empty())
    {
        _carouselPage==nullptr;
    }
    else
    {
        _iter=_subpages.begin();
        _carouselPage = *_iter;
    }
}

void Page::StepLastSubpage()
{
    if (_subpages.empty())
    {
        _carouselPage==nullptr;
    }
    else
    {
        _iter=_subpages.end();
        _carouselPage = *--_iter;
    }
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
        else if (!(_carouselPage->GetSubpageStatus() & PAGESTATUS_TRANSMITPAGE))
        {
            StepNextSubpageNoLoop(); // skip over subpages if transmit flag not set
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
            if (_iter == _subpages.end())
                _iter = _subpages.begin();
            if (++_iter == _subpages.end())
                _iter = _subpages.begin();
            _carouselPage = *_iter;
        }
        
        if (!(_carouselPage->GetSubpageStatus() & PAGESTATUS_TRANSMITPAGE))
        {
            StepNextSubpageNoLoop(); // skip over subpages if transmit flag not set
        }
    }
}

// Find a subpage by subcode - Warning: this will only find the first match so don't let multiples into the list!
std::shared_ptr<Subpage> Page::LocateSubpage(uint16_t SubpageNumber)
{
    for (std::list<std::shared_ptr<Subpage>>::iterator s=_subpages.begin();s!=_subpages.end();++s)
    {
        std::shared_ptr<Subpage> ptr = *s;
        if (SubpageNumber==ptr->GetSubCode())
            return ptr;
    }
    return nullptr;
}

// attempt to set current subpage by number
void Page::SetSubpage(uint16_t SubpageNumber)
{
    if (std::shared_ptr<Subpage> s = LocateSubpage(SubpageNumber))
        _carouselPage = s;
    // no warning on failure
}

std::shared_ptr<TTXLine> Page::GetTxRow(uint8_t row)
{
    // Return a line or nullptr if the row does not exist
    std::shared_ptr<TTXLine> line=nullptr;
    
    if (_carouselPage)
        line=_carouselPage->GetRow(row);
    
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
    _mag(0),
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
        _lines[row].reset(new TTXLine()); // return a blank row for X/1-X/25
    }
    return _lines[row];
}

void Subpage::SetRow(unsigned int rownumber, std::shared_ptr<TTXLine> line)
{
    unsigned int dc;
    
    // assert(rownumber<=MAXROW);
    if (rownumber>MAXROW) return;
    
    if (rownumber == 26)
    {
        dc = line->GetCharAt(0) & 0x0F;
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
        _lines[rownumber] = line; // Didn't exist before
    }
    else
    {
        if (rownumber<26) // Ordinary line
        {
            _lines[rownumber] = line;
        }
        else // Enhanced packet
        {
            // If the line already exists we want to add to linked list of different designation codes
            _lines[rownumber]->AppendLine(line);
        }
    }
}

void Subpage::DeleteRow(unsigned int rownumber, int designationCode)
{
    if (rownumber < 26)
        _subpageChanged = true; // page content within scope of CRC was changed
    
    if (rownumber < 26 || designationCode < 0 || designationCode > 15)
    {
        _lines[rownumber] = nullptr; // delete entire row
    }
    else
    {
        if (_lines[rownumber])
            _lines[rownumber] = _lines[rownumber]->RemoveLine(designationCode); // delete specific dc
    }
}

void Subpage::SetFastext(std::array<FastextLink, 6> links)
{
    std::array<uint8_t, 40> line; // 40 bytes of packet data in CODING_HAMMING_8_4 form
    
    uint16_t lp, ls;
    uint8_t p=0;
    line[p++] = 0x0; // designation code 0
    line[37] = 0xf; // link control set
    line[38] = 0;
    line[39] = 0; // last two bytes get overwritten with page CRC by packetmag
    for (uint8_t i=0; i<6; i++)
    {
        lp=links[i].page;
        if (lp == 0) lp = 0x8ff; // turn zero into 8FF to be ignored
        ls=links[i].subpage;

        uint8_t m=(lp/0x100 ^ _mag);     // calculate the relative magazine
        line[p++]=lp & 0xF;              // page units
        line[p++]=(lp & 0xF0) >> 4;      // page tens
        line[p++]=ls & 0xF;              // S1
        line[p++]=((m & 1) << 3) | ((ls >> 4) & 0xF); // S2 + M1
        line[p++]=((ls >> 8) & 0xF);     // S3
        line[p++]=((m & 6) << 1) | ((ls >> 4) & 0x3); // S4 + M2, M3
    }
    
    std::shared_ptr<TTXLine> ttxline(new TTXLine(line));
    SetRow(27,ttxline);
}

bool Subpage::GetFastext(std::array<FastextLink, 6> *links)
{
    std::shared_ptr<TTXLine> line = _lines[27];
    if (line == nullptr)
        return false; // no X/27
    
    line = line->LocateLine(0);
    if (line == nullptr)
        return false; // no X/27/0
    
    uint8_t p=1;
    for (uint8_t i=0; i<6; i++)
    {
        uint8_t m;
        uint16_t lp, ls;
        lp = line->GetCharAt(p++) & 0xf;            // page units
        lp |= (line->GetCharAt(p++) & 0xf) << 4;    // page tens
        ls = line->GetCharAt(p++) & 0xf;            // S1
        m = (line->GetCharAt(p) & 0x8) >> 3;        // M1
        ls |= (line->GetCharAt(p++) & 0x7) << 4;    // S2
        ls |= (line->GetCharAt(p++) & 0xf) << 8;    // S3
        m |= (line->GetCharAt(p) & 0xc) >> 1;       // M2 + M3
        ls |= (line->GetCharAt(p++) & 0x3) << 12;   // S4
        links->at(i).page = ((m ^ _mag) << 8) | lp;
        links->at(i).subpage = ls;
    }
    return true;
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
