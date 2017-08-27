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
    m_Loaded(false)
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
    m_Loaded(false)
{
    // std::cerr << "[TTXPage] file constructor loading " << filename<< std::endl;
    m_Init(); // Careful! We should move inits to the initialisation list and call the default constructor
    // Try all the possible formats.

    if (!m_Loaded)
        if (m_LoadTTI(filename))
				{
            m_Loaded=true;
				}

    if (!m_Loaded)
        if (m_LoadVTX(filename))
            m_Loaded=true;

    if (!m_Loaded)
        if (m_LoadEP1(filename))
            m_Loaded=true;
/* This is a disaster. index.html hits this as true
    if (!m_Loaded)
        if (m_LoadTTX(filename))
            m_Loaded=true;
*/
    TTXPage::pageChanged=false;
    // std::cerr << "Finished reading page. Loaded=" << m_Loaded << std::endl;
}


static int instanceCount=0;

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
    m_pagestatus=0x8000;
    m_pagecoding=CODING_7BIT_TEXT;
    instance=instanceCount++;
    TTXPage::pageChanged=false;

}

TTXPage::~TTXPage()
{
    static int j=0;
    j++;
    std::cerr << "[~TTXPage] " << this->GetSourcePage() << std::endl;
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

    /* Does this leave sub pages leaking memory?
       No. The destructor will cascade through the whole chain */
    if (Getm_SubPage()!=nullptr)
    {
        //std::cerr << "~[TTXPage]: " << j << std::endl;
        delete m_SubPage;
        m_SubPage=nullptr;
    }
}

// See http://rtlalphanet.asp.tss.nl/RTL4/100s01 for examples
bool TTXPage::m_LoadVTX(std::string filename)
{
    //std::cerr << "Trying VTX" << std::endl;
    char buf[500];
    TTXPage* p=this;
    std::ifstream filein(filename.c_str(), std::ios::binary | std::ios::in);
    // First 10 chars should be ham encoded. No error correction allowed
    filein.read(buf,9);
    for (int i=0;i<9;i++)
    {
        uint8_t ch=buf[i];
        switch (ch)
        {
        case 0x15: break;
        case 0x02: break;
        case 0x49: break;
        case 0x5e: break;
        case 0x64: break;
        case 0x73: break;
        case 0x38: break;
        case 0x2f: break;
        case 0xd0: break;
        case 0xc7: break;
        case 0x8c: break;
        case 0x9b: break;
        case 0xa1: break;
        case 0xb6: break;
        case 0xfd: break;
        case 0xea: break;
        default:
            return false; // Not a VTX if not HAM
        }
    }
    //std::cerr << std::endl;
    filein.read(buf,119); // This contains headery stuff to be decoded

    for (int line=1;line<25;line++)
    {
        filein.read(buf,42); // TODO: Check for a failed read and abandon
        std::string s(buf);
        p->SetRow(line,s);
    }


    for (int i=1;i<2000;i++)
    {
        filein.read(buf,1);
        uint8_t ch=buf[0];
        switch (ch)
        {
        case 0x15: std::cerr << "<0>";break;
        case 0x02: std::cerr << "<1>";break;
        case 0x49: std::cerr << "<2>";break;
        case 0x5e: std::cerr << "<3>";break;
        case 0x64: std::cerr << "<4>";break;
        case 0x73: std::cerr << "<5>";break;
        case 0x38: std::cerr << "<6>";break;
        case 0x2f: std::cerr << "<7>";break;
        case 0xd0: std::cerr << "<8>";break;
        case 0xc7: std::cerr << "<9>";break;
        case 0x8c: std::cerr << "<a>";break;
        case 0x9b: std::cerr << "<b>";break;
        case 0xa1: std::cerr << "<c>";break;
        case 0xb6: std::cerr << "<d>";break;
        case 0xfd: std::cerr << "<e>";break;
        case 0xea: std::cerr << "<f>";break;
        default:
            std::cerr << (char)(buf[0] & 0x7f);
        }
    }
    std::cerr << std::endl;
    return true;
    if ((buf[0]!=(char)0xFE) || (buf[1]!=(char)0x01) || (buf[2]!=(char)0x09))
    {
        filein.close();
        return false;
    }
    SetSourcePage(filename+".tti"); // Add tti to ensure that we don't destroy the original
    // Next we load 24 lines  of 40 characters
    for (int i=0;i<24;i++)
    {
        filein.read(buf,40); // TODO: Check for a failed read and abandon
        buf[40]=0;
        std::string s(buf);
        p->SetRow(i,s);
    }
    p->SetRow(0,"         wxTED mpp DAY dd MTH \x3 hh:nn.ss"); // Overwrite anything in row 0 (usually empty)
    // With a pair of zeros at the end we can skip
    filein.close(); // Not sure that we need to close it
    p->Setm_SubPage(nullptr);
    TTXPage::pageChanged=false;
    return true;
}

