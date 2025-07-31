#ifndef TTXLINE_H
#define TTXLINE_H
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <memory>
#include <array>

/** TTXLine - a single line of teletext
 *  The line is always stored in 40 bytes in transmission ready format
 * (but with the parity bit set to 0).
 */

class TTXLine : public std::enable_shared_from_this<TTXLine>
{
    public:
        /** Constructors */
        TTXLine();
        TTXLine(std::string const& line, bool validate=true);
        
        TTXLine(std::shared_ptr<TTXLine> line);
        
        /** Default destructor */
        virtual ~TTXLine();

        std::shared_ptr<TTXLine> getptr()
        {
            return shared_from_this();
        }

        /** Set the teletext line contents
         * \param val - New value to set
         * \param validateLine - If true, it ensures the line is checked and modified if needed to be transmission ready.
         */
        void SetLine(std::string const& val, bool validateLine=true);

        std::array<uint8_t, 40> GetLine(){return _line;};
        bool IsBlank();
        uint8_t GetCharAt(int index){return _line[index];};

        /** Adds line to a linked list
         *  This is used for enhanced packets which might require multiples of the same row
         */
        void AppendLine(std::string  const& line);

        std::shared_ptr<TTXLine> GetNextLine(){return _nextLine;}

    protected:
    private:
        std::array<uint8_t, 40> _line; // 40 byte line
        std::shared_ptr<TTXLine> _nextLine;
};

#endif // TTXLINE_H
