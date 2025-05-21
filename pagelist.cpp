/** PageList
 */
#include "pagelist.h"
#include "packetmag.h"

using namespace vbit;

PageList::PageList(Configure *configure, Debug *debug) :
    _configure(configure),
    _debug(debug)
{
    for (int i=0;i<8;i++)
    {
        _mag[i]=nullptr;
    }
    if (_configure==nullptr)
    {
        _debug->Log(Debug::LogLevels::logERROR,"NULL configuration object");
        return;
    }
    
    for (int i=0;i<8;i++)
    {
        _mag[i]=new PacketMag(i, this, _configure, _debug, 9); // this creates the eight PacketMags that Service will use. Priority will be set in Service later
    }
}

PageList::~PageList()
{
}

void PageList::AddPage(std::shared_ptr<TTXPageStream> page, bool noupdate)
{
    int num = page->GetPageNumber();
    int mag = (num >> 8) & 7;
    
    if ((num & 0xFF) != 0xFF)
    {
        // never load page mFF into page lists
        std::shared_ptr<TTXPageStream> q = Locate(num);
        if (q)
        {
            _pageList[mag].remove(q);
            q->MarkForDeletion();
            
            std::stringstream ss;
            ss << "[PageList::AddPage] Replacing page " << std::hex << num;
            _debug->Log(Debug::LogLevels::logERROR,ss.str());
        }

        _pageList[mag].push_back(page);
        UpdatePageLists(page, noupdate);
    
        _debug->SetMagazineSize(mag, _pageList[mag].size());
    }
}

void PageList::UpdatePageLists(std::shared_ptr<TTXPageStream> page, bool noupdate)
{
    int mag=(page->GetPageNumber() >> 8) & 0x7;
    
    if (page->Special())
    {
        // Page is 'special'
        if (!(page->GetSpecialFlag()))
        {
            if (page->GetNormalFlag())
            {
                std::stringstream ss;
                ss << "[PageList::UpdatePageLists] page was normal, is now special " << std::hex << (page->GetPageNumber());
                _debug->Log(Debug::LogLevels::logINFO,ss.str());
            }
            _mag[mag]->GetSpecialPages()->addPage(page);
        }
        
        page->SetNormalFlag(false);
        page->SetCarouselFlag(false);
        page->SetUpdatedFlag(false);
    }
    else
    {
        // Page is 'normal'
        if (!(page->GetNormalFlag()))
        {
            if (page->GetSpecialFlag())
            {
                std::stringstream ss;
                ss << "[PageList::UpdatePageLists] page was special, is now normal " << std::hex << (page->GetPageNumber());
                _debug->Log(Debug::LogLevels::logINFO,ss.str());
            }
            
            _mag[mag]->GetNormalPages()->addPage(page);
        }
        page->SetSpecialFlag(false);
        
        if (page->IsCarousel())
        {
            // Page is also a 'carousel'
            if (!(page->GetCarouselFlag()))
            {
                std::stringstream ss;
                ss << "[PageList::UpdatePageLists] page is now a carousel " << std::hex << (page->GetPageNumber());
                _debug->Log(Debug::LogLevels::logINFO,ss.str());
                page->StepNextSubpage(); // ensure we're pointing at a subpage
                _mag[mag]->GetCarousel()->addPage(page);
            }
            page->SetUpdatedFlag(false);
        }
        else
        {
            // add normal, non carousel pages to updatedPages list
            if ((!(page->GetUpdatedFlag())) && !noupdate)
            {
                _mag[mag]->GetUpdatedPages()->addPage(page);
            }
            else
            {
                page->SetUpdatedFlag(false);
            }
            page->SetCarouselFlag(false);
        }
    }
}

