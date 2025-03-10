/** Implements a packet source for magazines
 */

#include "packetmag.h"

using namespace vbit;

PacketMag::PacketMag(uint8_t mag, std::list<TTXPageStream>* pageSet, ttx::Configure *configure, Debug *debug, uint8_t priority) :
    _pageSet(pageSet),
    _configure(configure),
    _debug(debug),
    _page(nullptr),
    _subpage(nullptr),
    _magNumber(mag),
    _priority(priority),
    _priorityCount(priority),
    _state(PACKETSTATE_HEADER),
    _thisRow(0),
    _lastTxt(nullptr),
    _nextPacket29DC(0),
    _hasPacket29(false),
    _hasCustomHeader(false),
    _magRegion(0),
    _specialPagesFlipFlop(false),
    _waitingForField(0),
    _cycleDuration(-1)
{
    //ctor
    _lastCycleTimestamp = {0,0};
    
    for (int i=0;i<MAXPACKET29TYPES;i++)
    {
        _headerTemplate = configure->GetHeaderTemplate();
        _packet29[i]=nullptr;
    }

    _carousel=new vbit::Carousel(_debug);
    _specialPages=new vbit::SpecialPages(_debug);
    _normalPages=new vbit::NormalPages(_debug);
    _updatedPages=new vbit::UpdatedPages(_debug);
}

PacketMag::~PacketMag()
{
    //dtor
    delete _carousel;
    delete _specialPages;
    delete _normalPages;
    delete _updatedPages;
}

