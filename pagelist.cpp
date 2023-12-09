/** PageList
 */
#include "pagelist.h"

using namespace ttx;

PageList::PageList(Configure *configure) :
    _configure(configure),
    _iterMag(0),
    _iterSubpage(nullptr)
{
    for (int i=0;i<8;i++)
    {
        _mag[i]=nullptr;
    }
    if (_configure==nullptr)
    {
        std::cerr << "NULL configuration object" << std::endl;
        return;
    }
    LoadPageList(_configure->GetPageDirectory());
}

PageList::~PageList()
{
}

int PageList::LoadPageList(std::string filepath)
{
    // Create PacketMags before loading
    for (int i=0;i<8;i++)
    {
        _mag[i]=new vbit::PacketMag(i, &_pageList[i], _configure, 9); // this creates the eight PacketMags that Service will use. Priority will be set in Service later
    }
    
    // Load files
    if (ReadDirectory(filepath))
        return errno;
    
    PopulatePageTypeLists(); // add pages to the appropriate lists for their type
    
    return 0;
}

int PageList::ReadDirectory(std::string filepath)
{
    DIR *dp;
    TTXPageStream* q;
    struct dirent *dirp;
    struct stat attrib;
    
    // Open the directory
    if ((dp = opendir(filepath.c_str())) == NULL)
    {
        std::cerr << "Error(" << errno << ") opening " << filepath << std::endl;
        return errno;
    }
    
    // Load the filenames into a list
    while ((dirp = readdir(dp)) != NULL)
    {
        std::string name;
        name=filepath;
        name+="/";
        name+=dirp->d_name;
        
        if (stat(name.c_str(), &attrib) == -1) // get the attributes of the file
            continue; // skip file on failure
        
        if (attrib.st_mode & S_IFDIR)
        {
            // directory entry is another directory
            if (dirp->d_name[0] != '.') // ignore anything beginning with .
            {
                std::cerr << "[PageList::LoadPageList] recursing into " << name << std::endl;
                if (ReadDirectory(name)) // recurse into directory
                {
                    std::cerr << "Error(" << errno << ") recursing into " << filepath << std::endl;
                }
            }
            continue;
        }
        
        //p=new TTXPageStream(filepath+"/"+dirp->d_name);
        if (std::string(dirp->d_name).find(".tti") != std::string::npos) // Is the file type .tti or ttix?
        {
            q=new TTXPageStream(filepath+"/"+dirp->d_name);
            // If the page loaded, then push it into the appropriate magazine
            if (q->Loaded())
            {
                q->GetPageCount(); // Use for the side effect of renumbering the subcodes
                
                CheckForPacket29OrCustomHeader(q);
            }
            
            int mag=(q->GetPageNumber() >> 16) & 0x7;
            _pageList[mag].push_back(*q); // This copies. But we can't copy a mutex
            
            
        }
    }
    closedir(dp);
    
    return 0;
}

void PageList::AddPage(TTXPageStream* page)
{
    int mag=(page->GetPageNumber() >> 16) & 0x7;
    _pageList[mag].push_back(*page);
}

void PageList::CheckForPacket29OrCustomHeader(TTXPageStream* page)
{
    if (page->IsCarousel()) // page mFF should never be a carousel and this code leads to a crash if it is so bail out now
        return;
    
    int mag=(page->GetPageNumber() >> 16) & 0x7;
    if (((page->GetPageNumber() >> 8) & 0xFF) == 0xFF) // Only read from page mFF
    {
        /* attempt to load custom header from the page */
        if ((!_mag[mag]->GetCustomHeaderFlag()) || page->GetCustomHeaderFlag()) // Only allow one file to set header per magazine
        {
            if (page->GetTxRow(0)){
                _mag[mag]->SetCustomHeader(page->GetTxRow(0)->GetLine().substr(8,32)); // set custom headers
                
                if (!page->GetCustomHeaderFlag())
                    std::cerr << "[PageList::CheckForPacket29OrCustomHeader] Added custom header for magazine " << ((mag == 0)?8:mag) << std::endl;
                page->SetCustomHeaderFlag(true); // mark the page
            }
            else if (page->GetCustomHeaderFlag()) // page previously had custom header
            {
                _mag[mag]->DeleteCustomHeader();
                page->SetCustomHeaderFlag(false);
            }
        }
        
        /* attempt to load packet M/29 data from the page */
        if ((!_mag[mag]->GetPacket29Flag()) || page->GetPacket29Flag()) // Only allow one file to set packet29 per magazine
        {
            bool Packet29Flag = false;
            
            if (page->GetPacket29Flag())
                _mag[mag]->DeletePacket29(); // clear previous packet 29
            
            TTXLine* tempLine = page->GetTxRow(29);
            
            while (tempLine != nullptr)
            {
                switch (tempLine->GetCharAt(0))
                {
                    case '@':
                    {
                        Packet29Flag = true;
                        _mag[mag]->SetPacket29(0, new TTXLine(tempLine->GetLine(), true));
                        break;
                    }
                    case 'A':
                    {
                        Packet29Flag = true;
                        _mag[mag]->SetPacket29(1, new TTXLine(tempLine->GetLine(), true));
                        break;
                    }
                    case 'D':
                    {
                        Packet29Flag = true;
                        _mag[mag]->SetPacket29(2, new TTXLine(tempLine->GetLine(), true));
                        break;
                    }
                }
                
                tempLine = tempLine->GetNextLine();
                // loop until every row 29 is copied
            }
            if (Packet29Flag && !page->GetPacket29Flag())
                std::cerr << "[PageList::CheckForPacket29OrCustomHeader] Added packet 29 for magazine " << ((mag == 0)?8:mag) << std::endl;
            
            page->SetPacket29Flag(Packet29Flag); // mark the page
            
            
        }
    }
}

