#ifndef TTXLINE_H
#define TTXLINE_H
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <memory>

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
        void Setm_textline(std::string const& val, bool validateLine=true);

        /** Access m_textline
         * \return The current value of m_textline
         */
        std::string GetLine();

        /**
         * @brief Check if the line is blank so that we don't bother to write it to the file.
         * @return true is the line is blank
         */
        bool IsBlank();

        /** Get one character from this line.
         *  If there is no data set then return a space
         */
        char GetCharAt(int xLoc);

        /** Adds line to a linked list
         *  This is used for enhanced packets which might require multiples of the same row
         */
        void AppendLine(std::string  const& line);

        std::shared_ptr<TTXLine> GetNextLine(){return _nextLine;}

    protected:
    private:
        std::string validate(std::string const& test);

        std::string m_textline;
        std::shared_ptr<TTXLine> _nextLine;
};

#endif // TTXLINE_H
