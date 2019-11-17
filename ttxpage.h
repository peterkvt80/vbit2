#ifndef TTXPAGE_H
#define TTXPAGE_H
#include <stdlib.h>
#include "string.h"
#include <iostream>
#include <sstream>

#include <fstream>
#include <string>

#include <cstdlib>
#include <iomanip>

#include <assert.h>

#include "ttxcodes.h"
#include "ttxline.h"
#include "hamm-tables.h"

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
enum PageCoding {CODING_7BIT_TEXT,CODING_8BIT_DATA,CODING_13_TRIPLETS,CODING_HAMMING_8_4};
enum PageFunction {LOP, DATABROADCAST, GPOP, POP, GDRCS, DRCS, MOT, MIP, BTT, AIT, MPT, MPT_EX};

class TTXPage
{
    public:
        /** Default constructor */
        TTXPage();

        /** Construct from a file */
        TTXPage(std::string filename);

        /** Default destructor */
        virtual ~TTXPage();


        /** Copy constructor
         *  \param other Object to copy from
         */
        TTXPage(const TTXPage& other);

        /** Assignment operator
         *  \param other Object to assign from
         *  \return A reference to this
         */
        // TTXPage& operator=(const TTXPage& other);

        /** Access m_SubPage which is the next page in a carousel
         * \return The current value of m_SubPage
         */
        TTXPage* Getm_SubPage() { return m_SubPage; }

        /** Set m_SubPage
         * \param val New value to set
         */
        void Setm_SubPage(TTXPage* val) { m_SubPage = val; }

        /** Get the page
         *  Warning! This is not the teletext page number. It is just an index to the pages in this list.
         * \return Pointer to the page object that we want
         */
         TTXPage* GetPage(unsigned int pageNumber);

         /** Setter/Getter for m_pageNumber
          */
         int GetPageNumber() const {return m_PageNumber;}
         void SetPageNumber(int page);

         /** Setter/Getter for m_pageStatus
          */
         int GetPageStatus() {return m_pagestatus;}
         void SetPageStatus(int ps){m_pagestatus=ps;}

         /** Setter/Getter for m_description
          */
         std::string GetDescription() const {return m_description;}
         void SetDescription(std::string desc){m_description=desc;}

         /** Setter/Getter for cycle counter/timer seconds
          */
         int GetCycleTime() {return m_cycletimeseconds;}
         void SetCycleTime(int time){m_cycletimeseconds=time;}

         /** Setter/Getter for cycle counter/timer seconds
          */
         char GetCycleTimeMode() {return m_cycletimetype;}
         void SetCycleTimeMode(char mode){m_cycletimetype=mode;}

         /** Setter/Getter for m_sourcepage
          *  This is the filename that was used to load the page
          */
         std::string GetSourcePage() const {return m_sourcepage;}
         void SetSourcePage(std::string fname){m_sourcepage=fname;}

         /** Get the page count
         *  It also replaces the subcode sequence. (Is this a good idea?)
         * Must only call this in the root page.
         * \return Number of subpages.
         */
        int GetPageCount();

        /** Get a row of text
         * \return The TTXLine object of the required row. Check result for NULL if there isn't an actual row.
         */
         TTXLine* GetRow(unsigned int rowNumber);

        /** Set row rownumber with text line.
         * \return nowt
         */
         void SetRow(unsigned int rownumber, std::string line);

        /** Save the whole page set
         * \param filename : The destination file
         * \return false if the save failed
         */
        bool SavePage(std::string filename);

        /** Save the page set back to the original file
         * \return false if the save failed
         */
        bool SavePageDefault();


        /** Set the subcode. Subcode is effectively the subpage nummber in a carousel
         * \param subcode : A subcode value from 0000 to 3F7F (maybe we should check this!)
         */
         void SetSubCode(unsigned int subcode) {m_subcode=subcode;}

        /** Get the subcode
         * \return Subcode value
         */
         unsigned int GetSubCode() {return m_subcode;}

        /** Copy the metadata from page to here.
         *  Metadata is everything except the actual text lines and the SubPage link.
         * \param page : A TTXPage object to copy from
         * \return
         */
        void CopyMetaData(TTXPage* page);

        /** Set the language.
         * 0=Engliah, 1=German, 2=Swedish, 3=Italian, 4=French, 5=Spanish, 6=Czech
         * \param language A language number 0..6 for western europe.
         * \return Nothing.
         */
        void SetLanguage(int language);