Packet* PacketMag::GetPacket(Packet* p)
{
    int thisPageNum;
    unsigned int thisSubcode;
    int* links=NULL;
    bool updatedFlag=false;
    
    static vbit::Packet* TempPacket=new vbit::Packet(8,25,"                                        "); // a temporary packet for checksum calculation

    // We should only call GetPacket if IsReady has returned true

    // no pages
    if (_pageSet->size()<1)
    {
        return nullptr;
    }

    switch (_state)
    {
        case PACKETSTATE_HEADER: // Start to send out a new page, which may be a simple page or one of a carousel
        {
            if (GetEvent(EVENT_PACKET_29) && _hasPacket29)
            {
                if (_mtx.try_lock()) // skip if unable to get lock
                {
                    while (_packet29[_nextPacket29DC] == nullptr)
                    {
                        _nextPacket29DC++;
                    }
                    
                    if (_nextPacket29DC >= MAXPACKET29TYPES)
                    {
                        _nextPacket29DC = 0;
                        ClearEvent(EVENT_PACKET_29);
                    }
                    else
                    {
                        p->SetRow(_magNumber, 29, _packet29[_nextPacket29DC]->GetLine(), CODING_13_TRIPLETS);
                        _nextPacket29DC++;
                        _mtx.unlock(); // unlock before we return!
                        return p;
                    }
                    _mtx.unlock(); // we got a lock so unlock again
                }
            }
            _specialPagesFlipFlop = !_specialPagesFlipFlop; // toggle the flag so that we interleave special pages and regular pages during the special pages event so that rolling headers aren't stopped completely
            if (GetEvent(EVENT_SPECIAL_PAGES) && _specialPagesFlipFlop)
            {
                _page=_specialPages->NextPage();
                
                if (_page)
                {
                    // got a special page
                    
                    if (_page->GetPageFunction() == MIP)
                    {
                        // Magazine Inventory Page
                        _waitingForField = 2; // enforce 20ms page erasure interval
                    }
                    
                    if (_page->IsCarousel())
                        _subpage = _page->GetCarouselPage();
                    else
                        _subpage = _page;
                    
                    _status = _subpage->GetPageStatus() & 0x8000; // get transmit flag
                    _region = _subpage->GetRegion();
                    thisSubcode = (_subpage->GetSubCode() & 0x000F) | (_subpage->GetLastPacket() << 8);
                    
                    thisSubcode |= _page->GetUpdateCount() << 4;
                    
                    /* rules for the control bits are complicated. There are rules to allow the page to be sent as fragments. Since we aren't doing that, all the flags are left clear except for C9 (interrupted sequence) to keep special pages out of rolling headers */
                    _status |= 0x0010;
                    /* rules for the subcode are really complicated. The S1 nibble should be the sub page number, S2 is a counter that increments when the page is updated, S3 and S4 hold the last row number */
                }
                else
                {
                    // got to the end of the special pages
                    ClearEvent(EVENT_SPECIAL_PAGES);
                    return nullptr;
                }
            }
            else
            {
                _page=_updatedPages->NextPage(); // Get the next updated page (if there is one)
                if (_page)
                {
                    // updated page
                    updatedFlag=true; // use our own flag because the pagestream's _isUpdated flag gets cleared by NextPage
                }
                else
                {
                    // no updated pages
                    _page=_carousel->nextCarousel(); // Get the next carousel page (if there is one)
                    if (_page)
                    {
                        // carousel with hard timings
                    }
                    else
                    {
                        // no urgent carousels
                        _page=_normalPages->NextPage();  // Get the next normal page (if there is one)
                    }
                }
                
                if (_page == nullptr)
                {
                    // reached the end of a magazine cycle
                    // get master clock singleton
                    MasterClock *mc = mc->Instance();
                    MasterClock::timeStruct t = mc->GetMasterClock();
                    if (_lastCycleTimestamp.seconds){ // wait for real timestamps
                        int diffSeconds = difftime(t.seconds, _lastCycleTimestamp.seconds); // truncates double to int
                        _cycleDuration = ((diffSeconds * 50) - _lastCycleTimestamp.fields) + t.fields;
                        _debug->SetMagCycleDuration(_magNumber, _cycleDuration);
                    }
                    _lastCycleTimestamp = t; // update timestamp
                    
                    // couldn't get a page to send so sent a time filling header
                    p->Header(_magNumber,0xFF,0x0000,0x8010,_headerTemplate);
                    _waitingForField = 2; // enforce 20ms page erasure interval
                    return p;
                }
                
                _thisRow=0;
                
                if (_page->IsCarousel())
                {
                    if (_page->Expired(true))
                    {
                        // cycle if timer has expired
                        _page->StepNextSubpage();
                        _page->SetTransitionTime(_page->GetCarouselPage()->GetCycleTime());
                        _status=_page->GetCarouselPage()->GetPageStatus();
                    }
                    else
                    {
                        // clear any ERASE bit if page hasn't cycled to minimise flicker, and the interrupted status bit
                        _status=_page->GetCarouselPage()->GetPageStatus() & ~(PAGESTATUS_C4_ERASEPAGE | PAGESTATUS_C9_INTERRUPTED);
                    }
                    
                    _subpage = _page->GetCarouselPage();
                    
                    thisSubcode=_subpage->GetSubCode();
                    _region=_subpage->GetRegion();
                }
                else
                {
                    _subpage = _page;
                    
                    thisSubcode=_subpage->GetSubCode();
                    _status=_subpage->GetPageStatus();
                    _region=_subpage->GetRegion();
                }
                
                // Handle pages with update bit set in a useful way.
                // This isn't defined by the specification.
                if (_status & PAGESTATUS_C8_UPDATE)
                {
                    // Clear update bit in stored page so that update flag is only transmitted once
                    _subpage->SetPageStatus(_subpage->GetPageStatus() & ~PAGESTATUS_C8_UPDATE);
                    
                    // Also set the erase flag in output. This will allow left over rows in adaptive transmission to be cleared without leaving the erase flag set causing flickering.
                    _status|=PAGESTATUS_C4_ERASEPAGE;
                }
                
                if (updatedFlag)
                {
                    // page is updated set interrupted sequence flag
                    _status|=PAGESTATUS_C9_INTERRUPTED;
                }
            }
            
            // Assemble the header. (we can simplify this code or leave it for the optimiser)
            thisPageNum=_page->GetPageNumber();
            thisPageNum=(thisPageNum/0x100) % 0x100; // Remove this line for Continuous Random Acquisition of Pages.
            
            if (!(_status & 0x8000))
            {
                _page=nullptr;
                return nullptr;
            }
            
            _waitingForField = 2; // enforce 20ms page erasure interval
            
            // clear a flag we use to prevent duplicated X/28/0 packets
            _hasX28Region = false;
            p->Header(_magNumber,thisPageNum,thisSubcode,_status,_headerTemplate);// loads of stuff to do here!
            
            uint16_t tempCRC = p->PacketCRC(0); // calculate the crc of the new header
            
            bool headerChanged = _subpage->HasHeaderChanged(tempCRC);
            bool fileChanged = _subpage->HasFileChanged();
            if (headerChanged || fileChanged)
            {
                // the content of the header has changed or the page has been reloaded
                // we must now CRC the whole page
                
                for (int i=1; i<26; i++)
                {
                    TempPacket->SetRow(_magNumber, _thisRow, _subpage->GetRow(i)->GetLine(), _subpage->GetPageCoding());
                    tempCRC = TempPacket->PacketCRC(tempCRC);
                }
                
                _subpage->SetPageCRC(tempCRC);
                
                // TODO: the page content may get modified by substitutions in Packet::tx() which will result in an invalid checksum
            }
            
            assert(p!=NULL);

            links=_subpage->GetLinkSet();
            if ((links[0] & links[1] & links[2] & links[3] & links[4] & links[5]) != 0x8FF) // only create if links were initialised
            {
                _state=PACKETSTATE_FASTEXT;
                break;
            }
            else
            {
                _lastTxt=_page->GetTxRow(27); // Get _lastTxt ready for packet 27 processing
                _state=PACKETSTATE_PACKET27;
                break;
            }
        }
        case PACKETSTATE_PACKET27:
        {
            if (_lastTxt)
            {
                if ((_lastTxt->GetLine()[0] & 0xF) > 3) // designation codes > 3
                    p->SetRow(_magNumber, 27, _lastTxt->GetLine(), CODING_13_TRIPLETS); // enhancement linking
                else
                {
                    p->SetRow(_magNumber, 27, _lastTxt->GetLine(), CODING_HAMMING_8_4); // navigation packets
                    p->SetX27CRC(_subpage->GetPageCRC());
                }
                _lastTxt=_lastTxt->GetNextLine();
                break;
            }
            _lastTxt=_page->GetTxRow(28); // Get _lastTxt ready for packet 28 processing
            _state=PACKETSTATE_PACKET28; //  // Intentional fall through to PACKETSTATE_PACKET28
            /* fallthrough */
            [[gnu::fallthrough]];
        }
        case PACKETSTATE_PACKET28:
        {
            if (_lastTxt)
            {
                p->SetRow(_magNumber, 28, _lastTxt->GetLine(), CODING_13_TRIPLETS);
                if ((_lastTxt->GetCharAt(0) & 0xF) == 0 || (_lastTxt->GetCharAt(0) & 0xF) == 4)
                    _hasX28Region = true; // don't generate an X/28/0 for a RE line
                _lastTxt=_lastTxt->GetNextLine();
                break;
            }
            else if (!(_hasX28Region) && (_region != _magRegion))
            {
                // create X/28/0 packet for pages which have a region set with RE in file
                // this could almost certainly be done more efficiently but it's quite confusing and this is more readable for when it all goes wrong.
                std::string val = "@@@tGpCuW@twwCpRA`UBWwDsWwuwwwUwWwuWwE@@"; // default X/28/0 packet
                int NOS = (_status & 0x380) >> 7;
                int language = NOS | (_region << 3);
                int triplet = 0x3C000 | (language << 7); // construct triplet 1
                val.replace(1,1,1,(triplet & 0x3F) | 0x40);
                val.replace(2,1,1,((triplet & 0xFC0) >> 6) | 0x40);
                val.replace(3,1,1,((triplet & 0x3F000) >> 12) | 0x40);
                p->SetRow(_magNumber, 28, val, CODING_13_TRIPLETS);
                _lastTxt=_page->GetTxRow(26); // Get _lastTxt ready for packet 26 processing
                _state=PACKETSTATE_PACKET26;
                break;
            }
            else if (_page->GetPageCoding() == CODING_7BIT_TEXT)
            {
                // X/26 packets next in normal pages
                _lastTxt=_page->GetTxRow(26); // Get _lastTxt ready for packet 26 processing
                _state=PACKETSTATE_PACKET26; // Intentional fall through to PACKETSTATE_PACKET26
            }
            else
            {
                // do X/1 to X/25 first and go back to X/26 after
                _state=PACKETSTATE_TEXTROW;
                return nullptr;
            }
            /* fallthrough */
            [[gnu::fallthrough]];
        }
        case PACKETSTATE_PACKET26:
        {
            if (_lastTxt)
            {
                p->SetRow(_magNumber, 26, _lastTxt->GetLine(), CODING_13_TRIPLETS);
                // Do we have another line?
                _lastTxt=_lastTxt->GetNextLine();
                break;
            }
            if (_page->GetPageCoding() == CODING_7BIT_TEXT)
            {
                _state=PACKETSTATE_TEXTROW; // Intentional fall through to PACKETSTATE_TEXTROW
            }
            else
            {
                // otherwise we end the page here
                _state=PACKETSTATE_HEADER;
                _thisRow=0;
                return nullptr;
            }
            /* fallthrough */
            [[gnu::fallthrough]];
        }
        case PACKETSTATE_TEXTROW:
        {
            // Find the next row that isn't NULL
            for (_thisRow++;_thisRow<26;_thisRow++)
            {
                _lastTxt=_page->GetTxRow(_thisRow);
                if (_lastTxt!=NULL)
                    break;
            }

            // Didn't find? End of this page.
            if (_thisRow>25 || _lastTxt==NULL)
            {
                if(_page->GetPageCoding() == CODING_7BIT_TEXT)
                {
                    // if this is a normal page we've finished
                    _state=PACKETSTATE_HEADER;
                    _thisRow=0;
                }
                else
                {
                    // otherwise go on to X/26
                    _lastTxt=_page->GetTxRow(26);
                    _state=PACKETSTATE_PACKET26;
                }
                return nullptr;
            }
            else
            {
            //_outp("J");
                if (_lastTxt->IsBlank() && (_configure->GetRowAdaptive() || _page->GetPageFunction() != LOP)) // If a row is empty then skip it if row adaptive mode on, or not a level 1 page
                {
                    return nullptr;
                }
                else
                {
                    // Assemble the packet
                    p->SetRow(_magNumber, _thisRow, _lastTxt->GetLine(), _page->GetPageCoding());
                    assert(p->IsHeader()!=true);
                }
            }
            break;
        }
        case PACKETSTATE_FASTEXT:
        {
            p->SetMRAG(_magNumber,27);
            links=_subpage->GetLinkSet();
            p->Fastext(links,_magNumber);
            p->SetX27CRC(_subpage->GetPageCRC());
            _lastTxt=_page->GetTxRow(27); // Get _lastTxt ready for packet 27 processing
            _state=PACKETSTATE_PACKET27; // makes no attempt to prevent an FL row and an X/27/0 both being sent
            break;
        }
        default:
        {
            _state=PACKETSTATE_HEADER;// For now, do the next page
            return nullptr;
        }
    }

    return p; //
}

