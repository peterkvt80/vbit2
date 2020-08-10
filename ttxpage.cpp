/** ***************************************************************************
 * Description       : Class for a teletext page
 * Compiler          : C++
 *
 * Copyright (C) 2014-2016, Peter Kwan
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
 #include "ttxpage.h"


bool TTXPage::pageChanged=false;

TTXPage::TTXPage() :
    m_PageNumber(FIRSTPAGE),
    m_SubPage(nullptr),
    m_sourcepage("none"),   //ctor
    m_subcode(0),
    m_Loaded(false),
    _Selected(false)
{
    m_Init();
}

/* TODO: move the body of this out into a LoadPage function */
/** ctor
 *  Load a teletext page from file
 * \param filename : Name of teletext file to load
 * \param shortFilename : Filename without path
 */
TTXPage::TTXPage(std::string filename) :
    m_PageNumber(FIRSTPAGE),
    m_SubPage(nullptr),
    m_sourcepage(filename),
    m_subcode(0),
    m_Loaded(false),
    _Selected(false)
{
    // std::cerr << "[TTXPage] file constructor loading " << filename<< std::endl;
    m_Init(); // Careful! We should move inits to the initialisation list and call the default constructor

    if (!m_Loaded)
        if (m_LoadTTI(filename))
        {
            m_Loaded=true;
        }
    
    // Other file formats support removed from here
    
    TTXPage::pageChanged=false;
    // std::cerr << "Finished reading page. Loaded=" << m_Loaded << std::endl;
}

void TTXPage::m_Init()
{
    m_region=0;
    for (int i=0;i<=MAXROW;i++)
    {
        m_pLine[i]=nullptr;
    }
    for (int i=0;i<6;i++)
    {
        SetFastextLink(i,0x8ff);
    }
    // Member variables
    m_destination="inserter";
    m_description="Description goes here";
    m_cycletimeseconds=8;
    m_cycletimetype='T';
    m_pagestatus=0; /* default to not sending page to ignore malformed/blank tti files */
    m_lastpacket=0;
    m_pagecoding=CODING_7BIT_TEXT;
    m_pagefunction=LOP;
    TTXPage::pageChanged=false;
}

TTXPage::~TTXPage()
{
    static int j=0;
    j++;
    //std::cerr << "[~TTXPage] " << this->GetSourcePage() << std::endl;
    // This bit causes a lot of grief.
    // Need to be super careful that we don't destroy it. Like if you make a copy then destroy the copy.
    for (int i=0;i<=MAXROW;i++)
    {
        // std::cerr << "Deleting line " << i << std::endl;
        if (m_pLine[i]!=nullptr)
        {
            delete m_pLine[i];
            m_pLine[i]=nullptr;
        }
    }
    
    if (Getm_SubPage()!=nullptr)
    {
        //std::cerr << "~[TTXPage]: " << j << std::endl;
        delete m_SubPage;
        m_SubPage=nullptr;
    }
}

