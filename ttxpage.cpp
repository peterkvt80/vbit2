 #include "ttxpage.h"

TTXPage::TTXPage() :
    _headerCRC(0),
    _pageCRC(0)
{
    ClearPage(); // initialises variables
    
    for (int i=0;i<6;i++)
    {
        SetFastextLink(i,0x8ff);
    }
}

TTXPage::~TTXPage()
{
    //std::cerr << "TTXPage dtor\n";
}

void TTXPage::ClearPage()
{
    m_PageNumber = FIRSTPAGE;
    m_cycletimetype='C';
    m_cycletimeseconds=1; /* default to cycling carousels every page cycle */
    m_subcode=0;
    m_pagestatus=0; /* default to not sending page to ignore malformed/blank tti files */
    m_region=0;
    m_lastpacket=0;
    m_pagecoding=CODING_7BIT_TEXT;
    m_pagefunction=LOP;
    
    if (m_SubPage!=nullptr)
    {
        m_SubPage=nullptr; // delete subpages
    }
    
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

std::shared_ptr<TTXLine> TTXPage::GetRow(unsigned int row)
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

void TTXPage::SetRow(unsigned int rownumber, std::string line)
{
    unsigned int dc;
    
    // assert(rownumber<=MAXROW);
    if (rownumber>MAXROW) return;

    if (rownumber == 28 && line.length() >= 40)
    {
        dc = line.at(0) & 0x0F;
        if (dc == 0 || dc == 2 || dc == 3 || dc == 4)
        {
            // packet is X/28/0, X/28/2, X/28/3, or X/28/4
            int triplet = line.at(1) & 0x3F;
            triplet |= (line.at(2) & 0x3F) << 6;
            triplet |= (line.at(3) & 0x3F) << 12; // first triplet contains page function and coding
            // function and coding packet 28 override values set by an earlier PF row
            SetPageCodingInt((triplet & 0x70) >> 4);
            SetPageFunctionInt(triplet & 0x0F);
        }
    }
    
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

void TTXPage::RenumberSubpages()
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
        for (std::shared_ptr<TTXPage> p(this->getptr());p!=nullptr;p=p->m_SubPage)
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

void TTXPage::SetPageNumber(int page)
{
    if ((page<0x10000) || (page>0x8ff99))
    {
        page = 0x8FF00;
    }
    m_PageNumber=page;
}

void TTXPage::SetFastextLink(int link, int value)
{
    if (link<0 || link>5 || value>0x8ff)
    {
        m_fastextlinks[link]=0x8ff; // When no particular page is specified
        return;
    }
    m_fastextlinks[link]=value;
}

void TTXPage::SetPageFunctionInt(int pageFunction)
{
    switch (pageFunction)
    {
        default: // treat page functions we don't know as level one pages
        case 0:
        {
            m_pagefunction = LOP;
            break;
        }
        case 2:
        {
            m_pagefunction = GPOP;
            break;
        }
        case 3:
        {
            m_pagefunction = POP;
            break;
        }
        case 4:
        {
            m_pagefunction = GDRCS;
            break;
        }
        case 5:
        {
            m_pagefunction = DRCS;
            break;
        }
        case 6:
        {
            m_pagefunction = MOT;
            break;
        }
        case 7:
        {
            m_pagefunction = MIP;
            break;
        }
        case 8:
        {
            m_pagefunction = BTT;
            break;
        }
        case 9:
        {
            m_pagefunction = AIT;
            break;
        }
        case 10:
        {
            m_pagefunction = MPT;
            break;
        }
        case 11:
        {
            m_pagefunction = MPT_EX;
            break;
        }
    }
}

PageCoding TTXPage::ReturnPageCoding(int pageCoding)
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

bool TTXPage::HasHeaderChanged(uint16_t crc)
{
    if (_headerCRC != crc)
    {
        // update stored CRC and signal change
        _headerCRC = crc;
        return true;
    }
    
    return false; // no change
}