/** Is there a packet ready to go?
 *  If the ready flag is set
 *  and the priority count allows a packet to go out
 *  @param force - If true AND if the next packet is being held back due to priority, send the packet anyway
 */
bool PacketMag::IsReady(bool force)
{
    bool result=false;
    // We can always send something unless
    // 1) We have just sent out a header and are waiting on a new field
    // 2) There are no pages
    if (GetEvent(EVENT_FIELD))
    {
        ClearEvent(EVENT_FIELD);
        if (_waitingForField > 0)
        {
            // _waitingForField is set to 2 when the last packet was a header but field event is not yet cleared
            // on the next IsReady we clear the event and decrement it ready to wait for the next field event.
            _waitingForField--;
        }
    }
    
    if (_waitingForField == 0)
    {
        _priorityCount--;
        if (_priorityCount==0 || force || (_updatedPages->waiting())) // force if there are updated pages waiting
        {
            _priorityCount=_priority;
            result=true;
        }
    }
    
    if (_pageSet->size()>0)
    {
        return result;
    }
    else
    {
        return false;
    }
};

void PacketMag::SetPacket29(int i, TTXLine *line)
{
    _packet29[i] = line;
    _hasPacket29 = true;
    
    if (_packet29[0])
    {
        _magRegion = ((_packet29[0]->GetCharAt(2) & 0x30) >> 4) | ((_packet29[0]->GetCharAt(3) & 0x3) << 2);
    } 
    else if (_packet29[2])
    {
        _magRegion = ((_packet29[2]->GetCharAt(2) & 0x30) >> 4) | ((_packet29[2]->GetCharAt(3) & 0x3) << 2);
    }
}

void PacketMag::DeletePacket29()
{
    _mtx.lock();
    for (int i=0;i<MAXPACKET29TYPES;i++)
    {
        if (_packet29[i] != nullptr)
        {
            delete _packet29[i]; // delete TTXLine created in PageList::CheckForPacket29
            _packet29[i] = nullptr;
        }
    }
    _hasPacket29 = false;
    _mtx.unlock();
}

void PacketMag::DeleteCustomHeader()
{
    _headerTemplate = _configure->GetHeaderTemplate(); // revert to service default template
    _hasCustomHeader = false;
     _debug->Log(Debug::LogLevels::logINFO,"[PacketMag::DeleteCustomHeader] Removing custom header from magazine " + std::to_string(_magNumber));
}
