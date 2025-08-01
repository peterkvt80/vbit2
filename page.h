#ifndef PAGE_H
#define PAGE_H
#include <stdlib.h>
#include "string.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <fstream>
#include <string>
#include <list>
#include <array>

#include <cstdint>
#include <cstdlib>
#include <iomanip>

#include <assert.h>

#include "ttxline.h"

// TTI format Page Status word
#define PAGESTATUS_C4_ERASEPAGE     0x4000
#define PAGESTATUS_C5_NEWSFLASH     0x0001
#define PAGESTATUS_C6_SUBTITLE      0x0002
#define PAGESTATUS_C7_SUPPRESSHDR   0x0004
#define PAGESTATUS_C8_UPDATE        0x0008
#define PAGESTATUS_C9_INTERRUPTED   0x0010
#define PAGESTATUS_C10_INHIBIT      0x0020
#define PAGESTATUS_TRANSMITPAGE     0x8000
#define PAGESTATUS_SUBSTITUTEPAGE   0x0800
#define PAGESTATUS_C11_SERIALMAG    0x0040

// Allow for enhancement packets
#define MAXROW 29

enum PageCoding {CODING_7BIT_TEXT,CODING_8BIT_DATA,CODING_13_TRIPLETS,CODING_HAMMING_8_4,CODING_HAMMING_7BIT_GROUPS,CODING_PER_PACKET};
enum PageFunction {LOP, DATABROADCAST, GPOP, POP, GDRCS, DRCS, MOT, MIP, BTT, AIT, MPT, MPT_EX};

namespace vbit
{
class FastextLink
{
    public:
        uint16_t page;
        uint16_t subpage;
};

class Subpage
{
    public:
        Subpage();
        virtual ~Subpage();
        
        uint16_t GetSubCode() {return _subcode;}
        void SetSubCode(uint16_t subcode) {_subcode=subcode;}
        
        uint16_t GetSubpageStatus() {return _status;}
        void SetSubpageStatus(uint16_t ps){_status=ps;}
        
        int GetCycleTime() {return _cycleTime;}
        void SetCycleTime(int time){_cycleTime=time;}
        
        bool GetTimedMode() {return _timedMode;}
        void SetTimedMode(bool mode){_timedMode=mode;}
        
        uint8_t GetRegion(){return _region;}
        void SetRegion(uint8_t region){_region=region;}
        
        std::shared_ptr<TTXLine> GetRow(unsigned int rowNumber);
        void SetRow(unsigned int rownumber, std::shared_ptr<TTXLine> line);
        
        void SetFastext(std::array<FastextLink, 6> links, uint8_t mag);
        
        unsigned int GetLastPacket() {return _lastPacket;};
        
        bool HasSubpageChanged(){bool t = _subpageChanged; _subpageChanged = false; return t; }; // clears the flag for this subpage
        
        bool HasHeaderChanged(uint16_t crc); // updates the header crc for this subpage
        
        void SetSubpageCRC(uint16_t crc){_subpageCRC = crc;}; // update the stored crc
        uint16_t GetSubpageCRC(){return _subpageCRC;}; // retrieve the stored crc
        
    private:
        uint16_t _subcode;
        uint16_t _status;
        int _cycleTime;         // number of page cycles or seconds before subpage cycles
        bool _timedMode;        // cycle subpage based on time in seconds
        uint8_t _region;
        
        std::shared_ptr<TTXLine> _lines[MAXROW+1];
        unsigned int _lastPacket;
        
        bool _subpageChanged;   // page was reloaded
        uint16_t _headerCRC;    // holds the last calculated CRC of the page header
        uint16_t _subpageCRC;   // holds the last calculated CRC of the page
};

class Page
{
    public:
        /** Default constructor */
        Page();

        /** Default destructor */
        virtual ~Page();
        
        void AppendSubpage(std::shared_ptr<Subpage> s);
        void InsertSubpage(std::shared_ptr<Subpage> s);
        void RemoveSubpage(std::shared_ptr<Subpage> s);
        unsigned int GetSubpageCount() {return _subpages.size();};
        
        int GetPageNumber() const {return _pageNumber;};
        void SetPageNumber(int page);
        
        // get the function or coding of a page as the enum
        PageCoding GetPageCoding() {return _pageCoding;};
        PageFunction GetPageFunction() {return _pageFunction;};
        
        // set the page function or coding based on their integer representations in ETS 300 706 section 9.4.2.1
        static PageCoding ReturnPageCoding(int pageCoding);
        void SetPageFunctionInt(int pageFunction);
        void SetPageCodingInt(int pageCoding){_pageCoding = ReturnPageCoding(pageCoding);};

        bool Special() {return (_pageFunction == GPOP || _pageFunction == POP || _pageFunction == GDRCS || _pageFunction == DRCS || _pageFunction == MOT || _pageFunction == MIP);} // more convenient way to tell if a page is 'special'.
        
        void ClearPage();
        void RenumberSubpages();
        
        bool IsCarousel();

        void StepNextSubpage();
        void StepNextSubpageNoLoop();
        
        std::shared_ptr<Subpage> GetSubpage(){return _carouselPage;};
        void SetSubpage(uint16_t SubpageNumber);
        std::shared_ptr<Subpage> LocateSubpage(uint16_t SubpageNumber);
        
        std::shared_ptr<TTXLine> GetTxRow(uint8_t row);
        
    protected:
        
    private:
        int _pageNumber;
        PageCoding _pageCoding;
        PageFunction _pageFunction;
        bool _pageChanged; // page was reloaded
        
        std::list<std::shared_ptr<Subpage>> _subpages; // list of subpages
        std::list<std::shared_ptr<Subpage>>::iterator _iter;
        std::shared_ptr<Subpage> _carouselPage;
};
};
#endif // PAGE_H
