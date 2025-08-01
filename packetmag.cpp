/** Implements a packet source for magazines
 */

#include "packetmag.h"

using namespace vbit;

PacketMag::PacketMag(uint8_t mag, PageList *pageList, Configure *configure, Debug *debug, uint8_t priority) :
    _pageList(pageList),
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
    _packet29(nullptr),
    _nextPacket29(nullptr),
    _hasCustomHeader(false),
    _magRegion(0),
    _specialPagesFlipFlop(false),
    _waitingForField(0),
    _cycleDuration(-1)
{
    //ctor
    _lastCycleTimestamp = {0,0};
    
    _carousel=new Carousel(_magNumber, _pageList, _debug);
    _specialPages=new SpecialPages(_magNumber, _pageList, _debug);
    _normalPages=new NormalPages(_magNumber, _pageList, _debug);
    _updatedPages=new UpdatedPages(_magNumber, _pageList, _debug);
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
    unsigned int thisSubcode;
    bool updatedFlag=false;

    // We should only call GetPacket if IsReady has returned true

    // no pages
    if (_pageList->GetSize(_magNumber)<1)
    {
        return nullptr;
    }

loopback: // jump back point to avoid returning null packets when we could send something
    switch (_state)
    {
        case PACKETSTATE_HEADER: // Start to send out a new page, which may be a simple page or one of a carousel
        {
            _waitingForField = true; // enforce 20ms page erasure interval
            if (GetEvent(EVENT_PACKET_29) && _packet29 != nullptr)
            {
                if (_mtx.try_lock()) // skip if unable to get lock
                {
                    if (_nextPacket29 == nullptr)
                    {
                        _nextPacket29 = _packet29;
                        ClearEvent(EVENT_PACKET_29);
                    }
                    else
                    {
                        p->SetRow(_magNumber, 29, _nextPacket29->GetLine(), CODING_13_TRIPLETS);
                        _nextPacket29 = _nextPacket29->GetNextLine();
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
                    
                    if (_page->GetPageFunction() != MIP)
                    {
                        // presentation enhancement pages
                        _waitingForField = false; // don't need a page erasure interval
                    }
                    
                    _subpage = _page->GetSubpage();
                    if (_subpage == nullptr) // page is empty
                    {
                        _page->FreeLock(); // Must free the lock or we can never use this page again!
                        goto loopback;
                    }
                    
                    _status = _subpage->GetSubpageStatus() & 0x8000; // get transmit flag
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
                    goto loopback;
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
                    p->Header(_magNumber,0xFF,0x0000,0x8010,_hasCustomHeader?_customHeaderTemplate:_configure->GetHeaderTemplate());
                    return p;
                }
                
                _thisRow=0;
                
                if (_page->IsCarousel() && !_page->GetOneShotFlag()) // don't cycle oneshot pages
                {
                    if (_page->Expired(true))
                    {
                        // cycle if timer has expired
                        _page->StepNextSubpage();
                        _subpage = _page->GetSubpage();
                        if (_subpage == nullptr) // page is empty
                        {
                            _page->FreeLock(); // Must free the lock or we can never use this page again!
                            goto loopback;
                        }
                        _page->SetTransitionTime(_subpage->GetCycleTime());
                        _status=_subpage->GetSubpageStatus();
                    }
                    else
                    {
                        _subpage = _page->GetSubpage();
                        if (_subpage == nullptr) // page is empty
                        {
                            _page->FreeLock(); // Must free the lock or we can never use this page again!
                            goto loopback;
                        }
                        // clear any ERASE bit if page hasn't cycled to minimise flicker, and the interrupted status bit
                        _status=_subpage->GetSubpageStatus() & ~(PAGESTATUS_C4_ERASEPAGE | PAGESTATUS_C9_INTERRUPTED);
                    }
                    
                    thisSubcode=_subpage->GetSubCode();
                    _region=_subpage->GetRegion();
                }
                else
                {
                    _subpage = _page->GetSubpage();
                    if (_subpage == nullptr) // page is empty
                    {
                        _page->FreeLock(); // Must free the lock or we can never use this page again!
                        goto loopback;
                    }
                    
                    thisSubcode=_subpage->GetSubCode();
                    _status=_subpage->GetSubpageStatus();
                    _region=_subpage->GetRegion();
                }
                
                // Handle pages with update bit set in a useful way.
                // This isn't defined by the specification.
                if (_status & PAGESTATUS_C8_UPDATE)
                {
                    // Clear update bit in stored page so that update flag is only transmitted once
                    _subpage->SetSubpageStatus(_subpage->GetSubpageStatus() & ~PAGESTATUS_C8_UPDATE);
                    
                    // Also set the erase flag in output. This will allow left over rows in adaptive transmission to be cleared without leaving the erase flag set causing flickering.
                    _status|=PAGESTATUS_C4_ERASEPAGE;
                }
                
                if (updatedFlag)
                {
                    // page is updated set interrupted sequence flag
                    _status|=PAGESTATUS_C9_INTERRUPTED;
                }
            }
            
            if (!(_status & 0x8000))
            {
                _page->FreeLock(); // Must free the lock or we can never use this page again!
                goto loopback;
            }
            
            // clear a flag we use to prevent duplicated X/28/0 packets
            _hasX28Region = false;
            p->Header(_magNumber,_page->GetPageNumber(),thisSubcode,_status,_hasCustomHeader?_customHeaderTemplate:_configure->GetHeaderTemplate());
            
            uint16_t tempCRC = p->PacketCRC(0); // calculate the crc of the new header
            
            bool headerChanged = _subpage->HasHeaderChanged(tempCRC);
            bool pageChanged = _subpage->HasSubpageChanged();
            if (headerChanged || pageChanged)
            {
                // the content of the header has changed or the page has been reloaded
                // we must now CRC the whole page
                Packet TempPacket(8,25); // a temporary packet for checksum calculation
                for (int i=1; i<26; i++)
                {
                    TempPacket.SetRow(_magNumber, _thisRow, _subpage->GetRow(i)->GetLine(), _subpage->GetRow(i)->IsBlank()?CODING_7BIT_TEXT:_page->GetPageCoding());
                    tempCRC = TempPacket.PacketCRC(tempCRC);
                }
                
                _subpage->SetSubpageCRC(tempCRC);
                
                // TODO: the page content may get modified by substitutions in Packet::tx() which will result in an invalid checksum
            }
            
            assert(p!=NULL);
            
            _lastTxt=_page->GetTxRow(27); // Get _lastTxt ready for packet 27 processing
            _state=PACKETSTATE_PACKET27;
            break;
        }
        case PACKETSTATE_PACKET27:
        {
            if (_lastTxt)
            {
                if ((_lastTxt->GetCharAt(0) & 0xF) > 3) // designation codes > 3
                    p->SetRow(_magNumber, 27, _lastTxt->GetLine(), CODING_13_TRIPLETS); // enhancement linking
                else if ((_lastTxt->GetCharAt(1) & _lastTxt->GetCharAt(2) & _lastTxt->GetCharAt(7) & _lastTxt->GetCharAt(8) &
                         _lastTxt->GetCharAt(13) & _lastTxt->GetCharAt(14) & _lastTxt->GetCharAt(19) & _lastTxt->GetCharAt(20) &
                         _lastTxt->GetCharAt(25) & _lastTxt->GetCharAt(26) & _lastTxt->GetCharAt(31) & _lastTxt->GetCharAt(32)) != 0xf)
                         // don't generate packet if all page links are 0xFF
                {
                    p->SetRow(_magNumber, 27, _lastTxt->GetLine(), CODING_HAMMING_8_4); // navigation packets
                    if ((_lastTxt->GetCharAt(0) & 0xF) == 0) // only designation code 0 has CRC
                        p->SetX27CRC(_subpage->GetSubpageCRC());
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
                
                std::array<uint8_t, 40> val{
                    0x40, 0x40, 0x40, 0x74, 0x47, 0x70, 0x43, 0x75, 0x57, 0x40, 0x74, 0x77,
                    0x77, 0x43, 0x70, 0x52, 0x41, 0x60, 0x55, 0x42, 0x57, 0x77, 0x44, 0x73,
                    0x57, 0x77, 0x75, 0x77, 0x77, 0x77, 0x55, 0x77, 0x57, 0x77, 0x75, 0x57,
                    0x77, 0x45, 0x40, 0x40
                }; // default X/28/0 packet in pre Hamming24EncodeTriplet form (i.e. tti OL format)
                int NOS = (_status & 0x380) >> 7;
                int language = NOS | (_region << 3);
                int triplet = 0x3C000 | (language << 7); // construct triplet 1
                val[1] = (triplet & 0x3F) | 0x40;
                val[2] = ((triplet & 0xFC0) >> 6) | 0x40;
                val[3] = ((triplet & 0x3F000) >> 12) | 0x40;
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
                goto loopback;
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
                _pageList->RemovePage(_page); // remove from page list if no longer in any type lists - will free the lock
                // this is to handle the case where an UpdatedPages is the only copy of a page left and must be removed now it's been transmitted.
                goto loopback;
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
                    _page->FreeLock(); // Must free the lock or we can never use this page again!
                }
                else
                {
                    // otherwise go on to X/26
                    _lastTxt=_page->GetTxRow(26);
                    _state=PACKETSTATE_PACKET26;
                }
                goto loopback;
            }
            else
            {
            //_outp("J");
                if (_lastTxt->IsBlank() && (_configure->GetRowAdaptive() || _thisRow == 25 || _page->GetPageFunction() != LOP)) // If a row is empty then skip it if row adaptive mode on, or not a level 1 page
                {
                    goto loopback;
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
        default:
        {
            _state=PACKETSTATE_HEADER;// For now, do the next page
            _page->FreeLock(); // Must free the lock or we can never use this page again!
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
        if (_waitingForField)
        {
            _waitingForField = false;
        }
    }
    
    if (!_waitingForField)
    {
        _priorityCount--;
        if (_priorityCount==0 || force || (_updatedPages->waiting())) // force if there are updated pages waiting
        {
            _priorityCount=_priority;
            result=true;
        }
    }
    
    if (_pageList->GetSize(_magNumber)>0)
    {
        return result;
    }
    else
    {
        return false;
    }
};

void PacketMag::SetPacket29(std::shared_ptr<TTXLine> line)
{
    _packet29 = line;
    _nextPacket29 = _packet29;
    do
    {
        uint8_t dc = _nextPacket29->GetCharAt(0) & 0xf;
        if (dc == 0 || dc == 4)
        {
            _magRegion = ((_nextPacket29->GetCharAt(2) & 0x30) >> 4) | ((_nextPacket29->GetCharAt(3) & 0x3) << 2);
        }
        _nextPacket29 = _nextPacket29->GetNextLine();
    } while (_nextPacket29 != nullptr);
    _nextPacket29 = _packet29;
}

void PacketMag::DeletePacket29()
{
    _mtx.lock();
    _packet29 = nullptr;
    _nextPacket29 == nullptr;
    _mtx.unlock();
}

void PacketMag::SetCustomHeader(std::shared_ptr<TTXLine> line)
{
    _hasCustomHeader = true;
    std::string str = "";
    for (int i=8; i<40; i++)
        str += line->GetCharAt(i);
    _customHeaderTemplate.assign(str);
}

void PacketMag::DeleteCustomHeader()
{
    _hasCustomHeader = false;
    _debug->Log(Debug::LogLevels::logINFO,"[PacketMag::DeleteCustomHeader] Removing custom header from magazine " + std::to_string(_magNumber));
}