bool TTXPage::m_LoadEP1(std::string filename)
{
    char buf[100];
    TTXPage* p=this;
    std::ifstream filein(filename.c_str(), std::ios::binary | std::ios::in);
    // First 6 chars should be FE 01 09 00 00 00
    filein.read(buf,6);
    if ((buf[0]!=(char)0xFE) || (buf[1]!=(char)0x01) || (buf[2]!=(char)0x09))
    {
        filein.close();
        return false;
    }
    SetSourcePage(filename+".tti"); // Add tti to ensure that we don't destroy the original
    // Next we load 24 lines  of 40 characters
    for (int i=0;i<24;i++)
    {
        filein.read(buf,40); // TODO: Check for a failed read and abandon
        buf[40]=0;
        std::string s(buf);
        p->SetRow(i,s);
    }
    p->SetRow(0,"         wxTED mpp DAY dd MTH \x3 hh:nn.ss"); // Overwrite anything in row 0 (usually empty)
    // With a pair of zeros at the end we can skip
    filein.close(); // Not sure that we need to close it
    p->Setm_SubPage(nullptr);
    TTXPage::pageChanged=false;
    return true;
}

bool TTXPage::m_LoadTTX(std::string filename)
{
    char buf[1100]; // Don't think we need this much buffer. Just a line will do
    TTXPage* p=this;
    std::ifstream filein(filename.c_str(), std::ios::binary | std::ios::in);
    // First 0x61 chars are some sort of header. TODO: Find out what the format is to get metadata out
    filein.read(buf,0x61);

    // TODO: More validation for this format
    // File must start with CEBRA
    if ((buf[0]!='C') || (buf[1]!='E') || (buf[2]!='B') || (buf[3]!='R') || (buf[4]!='A'))
    {
        //char buf2[1100];
        // Not a CEBRA file. Could be a raw 1000 byte file?
        // get length of file:
        filein.seekg (0, filein.end);
        int length = filein.tellg();
        filein.seekg (0, filein.beg);
        std::cerr << "length=" << length << std::endl;
        if (length==1000) // Raw file? Yes! // @todo Multipage
        {
            SetSourcePage(filename+".tti"); // Add tti to ensure that we don't destroy the original
            // Next we load 24 lines of 40 characters
            for (int i=0;i<25;i++)
            {
                filein.read(buf,40);
                if (i==0)
                {
                    findPageNumber(buf);
                }

                for (int j=0;j<40;j++) if (buf[j]=='\0') buf[j]=ttxCodeAlphaBlue; // Should be Alpha black! But tricky!
                p->SetRow(i,buf);
            }

            filein.close();
            p->Setm_SubPage(nullptr);
            TTXPage::pageChanged=false;
            return true;
        }
        /// @todo teletext.org.uk ttx grabs
        if (length>1000) // Multiple raw page from teletext.co.uk
        {
            //wxTEDFrame * win = new wxTEDFrame(0);
            //win->OnMenuNew(event);
            //win->Show(true);
            /// @todo Open a new window with each page that we decode.
            //win->Page()->SetSourcePage(filename+".tti"); // Add tti to ensure that we don't destroy the original
            // Next we load 24 lines of 40 characters
            for (int i=0;i<25;i++)
            {
                filein.read(buf,40);
                int pageNum;
                if (i==0)
                {
                    pageNum=findPageNumber(buf); // @todo Take the number of this page and put it in the meta data
                    if (pageNum>0x100) {
                        p->SetPageNumber(pageNum);
                    }
                }
                for (int j=0;j<40;j++) if (buf[j]=='\0') buf[j]=ttxCodeAlphaBlue; // Should be Alpha black! But tricky!
                p->SetRow(i,buf);
            }

            filein.close();
            p->Setm_SubPage(nullptr);
            TTXPage::pageChanged=false;
            return true;

        }
        // File failed to load
        filein.close();
        return false;
    }
    // Cebra file follows....
    SetSourcePage(filename+".tti"); // Add tti to ensure that we don't destroy the original
    // Next we load 24 lines  of 40 characters
    for (int i=0;i<24;i++)
    {
        filein.read(buf,7); // Skip preamble
        filein.read(buf,40); // TODO: Check for a failed read and abandon
        buf[40]=0;
        std::string s(buf);
        p->SetRow(i+1,s);
    }
    p->SetRow(0,"         wxTED mpp DAY dd MTH \x3 hh:nn.ss"); // Overwrite anything in row 0 (usually empty)

    filein.close();
    p->Setm_SubPage(nullptr);
    TTXPage::pageChanged=false;
    return true;
}

