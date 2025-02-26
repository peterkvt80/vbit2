/** PageList
 */
#include "pagelist.h"

using namespace ttx;

PageList::PageList(Configure *configure, vbit::Debug *debug) :
    _configure(configure),
    _debug(debug),
    _iterMag(0),
    _iterSubpage(nullptr)
{
    for (int i=0;i<8;i++)
    {
        _mag[i]=nullptr;
    }
    if (_configure==nullptr)
    {
        _debug->Log(vbit::Debug::LogLevels::logERROR,"NULL configuration object");
        return;
    }
    
    for (int i=0;i<8;i++)
    {
        _mag[i]=new vbit::PacketMag(i, &_pageList[i], _configure, _debug, 9); // this creates the eight PacketMags that Service will use. Priority will be set in Service later
    }
}

PageList::~PageList()
{
}

void PageList::AddPage(TTXPageStream* page, bool noupdate)
{
    int mag=(page->GetPageNumber() >> 16) & 0x7;
    _pageList[mag].push_back(*page);
    _debug->SetMagazineSize(mag, _pageList[mag].size());
    
    page->RenumberSubpages();
    
    UpdatePageLists(&(_pageList[mag].back()), noupdate);
}

void PageList::UpdatePageLists(TTXPageStream* page, bool noupdate)
{
    int mag=(page->GetPageNumber() >> 16) & 0x7;
    
    if (page->Special())
    {
        // Page is 'special'
        if (!(page->GetSpecialFlag()))
        {
            if (page->GetNormalFlag())
            {
                page->SetNormalFlag(false);
                page->SetCarouselFlag(false);
                page->SetUpdatedFlag(false);
                std::stringstream ss;
                ss << "[PageList::UpdatePageLists] page was normal, is now special " << std::hex << (page->GetPageNumber() >> 8);
                _debug->Log(vbit::Debug::LogLevels::logINFO,ss.str());
            }
            _mag[mag]->GetSpecialPages()->addPage(page);
        }
    }
    else
    {
        // Page is 'normal'
        if (page->GetSpecialFlag())
        {
            std::stringstream ss;
            ss << "[PageList::UpdatePageLists] page was special, is now normal " << std::hex << (page->GetPageNumber() >> 8);
            _debug->Log(vbit::Debug::LogLevels::logINFO,ss.str());
        }
        
        page->SetSpecialFlag(false);
        _mag[mag]->GetNormalPages()->addPage(page);
        
        if (page->IsCarousel())
        {
            // Page is also a 'carousel'
            if (!(page->GetCarouselFlag()))
            {
                std::stringstream ss;
                ss << "[PageList::UpdatePageLists] page is now a carousel " << std::hex << (page->GetPageNumber() >> 8);
                _debug->Log(vbit::Debug::LogLevels::logINFO,ss.str());
                page->SetUpdatedFlag(false);
                page->StepNextSubpage(); // ensure we're pointing at a subpage
                _mag[mag]->GetCarousel()->addPage(page);
            }
        }
        else
        {
            page->SetCarouselFlag(false);
            // add normal, non carousel pages to updatedPages list
            if ((!(page->GetUpdatedFlag())) && !noupdate)
            {
                _mag[mag]->GetUpdatedPages()->addPage(page);
            }
            else
            {
                page->SetUpdatedFlag(false);
            }
        }
    }
    
    CheckForPacket29OrCustomHeader(page);
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
                    _debug->Log(vbit::Debug::LogLevels::logINFO,"[PageList::CheckForPacket29OrCustomHeader] Added custom header for magazine " + std::to_string((mag == 0)?8:mag));
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
                _debug->Log(vbit::Debug::LogLevels::logINFO,"[PageList::CheckForPacket29OrCustomHeader] Added packet 29 for magazine "+ std::to_string((mag == 0)?8:mag));
            
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
            //_debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::Locate]scan:" + ptr->GetSourcePage());
            if (filename==ptr->GetSourcePage())
            return ptr;
        }
    }
    return NULL; // @todo placeholder What should we do here?
}

int PageList::Match(char* pageNumber)
{
    int matchCount=0;

    _debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::Match] Selecting " + std::string(pageNumber));
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
                //_debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::Match] matching " + std::string(ps));

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
    _debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::NextPage] looking for a selected page, mag=" + std::to_string((int)_iterMag));
    bool more=true;
    if (_iterSubpage!=nullptr)
    {
        _debug->Log(vbit::Debug::LogLevels::logDEBUG,"A");
        _iterSubpage=(TTXPageStream*) _iterSubpage->Getm_SubPage();
        _debug->Log(vbit::Debug::LogLevels::logDEBUG,"B");
    }
    if (_iterSubpage!=nullptr)
    {
        _debug->Log(vbit::Debug::LogLevels::logDEBUG,"C");
        return _iterSubpage;
    }
    _debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::NextPage] _iterSubpage is null, so checking next page");

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
    //_debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::NextPage] looking for a selected page, mag=" + std::to_string((int)_iterMag));
    bool more=true;
    if (_iter!=_pageList[_iterMag].begin())
    {
        --_iter; // Previous page
    }

    if (_iter==_pageList[_iterMag].begin()) // beginning of mag?
    {
        if (_iterMag>0)
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
    _debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::FirstPage] about to find if there is a selected page");
    for (TTXPageStream* p=_iterSubpage; p!=nullptr; p=NextPage())
    {
        if (p->Selected()) // If the page is selected, return a pointer to it
        {
            _debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::FirstPage] selected page found");
            return p;
        }
    }
    _debug->Log(vbit::Debug::LogLevels::logDEBUG,"[PageList::FirstPage] no selected page");
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
                    _debug->Log(vbit::Debug::LogLevels::logINFO,"[PageList::DeleteOldPages] Removing packet 29 from magazine " + std::to_string((mag == 0)?8:mag));
                }
                if (ptr->GetCustomHeaderFlag())
                {
                    // Custom header was loaded from this page, so remove it.
                    _mag[mag]->DeleteCustomHeader();
                }
                
                // page has been removed from lists
                _pageList[mag].remove(*p--); // this finally deletes the pagestream/page object
                _debug->SetMagazineSize(mag, _pageList[mag].size());

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
