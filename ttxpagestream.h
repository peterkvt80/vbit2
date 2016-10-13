#ifndef _TTXPAGESTREAM_H_
#define _TTXPAGESTREAM_H_

#include "ttxpage.h"

/** @brief Extends TTXPage to allow Service to iterate through this page.
 *  It adds iterators to the page and also timing control if it is a carousel.
 */
class TTXPageStream : public TTXPage
{
    public:
        /** Default constructor. Don't call this */
        TTXPageStream();
        /** Default destructor */
        virtual ~TTXPageStream();

        /** The normal constructor
         */
        TTXPageStream(std::string filename);

        /** Access _lineCounter
         * \return The current value of _lineCounter
         */
        unsigned int GetLineCounter() { return _lineCounter; }
        /** Set _lineCounter
         * \param val New value to set
         */
        void SetLineCounter(unsigned int val) { _lineCounter = val; }

        /** Access _isCarousel
         * \return The current value of _isCarousel
         */
        bool GetCarouselFlag() { return _isCarousel; }

        /** Set _isCarousel
         * \param val New value to set
         */
        void SetCarouselFlag(bool val) { _isCarousel = val; }

        ///** Access _CurrentPage
         //* \return The current value of _CurrentPage
         //*/
        //TTXPageStream* GetCurrentPage(unsigned int line) { _CurrentPage->SetLineCounter(line);return _CurrentPage; }
				
        ///** Set _CurrentPage
         //* \param val New value to set
         //*/
        //void SetCurrentPage(TTXPageStream* val) { _CurrentPage = val; }

        /** Access the currently iterated row
         * \return The current row from the current page
         */
        TTXLine* GetCurrentRow();
        /** Access the next iterated row
         * \return The next row from the current page
         */
        TTXLine* GetNextRow();

        /** Is the page a carousel?
         *  Don't confuse this with the _isCarousel flag which is used to mark when a page changes between single/carousel
         * \return True if there are subpages
         */
        inline bool IsCarousel(){return Getm_SubPage()!=NULL;};

        /** Set the time when this carousel expires
         *  ...which is the current time plus the cycle time
         */
        inline void SetTransitionTime(){_transitionTime=time(0)+m_cycletimeseconds;};

        /** Used to time carousels
         *  @return true if it is time to change carousel page
         */
        inline bool Expired(){return _transitionTime<=time(0);};
				
        /** Step to the next page in a carousel
				 *  Updates _CarouselPage;
         */
				void StepNextSubpage();
				
				/** This is used by mag */
				TTXPage* GetCarouselPage(){return _CarouselPage;};

    protected:

    private:
        unsigned int _lineCounter; //!< Member variable "_LineCounter" indexes the line in the page being transmitted
        bool _isCarousel; //!< Member variable "_isCarousel" If
        // TTXPageStream* _CurrentPage; //!< Member variable "_currentPage" points to the subpage being transmitted

        time_t _transitionTime; // Records when the next carousel transition is due
				
				TTXPage* _CarouselPage; /// Pointer to the current subpage of a carousel
};

#endif // _TTXPAGESTREAM_H_