int TTXPage::findPageNumber(char* buf)
{
    int result=0;
    int state=0;
    char* p=buf;
    for (int i=0;i<40;i++)
    {
        switch (state)
        {
            // Looking for 1..8 magazine
        case 0: if (*p>='1' && *p<='8')
            {
                result=(*p-'0') << 4;
                state++;
            }
            break;
        case 1: if (*p>='0' && *p<='9')
            {
                result=(result+*p-'0') << 4;
                state++;
                break;
            }
            if (*p>='A' && *p<='F')
            {
                result=(result+*p-'A'+10) << 4;
                state++;
                break;
            }
            if (*p>='a' && *p<='f')
            {
                result=(result+*p-'0'+10) << 4;
                state++;
                break;
            }
            state=0;
            break;
        case 2:
            if (*p>='0' && *p<='9')
            {
                result=result+*p-'0';
            }
            else
            if (*p>='A' && *p<='F')
            {
                result=result+*p-'A'+10;
            }
            else
            if (*p>='a' && *p<='f')
            {
                result=result+*p-'0'+10;
            }
            else
            {
                state=0;
                break;
            }
            return result;
        }
        p++;
    }
    return -1;
}

bool TTXPage::m_LoadTTI(std::string filename)
{
    const std::string cmd[]={"DS","SP","DE","CT","PN","SC","PS","MS","OL","FL","RD","RE"};
    const int cmdCount = 12; // There are 12 possible commands, maybe DT and RT too on really old files
    unsigned int lineNumber;
    int lines=0;
    // Open the file
    std::ifstream filein(filename.c_str());
    TTXPage* p=this;
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
                    m_cycletimeseconds=atoi(line.c_str());
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
                    if (line.length()<5 && pageNumber<0x8ff) // Page number without subpage? Shouldn't happen but you never know.
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
                        TTXPage* newSubPage=new TTXPage();  // Create a new instance for the subpage
                        p->Setm_SubPage(newSubPage);            // Put in a link to it
                        p=newSubPage;                       // And jump to the next subpage ready to populate
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
                    m_pagestatus=std::strtol(line.c_str(), &ptr, 16);
                    // Don't copy the bits to the UI...
                    // because this may not be the root page.
                    break;
                case 7 : // "MS" - Mask
                    // MS,0
                    // std::cerr << "MS not implemented\n";
                    std::getline(filein, line); // Mask is intended for TED to protecting regions from editing.
                    break;
                case 8 : // "OL" - Output line
                    // OL,9,ƒA-Z INDEX     ‡199ƒNEWS HEADLINES  ‡101
                    std::getline(filein, line, ',');
                    lineNumber=atoi(line.c_str());
                    std::getline(filein, line);
                    if (lineNumber>MAXROW) break;
                    // std::cerr << "reading " << lineNumber << " line=" << line << std::endl;
                    p->SetRow(lineNumber,line);
                    // std::cerr << lineNumber << ": OL partly implemented. " << line << std::endl;
                    lines++;
                    break;
                case 9 : // "FL"; - Fastext links
                    // FL,104,104,105,106,F,100
                    // std::cerr << "FL not implemented\n";
                    for (int fli=0;fli<6;fli++)
                    {
                        if (fli<5)
                            std::getline(filein, line, ',');
                        else
                            std::getline(filein, line); // Last parameter no comma
                        SetFastextLink(fli,std::strtol(line.c_str(), &ptr, 16));
                    }
                    break;
                case 10 : // "RD"; - not sure!
                    std::getline(filein, line);
                    break;
                case 11 : // "RE"; - Set page region code 0..f
                    std::getline(filein, line); // TODO: Implement this
                    m_region=std::strtol(line.c_str(), &ptr, 16);
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
    m_region=other.m_region;               // RE
    m_pagecoding=other.m_pagecoding;

        //int instance;

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
	// assert(rownumber<=MAXROW);
	if (rownumber>MAXROW) return;

	if (rownumber == 28){
		int dc = line.at(0) & 0x0F;
		if (dc == 0 || dc == 2 || dc == 3 || dc == 4){
			// packet is X/28/0, X/28/2, X/28/3, or X/28/4
			int triplet = line.at(1) & 0x3F;
			triplet |= (line.at(2) & 0x3F) << 6;
			triplet |= (line.at(3) & 0x3F) << 12; // first triplet contains page function and coding

			m_pagecoding = (triplet & 0x70) >> 4;

			m_region = 0; // ignore any region from RE line as X/28 sets the character set
		}
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

void TTXPage::m_OutputLines(std::ofstream& ttxfile, TTXPage* p)
{
    ttxfile << "PN," << m_FormatPageNumber(p) << "\n";
    ttxfile << "SC," << std::dec << std::setw(4) << std::setfill('0') << p->m_subcode << "\n";   // Subcode for these lines
    ttxfile << "PS," << std::setw(4) << std::setfill('X') << std::hex << p->m_pagestatus << std::endl;
    ttxfile << "RE," << std::setw(1) << std::hex << p->m_region << std::endl;


    for (int i=0;i<=MAXROW;i++)
    {

        if (p->m_pLine[i]!=nullptr && !p->m_pLine[i]->IsBlank()) // Skip empty lines
        {
            // This one for Andreas
//             std::string s=p->m_pLine[i]->GetMappedline(); // Choose the 7 bit output as it is more useful. TODO: Make this a menu option.
            // This one for Droidfax compatibility
            std::string s=p->m_pLine[i]->GetMappedline7bit(); // Choose the 7 bit output as it is more useful. TODO: Make this a menu option.
            ttxfile << "OL," << std::dec << i << "," << s << "\n";
        }
				else
				if (p->m_pLine[i]==nullptr)
				{
					std::cerr << "[m_OutputLines] rejected NULL on row=" << i << std::endl;
				}
    }
    // std::cerr << "sent a subpage" << "\n";
}

std::string TTXPage::m_FormatPageNumber(TTXPage* p)
{
    std::ostringstream PN;
    int page=p->m_PageNumber;
    // Split the page number mppss
    int mpp=page >> 8; // This bit is hex
    int ss=page & 0xff; // But this bit is decimal
    PN << std::hex << std::setw(3) << mpp << std::setfill('0') << std::dec << std::setw(2) << ss;
    return PN.str();
}

bool TTXPage::SavePageDefault()
{
  return SavePage(GetSourcePage());
}

/* 8 bit save */
bool TTXPage::SavePage(std::string filename)
{
  std::ofstream ttxfile(filename.c_str()); // TODO: Save and Save as
  SetSourcePage(filename);
  // Fix up subcodes.
  // Subcodes need to be ascending starting from 1
  if (Getm_SubPage()!=nullptr)
  {
    // Fix up subcodes.
    // Subcodes need to be ascending starting from 1
    int sc=1;
    int pageNum=this->GetPageNumber() & 0xfff00; // Mask off the original subcode
    for (TTXPage* p=this;p!=nullptr;p=p->m_SubPage)
    {
      p->SetSubCode(sc);            // Monotonic subcode
      p->SetPageNumber(pageNum + (sc & 0xff)); // Fix the page number too. (@todo: sc needs to be decimal, not hex)
      sc++;
    }
  }
  if (ttxfile.is_open())
  {
    ttxfile << std::dec ;
    // std::cerr << "[TTXPage::SavePage] filename=" << filename << std::endl;
    ttxfile << "DE," << m_description << std::endl;
    //ttxfile << "PN," << std::hex << std::setprecision(5) << m_PageNumber << std::endl;
    ttxfile << "DS," << m_destination << std::dec << std::endl;
    ttxfile << "SP," << GetSourcePage() << std::endl; // SP is set every time there is a save
    ttxfile << "CT," << m_cycletimeseconds << "," << m_cycletimetype << std::dec << std::endl;
    // My spidey instincts tell me that this code could be factorised
    m_OutputLines(ttxfile, this);
    ttxfile << std::hex;
    // Don't output null links
    if (m_fastextlinks[0]!=0x8ff)
    {
      ttxfile << "FL,"
      << m_fastextlinks[0] << ","
      << m_fastextlinks[1] << ","
      << m_fastextlinks[2] << ","
      << m_fastextlinks[3] << ","
      << m_fastextlinks[4] << ","
      << m_fastextlinks[5] << std::endl;
    }
    ttxfile << std::dec;
    // Now also have to traverse the rest of the page tree
    if (Getm_SubPage()!=nullptr)
    {
      // std::cerr << "m_SubPage=" << std::hex << Getm_SubPage() << std::endl;
      for (TTXPage* p=this->m_SubPage;p!=nullptr;p=p->m_SubPage)
      {
        m_OutputLines(ttxfile, p);
        // Subpages now have an identical copy of the main fastext links
        // Don't output null links
        if (m_fastextlinks[0]!=0x8ff)
        {
          ttxfile << std::hex;
          ttxfile << "FL,"
          << m_fastextlinks[0] << ","
          << m_fastextlinks[1] << ","
          << m_fastextlinks[2] << ","
          << m_fastextlinks[3] << ","
          << m_fastextlinks[4] << ","
          << m_fastextlinks[5] << std::endl;
          ttxfile << std::dec;
        }
      }
    }
  }
  else
    return false; // fail
  return true; // success
}

int TTXPage::GetPageCount()
{
	// See Annex A.1 rules
	int count=0;
	unsigned int subcode; // Start from 1.
	int code[4];
	for (int i=0;i<4;i++) code[i]=0;
	for (TTXPage* p=this;p!=nullptr;p=p->m_SubPage)
	{
		// Pages intended for display with sub-pages should have sub-pages coded sequentially from Mxx-0001 to
		// Mxx-0009 and then Mxx-0010 to Mxx-0019 and similarly using the decimal values of sub-code nibbles S2
		// and S1 to Mxx-0079.

		// Increment the subcode is a baroque way
		code[3]++;
		if (code[3]>9)
		{
			code[3]=0;
			code[2]++;
			if (code[2]>9)
			{
				code[2]=0;
				code[1]++;
				if (code[1]>9)
				{
					code[1]=0;
					code[0]++;
					if (code[0]>9)
					{
						code[0]=0;
						code[1]=0;
						code[2]=0;
						code[3]=0;
					}
				}
			}
		}
		subcode=(code[0]<<12) + (code[1]<<8) + (code[2]<<4) + code[3];

		// std::cerr <<"Get page count happens here, subcode=" << subcode << " " << (int)p << std::endl;
		if (p!=nullptr)
			p->SetSubCode(subcode);   // Always redo the subcodes
		count++;
	}
	// std::cerr << "GetPageCount returns " << count << std::endl;
	if (count==1)
		this->SetSubCode(0); // Pages with no sub-pages associated should be coded Mxx-0000
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
    if ((page<0x10000) || (page>0x8ff99) || (page&0xFF00) == 0xFF00)
    {
        std::cerr << "[TTXPage::SetPageNumber] Page number is out of range: " << std::hex << page << std::endl;
    }
    if (page<0x10000) page=0x10000;
    if (page>0x8ff99) page=0x8ff99;
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
 * @todo This would be a better version?
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
