#ifndef TTXLINE_H
#define TTXLINE_H
#include <iostream>
#include <iomanip>
#include <string>

/** TTXLine - a single line of teletext
 *  The line is always stored in 40 bytes in transmission ready format
 * (but with the parity bit set to 0).
 */

class TTXLine
{
    public:
        /** Constructors */
        TTXLine();
        TTXLine(std::string const& line, bool validate=true);
        // TTXLine(std::string const& line);
        /** Default destructor */
        virtual ~TTXLine();

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

        /** Place a character in a line. Must be an actual teletext code.
         *  Bit 7 will be stripped off.
         * @param x - Address of the character
         & @param code - The character to set
         * \return previous character at that location (for undo)
         */
        char SetCharAt(int x,int code);

        /** Get one character from this line.
         *  If there is no data set then return a space
         */
        char GetCharAt(int xLoc);

				/** Adds line to a linked list
				 *  This is used for enhanced packets which might require multiples of the same row
				 */
				void AppendLine(std::string  const& line);

				TTXLine* GetNextLine(){return _nextLine;}

				void Dump();

    protected:
    private:
        std::string validate(std::string const& test);

        std::string m_textline;
				TTXLine* _nextLine;
				// If SetLine or SetChar can set the changed flag.

};

#endif // TTXLINE_H