void PageList::RemovePage(std::shared_ptr<TTXPageStream> page)
{
    if (!(page->GetCarouselFlag() || page->GetNormalFlag() || page->GetSpecialFlag() || page->GetUpdatedFlag()))
    {
        // page has been removed from all of the page type lists
        
        int mag=(page->GetPageNumber() >> 8) & 0x7;
        _pageList[mag].remove(page);
        _debug->SetMagazineSize(mag, _pageList[mag].size());
        
        std::stringstream ss;
        ss << "[PageList::RemovePage] Deleted " << std::hex << (page->GetPageNumber());
        _debug->Log(Debug::LogLevels::logINFO,ss.str());
    }
    page->FreeLock(); // free the lock on page
}

void PageList::CheckForPacket29OrCustomHeader(std::shared_ptr<TTXPageStream> page)
{
    if (page->IsCarousel()) // page mFF should never be a carousel and this code leads to a crash if it is so bail out now
        return;
    
    int mag=(page->GetPageNumber() >> 8) & 0x7;
    if ((page->GetPageNumber() & 0xFF) == 0xFF) // Only read from page mFF
    {
        /* attempt to load custom header from the page */
        if ((!_mag[mag]->GetCustomHeaderFlag()) || page->GetCustomHeaderFlag()) // Only allow one file to set header per magazine
        {
            if (page->GetTxRow(0)){
                _mag[mag]->SetCustomHeader(page->GetTxRow(0)->GetLine().substr(8,32)); // set custom headers
                
                if (!page->GetCustomHeaderFlag())
                    _debug->Log(Debug::LogLevels::logINFO,"[PageList::CheckForPacket29OrCustomHeader] Added custom header for magazine " + std::to_string((mag == 0)?8:mag));
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
            
            std::shared_ptr<TTXLine> tempLine = page->GetTxRow(29);
            
            while (tempLine != nullptr)
            {
                switch (tempLine->GetCharAt(0))
                {
                    case '@':
                    {
                        Packet29Flag = true;
                        _mag[mag]->SetPacket29(0, std::shared_ptr<TTXLine>(new TTXLine(tempLine->GetLine(), true)));
                        break;
                    }
                    case 'A':
                    {
                        Packet29Flag = true;
                        _mag[mag]->SetPacket29(1, std::shared_ptr<TTXLine>(new TTXLine(tempLine->GetLine(), true)));
                        break;
                    }
                    case 'D':
                    {
                        Packet29Flag = true;
                        _mag[mag]->SetPacket29(2, std::shared_ptr<TTXLine>(new TTXLine(tempLine->GetLine(), true)));
                        break;
                    }
                }
                
                tempLine = tempLine->GetNextLine();
                // loop until every row 29 is copied
            }
            if (Packet29Flag && !page->GetPacket29Flag())
                _debug->Log(Debug::LogLevels::logINFO,"[PageList::CheckForPacket29OrCustomHeader] Added packet 29 for magazine "+ std::to_string((mag == 0)?8:mag));
            
            page->SetPacket29Flag(Packet29Flag); // mark the page
            
            
        }
    }
}

// Find a page by number - Warning: this will only find the first match so don't let multiples into the list!
std::shared_ptr<TTXPageStream> PageList::Locate(int PageNumber)
{
    // This is called from the FileMonitor thread
    int mag = (PageNumber >> 8) & 7;
    for (std::list<std::shared_ptr<TTXPageStream>>::iterator p=_pageList[mag].begin();p!=_pageList[mag].end();++p)
    {
        std::shared_ptr<TTXPageStream> ptr = *p;
        if (PageNumber==ptr->GetPageNumber())
            return ptr;
    }
    return nullptr;
}

// Does the page list contain a particular TTXPageStream
bool PageList::Contains(std::shared_ptr<TTXPageStream> page)
{
    // This is called from the FileMonitor thread
    int mag = (page->GetPageNumber() >> 8) & 7;
    for (std::list<std::shared_ptr<TTXPageStream>>::iterator p=_pageList[mag].begin();p!=_pageList[mag].end();++p)
    {
        if (*p==page)
            return true;
    }
    return false;
}

int PageList::GetSize(int mag)
{
    if (mag < 8 && mag >= 0)
        return _pageList[mag].size();
    else
        return 0;
}
