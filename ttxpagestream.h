#ifndef _TTXPAGESTREAM_H_
#define _TTXPAGESTREAM_H_

#include <mutex>
#include <sys/stat.h>
#include <memory>

#include "page.h"
#include "packet.h"
#include "masterClock.h"

/** @brief Extends Page to allow Service to iterate through this page.
 *  It adds iterators to the page and also timing control if it is a carousel.
 *  It also has features to help add, remove and update pages in a service.
 */
namespace vbit
{
class TTXPageStream : public Page
{
    public:
        /** Default constructor. */
        TTXPageStream();
        /** Default destructor */
        virtual ~TTXPageStream();
        
        void MarkForDeletion() { _deleteFlag = true; }
        bool GetIsMarked() { return _deleteFlag; }
        
        void SetOneShotFlag(bool val) { _isOneShot = val; }
        bool GetOneShotFlag() { return _isOneShot; }

        bool GetCarouselFlag() { return _isCarousel; }
        void SetCarouselFlag(bool val) { _isCarousel = val; }

        bool GetSpecialFlag() { return _isSpecial; }
        void SetSpecialFlag(bool val) { _isSpecial = val; }
        
        bool GetNormalFlag() { return _isNormal; }
        void SetNormalFlag(bool val) { _isNormal = val; }
        
        bool GetUpdatedFlag() { return _isUpdated; }
        void SetUpdatedFlag(bool val) { _isUpdated = val; }
        
        int GetUpdateCount() {return _updateCount;}
        void IncrementUpdateCount();

        /** Set the time when this carousel expires
         *  which is the current time plus the cycle time
         *  or the number of page cycles remaining
         */
        void SetTransitionTime(int cycleTime);

        /** Used to time carousels
         *  If StepCycles is set, decrement page cycle count
         *  @return true if it is time to change carousel page
         */
        bool Expired(bool StepCycles=false);

        bool LoadPage(std::string filename);
        
        bool GetLock();
        void FreeLock();

        /** Used to enable list->remove
         */
        bool operator==(const TTXPageStream& rhs) const;

        // Todo: These are migrating to Page
        void SetSelected(bool value){_Selected=value;}; /// Set the selected state to value
        bool Selected(){return _Selected;}; /// Return the selected state
        
        void SetPacket29Flag(bool value){_loadedPacket29=value;}; // Used by PageList::CheckForPacket29OrCustomHeader
        bool GetPacket29Flag(){return _loadedPacket29;}; // Used by PageList::DeleteOldPages
        
        void SetCustomHeaderFlag(bool value){_loadedCustomHeader=value;}; // Used by PageList::CheckForPacket29OrCustomHeader
        bool GetCustomHeaderFlag(){return _loadedCustomHeader;};
        
    protected:
        
    private:
        time_t _transitionTime; // Records when the next carousel transition is due
        int _cyclesRemaining; // As above for cycle mode
        
        bool _loadedPacket29; // Packet 29 for magazine was loaded from this page. Should only be set on one page in each magazine.
        
        bool _loadedCustomHeader; // Custom header was loaded from this page. Should only be set on one page in each magazine.
        
        bool _Selected;   /// Marked as selected by the inserter P command

        bool _isCarousel;
        bool _isSpecial;
        bool _isNormal;
        bool _isUpdated;

        int _updateCount; // update counter for special pages.
        
        bool _deleteFlag; // marks a page for deletion from the service and cannot be undone
        
        bool _isOneShot;
        
        std::shared_ptr<std::mutex> _mtx;
};
};
#endif // _TTXPAGESTREAM_H_