bool TTXPage::m_LoadTTI(std::string filename)
{
    const std::string cmd[]={"DS","SP","DE","CT","PN","SC","PS","MS","OL","FL","RD","RE","PF"};
    const int cmdCount = 13; // There are 13 possible commands, maybe DT and RT too on really old files
    unsigned int lineNumber;
    int lines=0;
    // Open the file
    std::ifstream filein(filename.c_str());
    TTXPage* p=this;
    p->m_Init(); // reset page
    char * ptr;
    unsigned int subcode;
    std::string subpage;
    int pageNumber;
    char m;
    for (std::string line; std::getline(filein, line, ','); )
    {
        // This shows the command code:
        //std::cerr << line << std::endl;
        bool found=false;
        for (int i=0;i<cmdCount && !found; i++)
        {
            // std::cerr << "matching " << line << std::endl;
            if (!line.compare(cmd[i]))
            {
                found=true;
                // std::cerr << "Matched " << line << std::endl;
                switch (i)
                {
                case 0 : // "DS" - Destination inserter name
                    // DS,inserter
                    // std::cerr << "DS not implemented\n";
                    std::getline(filein, m_destination);
                    // std::cerr << "DS read " << m_destination << std::endl;
                    break;
                case 1 : // "SP" - Source page file name
                    // SP is the path + name of the file from where is was loaded. Used also for Save.
                    // SP,c:\Minited\inserter\ONAIR\P100.tti
                    //std::cerr << "SP not implemented\n";

                    std::getline(filein, line);
                    // std::getline(filein, m_sourcepage);
                    break;
                case 2 : // "DE" - Description
                    // DE,Read back page  20/11/07
                    //std::cerr << "DE not implemented\n";
                    std::getline(filein, m_description);
                    break;
                case 3 : // "CT" - Cycle time (seconds)
                    // CT,8,T
                    // std::cerr << "CT not implemented\n";
                    std::getline(filein, line, ',');
                    p->SetCycleTime(atoi(line.c_str()));
                    std::getline(filein, line);
                    m_cycletimetype=line[0]=='T'?'T':'C';
                    // TODO: CT is not decoded correctly
                    break;
                case 4 : // "PN" - Page Number mppss
                    // Where m=1..8
                    // pp=00 to ff (hex)
                    // ss=00 to 99 (decimal)
                    // PN,10000
                    std::getline(filein, line);
                    if (line.length()<3) // Must have at least three characters for a page number
                        break;
                    m=line[0];
                    if (m<'1' || m>'8') // Magazine must be 1 to 8
                        break;
                    pageNumber=std::strtol(line.c_str(), &ptr, 16);
                    // std::cerr << "Line=" << line << " " << "line length=" << line.length() << std::endl;
                    if (line.length()<5 && pageNumber<=0x8ff) // Page number without subpage? Shouldn't happen but you never know.
                    {
                        pageNumber*=0x100;
                    }
                    else   // Normally has subpage digits
                    {
                        subpage=line.substr(3,2);
                        // std::cerr << "Subpage=" << subpage << std::endl;
                        pageNumber=(pageNumber & 0xfff00) + std::strtol(subpage.c_str(),nullptr,10);
                    }
                    // std::cerr << "PN enters with m_PageNumber=" << std::hex << m_PageNumber << " pageNumber=" << std::hex << pageNumber << std::endl;
                    if (p->m_PageNumber!=FIRSTPAGE) // // Subsequent pages need new page instances
                    {
                        // std::cerr << "Created a new subpage" << std::endl;
                        int pagestatus = p->GetPageStatus();
                        TTXPage* newSubPage=new TTXPage();  // Create a new instance for the subpage
                        p->Setm_SubPage(newSubPage);            // Put in a link to it
                        p=newSubPage;                       // And jump to the next subpage ready to populate
                        p->SetPageStatus(pagestatus); // inherit status of previous page instead of default
                        p->SetCycleTimeMode(m_cycletimetype); // inherit cycle time
                        p->SetCycleTime(m_cycletimeseconds);
                    }
                    p->SetPageNumber(pageNumber);

                    // std::cerr << "PN =" << std::hex << m_PageNumber << "\n";
                    //if (m_PageNumber)
                    //    std::cerr << "new page. TBA\n";
                    break;
                case 5 : // "SC" - Subcode
                    // SC,0000
                    std::getline(filein, line);
                    subcode=std::strtol(line.c_str(), &ptr, 16);
                    // std::cerr << "SC: Subcode=" << subcode << std::endl;;

                    p->SetSubCode(subcode);
                    break;
                case 6 : // "PS" - Page status flags
                    // PS,8000
                    std::getline(filein, line);
                    p->SetPageStatus(std::strtol(line.c_str(), &ptr, 16));
                    break;
                case 7 : // "MS" - Mask
                    // MS,0
                    // std::cerr << "MS not implemented\n";
                    std::getline(filein, line); // Mask is intended for TED to protecting regions from editing.
                    break;
                case 8 : // "OL" - Output line
                    std::getline(filein, line, ',');
                    lineNumber=atoi(line.c_str());
                    std::getline(filein, line);
                    if (lineNumber>MAXROW) break;
                    p->SetRow(lineNumber,line);
                    lines++;
                    break;
                case 9 : // "FL"; - Fastext links
                    // FL,104,104,105,106,F,100
                    for (int fli=0;fli<6;fli++)
                    {
                        if (fli<5)
                            std::getline(filein, line, ',');
                        else
                            std::getline(filein, line); // Last parameter no comma
                        p->SetFastextLink(fli,std::strtol(line.c_str(), &ptr, 16));
                    }
                    break;
                case 10 : // "RD"; - not sure!
                    std::getline(filein, line);
                    break;
                case 11 : // "RE"; - Set page region code 0..f
                    std::getline(filein, line);
                    p->SetRegion(std::strtol(line.c_str(), &ptr, 16));
                    break;
                case 12 : // "PF"; - not in the tti spec, page function and coding
                    std::getline(filein, line);
                    if (line.length()<3)
                        std::cerr << "invalid page function/coding " << line << std::endl;
                    else
                    {
                        SetPageFunctionInt(std::strtol(line.substr(0,1).c_str(), &ptr, 16));
                        SetPageCodingInt(std::strtol(line.substr(2,1).c_str(), &ptr, 16));
                    }
                    break;
                default:
                    std::cerr << "Command not understood " << line << std::endl;
                } // switch
            } // if matched command
            // If the command was not found then skip the rest of the line
        } // seek command
        if (!found) std::getline(filein, line);
    }
    filein.close(); // Not sure that we need to close it
    p->Setm_SubPage(nullptr);
    // if (lines>0) std::cerr << "Finished reading TTI page. Line count=" << lines << std::endl;
    TTXPage::pageChanged=false;
    return (lines>0);
}