// Find a page by filename
TTXPageStream* PageList::Locate(std::string filename)
{
    // This is called from the FileMonitor thread
    for (int mag=0;mag<8;mag++)
    {
        //for (auto p : _pageList[mag])
        for (std::list<TTXPageStream>::iterator p=_pageList[mag].begin();p!=_pageList[mag].end();++p)
        {
            TTXPageStream* ptr;
            ptr=&(*p);
            // std::cerr << "[PageList::Locate]scan:" << ptr->GetSourcePage() << std::endl;
            if (filename==ptr->GetSourcePage())
            return ptr;
        }
    }
    return NULL; // @todo placeholder What should we do here?
}

int PageList::Match(char* pageNumber)
{
    int matchCount=0;

    std::cerr << "[PageList::Match] Selecting " << pageNumber << std::endl;
    int begin=0;
    int end=7;

    for (int mag=begin;mag<end+1;mag++)
    {
        // For each page
        for (std::list<TTXPageStream>::iterator p=_pageList[mag].begin();p!=_pageList[mag].end();++p)
        {
            TTXPageStream* ptr;
            char s[6];
            char* ps=s;
            bool match=true;
            for (ptr=&(*p);ptr!=nullptr;ptr=(TTXPageStream*)ptr->Getm_SubPage()) // For all the subpages in a carousel
            {
                // Convert the page number into a string so we can compare it
                std::stringstream ss;
                ss << std::hex << std::uppercase << std::setw(5) << ptr->GetPageNumber();
                strcpy(ps,ss.str().c_str());
                // std::cerr << "[PageList::Match] matching " << ps << std::endl;

                for (int i=0;i<5;i++)
                {
                    if (pageNumber[i]!='*') // wildcard
                    {
                        if (pageNumber[i]!=ps[i])
                        {
                            match=false;
                        }
                    }
                }
                
                if (match)
                {
                    matchCount++;
                }
                
                ptr->SetSelected(match);
            }
        }
    }
    // Set up the iterator for commands that use pages selected by the Page Identity
    _iterMag=0;
    _iter=_pageList[_iterMag].begin();
    
    return matchCount; // final count
}

TTXPageStream* PageList::NextPage()
{
    std::cerr << "[PageList::NextPage] looking for a selected page, mag=" << (int)_iterMag << std::endl;
    bool more=true;
    if (_iterSubpage!=nullptr)
    {
        std::cerr << "A";
        _iterSubpage=(TTXPageStream*) _iterSubpage->Getm_SubPage();
        std::cerr << "B";
    }
    if (_iterSubpage!=nullptr)
    {
        std::cerr << "C";
        return _iterSubpage;
    }
    std::cerr << "[PageList::NextPage] _iterSubpage is null, so checking next page" << std::endl;

    if (_iter!=_pageList[_iterMag].end())
    {
        ++_iter; // Next page
    }
    if (_iter==_pageList[_iterMag].end()) // end of mag?
    {
        if (_iterMag<7)
        {
            _iterMag++; // next mag
            _iter=_pageList[_iterMag].begin();
        }
        else
        {
            more=false; // End of last mag
        }
    }

    if (more)
    {
        _iterSubpage=&(*_iter);
        return _iterSubpage;
    }
    _iterSubpage=nullptr;
    return nullptr; // Returned after the last page is iterated
}

TTXPageStream* PageList::PrevPage()
{
    //std::cerr << "[PageList::NextPage] looking for a selected page, mag=" << (int)_iterMag << std::endl;
    bool more=true;
    if (_iter!=_pageList[_iterMag].begin())
    {
        --_iter; // Previous page
    }

    if (_iter==_pageList[_iterMag].begin()) // beginning of mag?
    {
        if (_iterMag<=0)
        {
            _iterMag--; // previous mag
            _iter=_pageList[_iterMag].end();
        }
        else
        {
            more=false;
        }
    }

    if (more)
    {
        return &(*_iter);
    }
    return nullptr; // Returned after the first page is iterated
}