        /** Get the language.
         * \return language 0..6.
         */
        int GetLanguage();

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

        /** Get a Fastext link
         * \param link 0..5 where 0..3 are the main links, 5 is index and 4, nobody knows why
         * \return A link number (in hex base)
         */
        int GetFastextLink(int link);

        /** Set a Fastext link
         * \param link 0..5 where 0..3 are the main links, 5 is index and 4, nobody knows why
         * \param value - The link page number. Note that out of range numbers less than 0x100 are permitted. Droidfax can use these to switch services.
         */
        void SetFastextLink(int link, int value);
        
        /** Get the array of 6 fastext links */
        int* GetLinkSet(){return m_fastextlinks;};

        /** @brief Should check this before closing a page
         */
        inline bool PageChanged(){return pageChanged;};
        static bool pageChanged;         // / True if we have done some edits

        void DebugDump() const;

        inline bool Loaded() const {return m_Loaded;};
        
        unsigned int GetLastPacket() {return m_lastpacket;};
        
        // get the function or coding of a page as the enum
        PageCoding GetPageCoding() {return m_pagecoding;}
        PageFunction GetPageFunction() {return m_pagefunction;}
        
        // set the page function or coding based on their integer representations in ETS 300 706 section 9.4.2.1
        void SetPageFunctionInt(int pageFunction);
        void SetPageCodingInt(int pageCoding);

        bool Special() {return (m_pagefunction == GPOP || m_pagefunction == POP || m_pagefunction == GDRCS || m_pagefunction == DRCS || m_pagefunction == MOT || m_pagefunction == MIP);} // more convenient way to tell if a page is 'special'.

        /** @todo migrate this deep copy into the standard copy constructor
         * Warning. Only deep copies the top page. Not for carousels (yet)
         */
        void Copy(TTXPage* src);

        /** \brief Check if any line in the page has changed.
         *  This can only be called once as it will clear the changed state.
         *  This is NOT the same as pageChanged.
         */
        bool Changed();
        
        void SetFileChangedFlag(){_fileChanged=true;};

        void SetSelected(bool value){_Selected=value;}; /// Set the selected state to value
        bool Selected(){return _Selected;}; /// Return the selected state




    protected:
        /** Load a TTI page (MRG Systems)
         * \param filename : The source file
         * \return true if the page was loaded
         */
        bool m_LoadTTI(std::string filename);

        int m_cycletimeseconds;     // CT
        int m_fastextlinks[6];      // FL
        int m_PageNumber;  					// PN
    private:
        // Private variables
        // Private objects
        TTXPage* m_SubPage; //!< Member variable "m_SubPage"
        /// @ todo: What about multiple enhancement packets. More than one is allowed.
        TTXLine* m_pLine[MAXROW+1];   // OL. 26 and 27 can be set manually. Keep whatever people put in there.

        std::string m_destination;  // DS
        std::string m_sourcepage;   // SP
        std::string m_description;  // DE
        char m_cycletimetype;       // CT
        unsigned int m_subcode;     // SC
        int m_pagestatus;           // PS
        int m_region;               // RE
        unsigned int m_lastpacket;
        PageCoding m_pagecoding;
        PageFunction m_pagefunction;
        // Private functions
        void m_Init();
        void m_OutputLines(std::ofstream& ttxfile, TTXPage* p); /// Send ttx lines to an output stream
        int instance;

        std::string m_FormatPageNumber(TTXPage* p); /// \return the page number ready to write to file
        int findPageNumber(char* buf);
        bool m_Loaded;

        /** Load a VTXv$ page
         * \param filename : The source file
         * \return true if the page was loaded
         */
        bool m_LoadVTXv4(std::string filename);

        /** Load an EP1 page
         * \param filename : The source file
         * \return true if the page was loaded
         */
        bool m_LoadEP1(std::string filename);

        /** Load a VTX page
         * \param filename : The source file
         * \return true if the page was loaded
         */
        bool m_LoadVTX(std::string filename);

        /** Load a TTX page (Cebra)
         * \param filename : The source file
         * \return true if the page was loaded
         */
        bool m_LoadTTX(std::string filename);

        bool _Selected; /// True if this page has been selected.
        
        bool _fileChanged; // page was reloaded by the filemonitor

};

#endif // TTXPAGE_H