TTXPage::TTXPage(const TTXPage& other)
{
    //copy ctor.
    // std::cerr << "TTXPage copy constructor" << std::endl;

    m_PageNumber=other.m_PageNumber;  // PN
    m_SubPage=other.m_SubPage;
    for (int i=0;i<=MAXROW;i++)
    {
        m_pLine[i]=other.m_pLine[i]; // Is this a deep copy?
        /*
        if (m_pLine[i]!=nullptr)
            std::cerr << "[copy] Line=" << m_pLine[i]->GetLine() << std::endl;
        */

        /* Don't cover up for missing data. We need to know if a line is left empty
        if (m_pLine[i]==nullptr)
            m_pLine[i]=new TTXLine("This is a blank filler line in TTXPage copy constructor");
        */
    }
    for (int i=0;i<6;i++)
        m_fastextlinks[i]=other.m_fastextlinks[i];      // FL

    m_destination=other.m_destination;  // DS
    m_sourcepage=other.m_sourcepage;   // SP
    m_description=other.m_description;  // DE
    m_cycletimeseconds=other.m_cycletimeseconds;
    m_subcode=other.m_subcode;              // SC
    m_pagestatus=other.m_pagestatus;           // PS
    m_lastpacket=other.m_lastpacket;
    m_region=other.m_region;               // RE
    m_pagecoding=other.m_pagecoding;
    m_pagefunction=other.m_pagefunction;
    m_Loaded=other.m_Loaded;

}


/*
TTXPage& TTXPage::operator=(const TTXPage& rhs)
{
    if (this == &rhs) return *this; // handle self assignment
    //assignment operator
    return *this;
}
*/

TTXPage* TTXPage::GetPage(unsigned int pageNumber)
{
    //std::cerr << "[TTXPage::GetPage]" << std::endl;
    TTXPage* p=this;
    for (;pageNumber>0 && p->m_SubPage!=nullptr;p=p->m_SubPage, pageNumber--);
    // Iterate down the page list to find the required page object
    return p;
}