TTXPageStream* PageList::FirstPage()
{
    // Reset the iterators
    _iterMag=0;
    _iter=_pageList[_iterMag].begin();
    _iterSubpage=&(*_iter);
    // Iterate through all the pages
    std::cerr << "[PageList::FirstPage] about to find if there is a selected page" << std::endl;
    for (TTXPageStream* p=_iterSubpage; p!=nullptr; p=NextPage())
    {
        if (p->Selected()) // If the page is selected, return a pointer to it
        {
            std::cerr << "[PageList::FirstPage] selected page found" << std::endl;
            return p;
        }
    }
    std::cerr << "[PageList::FirstPage] no selected page" << std::endl;
    return nullptr; // No selected page
}

TTXPageStream* PageList::LastPage()
{
    // Reset the iterators
    _iterMag=7;
    _iter=_pageList[_iterMag].end();
    // Iterate through all the pages
    for (TTXPageStream* p=&(*_iter); p!=nullptr; p=PrevPage())
    {
        if (p->Selected()) // If the page is selected, return a pointer to it
        {
            return p;
        }
    }
    return nullptr; // No selected page
}

TTXPageStream* PageList::NextSelectedPage()
{
    TTXPageStream* page;
    for(;;)
    {
        page=NextPage();
        if (page==nullptr || page->Selected())
        {
            return page;
        }
    }
}

// Detect pages that have been deleted from the drive
// Do this by first clearing all the "exists" flags
// As we scan through the list, set the "exists" flag as we match up the drive to the loaded page
void PageList::ClearFlags()
{
    for (int mag=0;mag<8;mag++)
    {
        for (std::list<TTXPageStream>::iterator p=_pageList[mag].begin();p!=_pageList[mag].end();++p)
        {
            TTXPageStream* ptr;
            ptr=&(*p);
            // Don't unmark a file that was MARKED. Once condemned it won't be pardoned
            if (ptr->GetStatusFlag()==TTXPageStream::FOUND || ptr->GetStatusFlag()==TTXPageStream::NEW)
            {
                ptr->SetState(TTXPageStream::NOTFOUND);
            }
        }
    }
}

void PageList::DeleteOldPages()
{
    // This is called from the FileMonitor thread
    for (int mag=0;mag<8;mag++)
    {
        for (std::list<TTXPageStream>::iterator p=_pageList[mag].begin();p!=_pageList[mag].end();++p)
        {
            TTXPageStream* ptr;
            ptr=&(*p);
            if (ptr->GetStatusFlag()==TTXPageStream::GONE)
            {
                if (ptr->GetPacket29Flag())
                {
                    // Packet 29 was loaded from this page, so remove it.
                    _mag[mag]->DeletePacket29();
                    std::cerr << "[PageList::DeleteOldPages] Removing packet 29 from magazine " << ((mag == 0)?8:mag) << std::endl;
                }
                if (ptr->GetCustomHeaderFlag())
                {
                    // Custom header was loaded from this page, so remove it.
                    _mag[mag]->DeleteCustomHeader();
                }
                
                // page has been removed from lists
                _pageList[mag].remove(*p--);

                if (_iterMag == mag)
                {
                    // _iter is iterating _pageList[mag]
                    _iter=_pageList[_iterMag].begin(); // reset it?
                }
            }
            else if (ptr->GetStatusFlag()==TTXPageStream::NOTFOUND)
            {
                // Pages marked here get deleted in the Service thread
                ptr->SetState(TTXPageStream::MARKED);
            }
        }
    }
}

void PageList::PopulatePageTypeLists()
{
    for (int mag=0;mag<8;mag++)
    {
        for (std::list<TTXPageStream>::iterator p=_pageList[mag].begin();p!=_pageList[mag].end();++p)
        {
            TTXPageStream* ptr;
            ptr=&(*p);
            if (ptr->Special())
            {
                // Page is 'special'
                ptr->SetSpecialFlag(true);
                ptr->SetNormalFlag(false);
                ptr->SetCarouselFlag(false);
                _mag[mag]->GetSpecialPages()->addPage(ptr);
            }
            else
            {
                // Page is 'normal'
                ptr->SetSpecialFlag(false);
                ptr->SetNormalFlag(true);
                _mag[mag]->GetNormalPages()->addPage(ptr);
                
                if (ptr->IsCarousel())
                {
                    // Page is also 'carousel'
                    ptr->SetCarouselFlag(true);
                    _mag[mag]->GetCarousel()->addPage(ptr);
                    ptr->StepNextSubpage();
                }
                else
                    ptr->SetCarouselFlag(false);
            }
        }
    }
}
