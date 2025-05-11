#ifndef TTXPAGE_H
#define TTXPAGE_H
#include <stdlib.h>
#include "string.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <fstream>
#include <string>

#include <cstdint>
#include <cstdlib>
#include <iomanip>

#include <assert.h>

#include "ttxline.h"

#define FIRSTPAGE 0x1ff00

// MiniTED Page Status word
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

// @todo more page codings
enum PageCoding {CODING_7BIT_TEXT,CODING_8BIT_DATA,CODING_13_TRIPLETS,CODING_HAMMING_8_4,CODING_HAMMING_7BIT_GROUPS,CODING_PER_PACKET};
enum PageFunction {LOP, DATABROADCAST, GPOP, POP, GDRCS, DRCS, MOT, MIP, BTT, AIT, MPT, MPT_EX};

class TTXPage : public std::enable_shared_from_this<TTXPage>
{
    public:
        /** Default constructor */
        TTXPage();

        /** Default destructor */
        virtual ~TTXPage();
        
        std::shared_ptr<TTXPage> getptr()
        {
            return shared_from_this();
        }

        /** Access m_SubPage which is the next page in a carousel
         * \return The current value of m_SubPage
         */
        std::shared_ptr<TTXPage> Getm_SubPage() { return m_SubPage; }

        /** Set m_SubPage
         * \param val New value to set
         */
        void Setm_SubPage(std::shared_ptr<TTXPage> val) { m_SubPage = val; }

         /** Setter/Getter for m_pageNumber
          */
         int GetPageNumber() const {return m_PageNumber;}
         void SetPageNumber(int page);

         /** Setter/Getter for m_pageStatus
          */
         int GetPageStatus() {return m_pagestatus;}
         void SetPageStatus(int ps){m_pagestatus=ps;}

         /** Setter/Getter for cycle counter/timer seconds
          */
         int GetCycleTime() {return m_cycletimeseconds;}
         void SetCycleTime(int time){m_cycletimeseconds=time;}

         /** Setter/Getter for cycle counter/timer seconds
          */
         char GetCycleTimeMode() {return m_cycletimetype;}
         void SetCycleTimeMode(char mode){m_cycletimetype=mode;}

        /** Get a row of text
         * \return The TTXLine object of the required row. Check result for NULL if there isn't an actual row.
         */
         std::shared_ptr<TTXLine> GetRow(unsigned int rowNumber);

        /** Set row rownumber with text line.
         * \return nowt
         */
         void SetRow(unsigned int rownumber, std::string line);

        /** Set the subcode. Subcode is effectively the subpage nummber in a carousel
         * \param subcode : A subcode value from 0000 to 3F7F (maybe we should check this!)
         */
         void SetSubCode(unsigned int subcode) {m_subcode=subcode;}

        /** Get the subcode
         * \return Subcode value
         */
         unsigned int GetSubCode() {return m_subcode;}

        /** Set the region.
         * A region is just one of the 16 sets of character sets.
         * \param region : A hex value 0..f
         * \return Nothing.
         */
        void SetRegion(int region){m_region=region;}

        /** Get the region.
         * \return region 0..f.
         */
        int GetRegion(){return m_region;}

        /** Set a Fastext link
         * \param link 0..5 where 0..3 are the main links, 5 is index and 4, nobody knows why
         * \param value - The link page number. Note that out of range numbers less than 0x100 are permitted. Droidfax can use these to switch services.
         */
        void SetFastextLink(int link, int value);
        
        /** Get the array of 6 fastext links */
        int* GetLinkSet(){return m_fastextlinks;};
        
        unsigned int GetLastPacket() {return m_lastpacket;};
        
        // get the function or coding of a page as the enum
        PageCoding GetPageCoding() {return m_pagecoding;}
        PageFunction GetPageFunction() {return m_pagefunction;}
        
        // set the page function or coding based on their integer representations in ETS 300 706 section 9.4.2.1
        static PageCoding ReturnPageCoding(int pageCoding);
        void SetPageFunctionInt(int pageFunction);
        void SetPageCodingInt(int pageCoding){m_pagecoding = ReturnPageCoding(pageCoding);};

        bool Special() {return (m_pagefunction == GPOP || m_pagefunction == POP || m_pagefunction == GDRCS || m_pagefunction == DRCS || m_pagefunction == MOT || m_pagefunction == MIP);} // more convenient way to tell if a page is 'special'.
        
        bool HasPageChanged(){bool t = _pageChanged; _pageChanged = false; return t; }; // clears the flag for this subpage
        
        bool HasHeaderChanged(uint16_t crc); // updates the header crc for this subpage
        
        void SetPageCRC(uint16_t crc){_pageCRC = crc;}; // update the stored crc
        uint16_t GetPageCRC(){return _pageCRC;}; // retrieve the stored crc
        
        void ClearPage();
        void RenumberSubpages();
        
    protected:
        
    private:
        // Private variables
        // Private objects
        int m_PageNumber;           // PN
        std::shared_ptr<TTXPage> m_SubPage;
        std::shared_ptr<TTXLine> m_pLine[MAXROW+1];
        char m_cycletimetype;       // CT
        int m_cycletimeseconds;     // CT
        int m_fastextlinks[6];      // FL
        unsigned int m_subcode;     // SC
        int m_pagestatus;           // PS
        int m_region;               // RE
        unsigned int m_lastpacket;
        PageCoding m_pagecoding;
        PageFunction m_pagefunction;
        bool _pageChanged; // page was reloaded
        
        uint16_t _headerCRC; // holds the last calculated CRC of the page header
        uint16_t _pageCRC; // holds the calculated CRC of the page
        
        // Private functions

};

#endif // TTXPAGE_H