TTXLine* TTXPage::GetRow(unsigned int row)
{
    //std::cerr << "[TTXPage::GetRow] getting row " << row << std::endl;
    if (row>MAXROW)
    {
        std::cerr << "[TTXPage::GetRow]Invalid row requested: " << row << std::endl;
        return nullptr;
    }
    TTXLine* line=m_pLine[row];
    // Don't create row 0, or enhancement rows as they are special.
    if (line==nullptr && row>0 && row<25)
        line=m_pLine[row]=new TTXLine("                                        ");
    return line;
}

void TTXPage::SetRow(unsigned int rownumber, std::string line)
{
	unsigned int dc;
	
	// assert(rownumber<=MAXROW);
	if (rownumber>MAXROW) return;

	if (rownumber == 28 && line.length() >= 40){
		dc = line.at(0) & 0x0F;
		if (dc == 0 || dc == 2 || dc == 3 || dc == 4){
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
		if (rownumber > m_lastpacket)
			m_lastpacket = rownumber;
	}

	if (m_pLine[rownumber]==nullptr)
			m_pLine[rownumber]=new TTXLine(line,rownumber<MAXROW); // Didn't exist before
	else
	{
		//std::cerr << "[TTXPage::SetRow] row=" << rownumber << std::endl;
		if (rownumber<26) // Ordinary line
		{
			m_pLine[rownumber]->Setm_textline(line, true);
		}
		else // Enhanced packet
		{
			//std::cerr << "[TTXPage::SetRow] APPEND row=" << rownumber << std::endl;
			// If the line already exists we want to add the packet rather than overwrite what is already there
			m_pLine[rownumber]->AppendLine(line);
		}
	}
}

int TTXPage::GetPageCount()
{
    int count=0;
    unsigned int subcode;
    int code[4];
    if (this->m_SubPage == nullptr)
    {
        // A single page (no subpages).
        
        // Annex A.1 states that pages with no sub-pages should be coded Mxx-0000. This is the default when no subcode is specified in tti file.
        // Annex E.2 states that the subcode may be used to transmit a BCD time code, e.g for an alarm clock. Where a non zero subcode is specified in the tti file keep it.
        
        if (Special()){
            // "Special" pages (e.g. MOT, POP, GPOP, DRCS, GDRCS, MIP) should be coded sequentially in hexadecimal 0000-000F
            this->SetSubCode(0);
        }
        
        count = 1;
    }
    else
    {
        // Page has subpages. Renumber according to Annex A.1.
        for (int i=0;i<4;i++) code[i]=0;
        for (TTXPage* p=this;p!=nullptr;p=p->m_SubPage)
        {
            if (Special()){
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
    
    return count;
}

void TTXPage::CopyMetaData(TTXPage* page)
{
    m_PageNumber=page->m_PageNumber;
    for (int i=0;i<6;i++)
        SetFastextLink(i,page->GetFastextLink(i));

    m_destination=page->m_destination;  // DS
    SetSourcePage(page->GetSourcePage());// SP
    m_description=page->m_description;  // DE
    m_cycletimeseconds=page->m_cycletimeseconds;     // CT
    m_cycletimetype=page->m_cycletimetype;       // CT
    m_subcode=page->m_subcode;              // SC
    m_pagestatus=page->m_pagestatus;           // PS
    m_lastpacket=page->m_lastpacket;
    m_region=page->m_region;            // RE
}

void TTXPage::SetLanguage(int language)
{
    language=language & 0x07;   // Limit language 0..7
    m_pagestatus=m_pagestatus & ~0x0380; // Clear the old language bits
    m_pagestatus=m_pagestatus | (language << 7);   // Shift the language bits into the right place and OR them in
    // std::cerr << "Set Language: PS," << std::setw(4) << std::setfill('X') << std::hex << m_pagestatus << std::endl;
}

int TTXPage::GetLanguage()
{
    int language;
    language=(m_pagestatus >> 7) & 0x07;
    // std::cerr << "Get Language PS," << std::setw(4) << std::setfill('X') << std::hex << m_pagestatus << std::endl;
    return language;
}

void TTXPage::SetPageNumber(int page)
{
    if ((page<0x10000) || (page>0x8ff99))
    {
        std::cerr << "[TTXPage::SetPageNumber] Page number is out of range: " << std::hex << page << std::endl;
        page = 0x8FF00;
    }
    //std::cerr << "PageNumber changed from " << std::hex << m_PageNumber << " to ";
    m_PageNumber=page;
    //std::cerr << std::hex << m_PageNumber << std::endl;
}

int TTXPage::GetFastextLink(int link)
{
    if (link<0 || link>5)
        return 0;
    return m_fastextlinks[link];
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

void TTXPage::DebugDump() const
{
    std::cerr << "PAGE DUMP\n";
    std::cerr << "Desc:" << GetDescription() << std::endl;
    std::cerr << "Load:" << Loaded() << std::endl;
    std::cerr << "Page:" << std::hex << GetPageNumber() << std::dec << std::endl;
    std::cerr << "File:" << GetSourcePage() << std::endl;

    for (const TTXPage* p=this;p!=nullptr;p=p->m_SubPage)
        for (int i=0;i<25;i++)
        {
            std::cerr << "Line " << i << ": ";
            if (p->m_pLine[i]!=nullptr)
                std::cerr << " text=" << p->m_pLine[i]->GetLine() << std::endl;
        }
}

/** This is similar to the copy constructor
 * @todo This would be a better version to make into a copy constructor?
 */
void TTXPage::Copy(TTXPage* src)
{
  // Deep copy the rows.
  //std::cerr << "[Copy] Trace A" << std::endl;
  for (int i=0;i<=MAXROW;i++)
  {
    // Make sure that we have line object and that it does not get destroyed
    //std::cerr << "[Copy] Trace B" << std::endl;

    if (this->m_pLine[i]==nullptr)
    {
      this->m_pLine[i]=new TTXLine();
    }
    //std::cerr << "[Copy] Trace C" << std::endl;

    // If null then blank the line
    if (src->m_pLine[i]==nullptr)
    {
      this->SetRow(i,"                                        "); // Just blank lines rather than destroy them
    }
    else
    {
      *(this->m_pLine[i])=*(src->m_pLine[i]);
    }
    //std::cerr << "[Copy] Trace D" << std::endl;

  }
  //std::cerr << "[Copy] Trace E" << std::endl;
	this->m_SubPage=nullptr;  // (Might want to copy carousels but this is only used for subtitles so far)
  // Copy everything else
  this->CopyMetaData(src);
}

void TTXPage::SetPageFunctionInt(int pageFunction)
{
    switch (pageFunction){
        default: // treat page functions we don't know as level one pages
        case 0:
            m_pagefunction = LOP;
            break;
        case 2:
            m_pagefunction = GPOP;
            break;
        case 3:
            m_pagefunction = POP;
            break;
        case 4:
            m_pagefunction = GDRCS;
            break;
        case 5:
            m_pagefunction = DRCS;
            break;
        case 6:
            m_pagefunction = MOT;
            break;
        case 7:
            m_pagefunction = MIP;
            break;
        case 8:
            m_pagefunction = BTT;
            break;
        case 9:
            m_pagefunction = AIT;
            break;
        case 10:
            m_pagefunction = MPT;
            break;
        case 11:
            m_pagefunction = MPT_EX;
            break;
    }
}

void TTXPage::SetPageCodingInt(int pageCoding)
{
    switch (pageCoding){
        default: // treat codings we don't know yet as normal text.
        case 0:
            m_pagecoding = CODING_7BIT_TEXT;
            break;
        case 1:
            m_pagecoding = CODING_8BIT_DATA;
            break;
        case 2:
            m_pagecoding = CODING_13_TRIPLETS;
            break;
        case 3:
            m_pagecoding = CODING_HAMMING_8_4;
            break;
    }
}
