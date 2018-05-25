#include <stdio.h>      /* for printf() and fprintf() */
#include <string.h>


#include "TCPClient.h"

using namespace vbit;
using namespace ttx;

static int
vbi_unham8			(unsigned int		c)
{
	return _vbi_hamm8_inv[(uint8_t) c];
}

TCPClient::TCPClient(PacketSubtitle* subtitle, PageList* pageList) :
	_pCmd(_cmd),
	_newfor(subtitle),
	_pageList(pageList)
{
	strcpy(_pageNumber,"10000");
}

TCPClient::~TCPClient()
{
}

void TCPClient::DieWithError(std::string errorMessage)
{
    perror(errorMessage.c_str());
    exit(1);
}

void TCPClient::command(char* cmd, char* response)
{
	char result[132];
	char* ptr;
	int row;
	int status=0;
	//char statusText[4];
#ifdef DEBUG
	strcpy(response,"todo");
#endif
	switch (cmd[0])
	{
  case 'D' : // D[<start>]<sign>[<steps>] - directory
    // <start> = F | L
    // <sign> = + | -
    // <steps> = number of pages to step through (default 1)
    {
      TTXPageStream* page=nullptr;
      int ix=1;
      switch (cmd[ix])
      {
      case 'F' : // Move to first page
        std::cerr << "DF" << std::endl;
        ix++;
        if ((page=_pageList->FirstPage())==nullptr)
        {
          status=1; // Fail: There are no pages selected
          break;
        }
        status=0;
        break;
      case '+' : // Step to next entry
        page=_pageList->NextPage();
        if (page!=nullptr)
        {
          status=0;
        }
        else
        {
          status=1;
        }
        break;
      case '-' : // Step to previous entry
        page=_pageList->PrevPage();
        if (page!=nullptr)
        {
          status=0;
        }
        else
        {
          status=1;
        }
        break;
      case 'L' : // Move to last page
        std::cerr << "DL" << std::endl;
        ix++;
        if ((page=_pageList->LastPage())==nullptr)
        {
          status=1; // Fail: There are no pages selected
          break;
        }
        status=0;
        break;
      case 'P' : // Secret debug code - Sends page into to stderr
        std::cerr << "DP" << std::endl;
        {
          page=_pageList->FirstPage();
          for (int row=0;row<24;row++)
          {
            std::cerr << "OL," << std::setw(2) << row;
            TTXLine* line=page->GetRow(row);
            std::cerr << "," << line->GetLine() << std::endl;
          }
        }
        status=0;
        break;
      default:
        status=1;
        strcpy(result,"This was unexpected");
      }
      // Now we either have a page pointer or null
      if (status==0)
      {
        // ss mpp qq cc tttt ssss n xxxxxxxx
        sprintf(result,"%02d %03x %02d %02d %04x %1d %s",
                page->GetSubCode(),
                page->GetPageNumber(),
                page->GetCycleTime(),
                page->GetSubCode(),
                page->GetPageStatus(),
                0,
                "????????");
      }
      else if (status==1)
      {
        result[0]=0;
      }
      break;

    } // D command
	case 'L' : // L<nn><text> - Set row in current page with text
		{
			char rowStr[3];

			rowStr[0]=cmd[1];
			rowStr[1]=cmd[2];
			rowStr[2]=0;
			row=std::strtol(rowStr, &ptr, 10);
			ptr=&(cmd[3]);
			result[0]=0;
			status=0; // @todo Line number out of range =1
			//sprintf(result, "L Command Page=%.*s row=%d ptr=%s\n\r", 5, _pageNumber, row, ptr);

      std::cerr << "[TCPClient::command] L command starts" << std::endl;
			for (TTXPageStream* p=_pageList->FirstPage();
        p!=nullptr;
        p=_pageList->NextSelectedPage())
      {
        TTXPageStream* page=p;
        std::cerr << "[TCPClient::command] L command applied to page=" << std::hex << page->GetPageNumber() << std::endl;
				page->SetRow(row, ptr);
      }

		}
		break;
	case 'M' : // MD - delete pages
		status=1; // Incorrect syntax or no page deleted.
		result[0]=0;
		if (cmd[1]=='D')
		{
			for (TTXPageStream* p=_pageList->NextSelectedPage();
				p!=nullptr;
				p=_pageList->NextSelectedPage())
			{
				TTXPageStream* page=p;
				std::cerr << "[TCPClient::command] MD command applied to page=" << std::hex << page->GetPageNumber() << std::endl;
				status=0;
				// @todo Actually delete the page from _pageList
			}
		}
		break;
	case 'P' : // P<mppss> - Set the page number. @todo Extend to multiple page selection
		{
		  char* param=&(cmd[1]);    // The page identity parameter
		  char* pageSet=_pageNumber;// The validated page identity
		  int matchedPages=-1;
		  if (Validate(pageSet, param))
      {
        matchedPages=_pageList->Match(pageSet);
      }
      // pageSet is empty
      if (matchedPages==0)
      {
        matchedPages=1;
        status=8;
        for (int i=0;i<5;i++) // Wildcards are not allowed here
        {
          if (_pageNumber[i]=='*')
          {
            matchedPages=-1; // fail
          }
        }
        // @todo Create the page if status is 8
        if (status==8)
        {
          std::cerr << "[TCPClient::command] @todo Create the page" << std::endl;
        }
      }
      // Failed command
      if (matchedPages<0)
      {
        matchedPages=0;
        status=1;
      }
      _pageNumber[5]=0; // Terminate the page number before we print it just in case
      std::cerr << "[TCPClient::command] _pageNumber=" << _pageNumber << std::endl;
      sprintf(result,"%03d",matchedPages);
		}
		break;
	case 'R' : // R<nn> - Read back row <nn> from the first selected page
		{
			TTXPageStream* p;
			char rowStr[3];
			rowStr[0]=cmd[1];
			rowStr[1]=cmd[2];
			rowStr[2]=0;
			row=std::strtol(rowStr, &ptr, 10);
			p=_pageList->FirstPage();
			if (p==nullptr)
			{
				result[0]=0;
				status=1;
			}
			else
			{
				TTXLine* line=p->GetRow(row);
				strcpy(result,line->GetLine().c_str());
			}
		}
		break;
	case 'T' :;
		strcpy(result,"T not implemented\n\r");
		break;
	case 'Y' :
		strcpy(result,"VBIT620 Text Generator V2.01    "); // 32 character limit
		break;
	case 0x0e:
		strcpy(result,"This of course does not work. No CR in Softel \n\r");
		break;
	default:
		sprintf(result,"Command not recognised cmd=%s\n\r",cmd);
	}
	sprintf(response,"%s%1d\n\r",result,status);
} // command

void TCPClient::clearCmd(void)
{
	*_cmd=0;
	_pCmd=_cmd;
}

/** AddChar
 * Add a char to the command buffer and call command when we get CR
 * There is only a response if a command needs to send something back.
 * If the first character is a Newfor character then the data is parsed as Nu4.
 * Commands that are not Newfor are MODENORMAL. They are stateless (so far)
 * The state machine for OnAir and OffAir is not needed as they happen immediately.
 * MODESOFTELPAGEINIT sets a char count and when it gets the four chars it resets.
 * When loading subtitle data MODEGETROWCOUNT gets the row count then repeats for each row
 * MODESUBTITLEDATAHIGHNYBBLE, MODESUBTITLEDATALOWNYBBLE and MODEGETROW
 */
void TCPClient::addChar(char ch, char* response)
{
	static int mode=MODENORMAL;
	static int charCount=0; // Used to accumulate Newfor

	response[0]=0;
	// std::cerr << "[TCPClient::addChar] ch=" << ((int)ch) << " mode=" << mode << std::endl;
	switch (mode)
	{
	case MODENORMAL :
		_pkt=_cmd;
		if (_pCmd==_cmd) // On the first character we check if it is a Softel
		{
			switch (ch)
			{
			case 0x0e :
				mode=MODESOFTELPAGEINIT; // page 0nnn
				charCount=4;
				break;
			case 0x0f :
				mode=MODEGETROWCOUNT; // n and n*(rowhigh, rowlow, 40 bytes)
				break;
			case 0x10 :
				// Put the subtitle on air immediately
				_newfor.SubtitleOnair(response);
				// std::cerr << "[TCPClient::addChar] On air" << std::endl;
				clearCmd();
				mode=MODENORMAL;
				return;
			case 0x18 :
				// Remove the subtitle immediately
				// std::cerr << "[TCPClient::addChar] Subtitle Off" << std::endl;
				_newfor.SubtitleOffair();
				strcpy(response, "[addChar]Clear");
				clearCmd();
				mode=MODENORMAL;
				return;
			}
		} // If first character
		if (ch!='\n' && ch!='\r')
		{
			*_pCmd++=ch;
			*_pCmd=0;
			if (response) response[0]=0;
			// std::cerr << "[TCPClient::addChar] accumulate cmd=" << _cmd << std::endl;
		}
		else
		{
			// Got a complete non-newfor command
			std::cerr << "[TCPClient::addChar] finished cmd='" << strlen(_cmd) << std::endl;
			if (strlen(_cmd))  // Avoid this being called twice by \n\r combinations
      {
        command(_cmd, response);
      }
			clearCmd();
			mode=MODENORMAL;
		}
		break;
	// Message type 1 - Set subtitle page.
	case MODESOFTELPAGEINIT:	// We get four more characters and then the page is set
		// @todo If a nybble fails deham or isn't in range we should return nack
  	// std::cerr << "[TCPClient::addChar] Page init char=" << ((int)ch) << std::endl;
		*_pCmd++=ch;
		charCount--;
		// The last time around we have the completed command
		if (!charCount)
		{
			int page=_newfor.SoftelPageInit(_cmd);
			sprintf(response,"[addChar]MODESOFTELPAGEINIT Set page=%03x",page);
			// std::cerr << "[TCPClient::addChar] Softel page init response=" << response << std::endl;
			// Now that we are done, set up for the next command
			clearCmd();
			mode=MODENORMAL;
		}
		break;
	case MODEGETROWCOUNT: // Subtitle rows follow this
		*_pCmd++=ch;
		{
		  char* p=_cmd;
		  _row=_newfor.GetRowCount(p);
		}
	  sprintf(response,"[TCPClient::addChar] MODEGETROWCOUNT =%d\n",_row);
  	// std::cerr << response << std::endl;
		mode=MODESUBTITLEDATAHIGHNYBBLE;
		break;
	case MODESUBTITLEDATAHIGHNYBBLE:
		*_pCmd++=ch;
		charCount=40;
		mode=MODESUBTITLEDATALOWNYBBLE;
		_rowAddress=vbi_unham8(ch)*16; // @todo Check validity
		break;
	case MODESUBTITLEDATALOWNYBBLE:
		*_pCmd++=ch;
		_rowAddress+=vbi_unham8(ch); // @todo Check validity
	  sprintf(response,"[addChar]MODESUBTITLEDATALOWNYBBLE _rowAddress=%d\n",_rowAddress);
		// std::cerr << response << std::endl;
		mode=MODEGETROW;
		_pkt=_pCmd; // Save the start of this packet
		break;
	case MODEGETROW:
		*_pCmd++=ch;
		*_pCmd=0;	// cap off the string
		charCount--;
		if (charCount<=0) // End of line?
		{
			sprintf(response,"[TCPClient::addChar] MODEGETROW _rowAddress=%d _pkt=%s\n",_rowAddress,_pkt);
			// std::cerr << response << std::endl;
			// Generate the teletext packet
			_newfor.saveSubtitleRow(8,_rowAddress,_pkt);
			if (_row>1) // Next row
			{
				_row--;
				mode=MODESUBTITLEDATAHIGHNYBBLE;
			}
			else // Last row
			{
				// Now that we are done, set up for the next command
				sprintf(response,"subtitle data complete\n");
				mode=MODENORMAL;
				clearCmd();
			}
		}
		break;
	} // State machine switch
}


/** HandleTCPClient
 *  Commands come in here.
 * They need to be accumulated and when we have a complete line, send it to a command interpreter.
 */
void TCPClient::Handler(int clntSocket)
{
	char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
	char response[RCVBUFSIZE];
  int recvMsgSize;                    /* Size of received message */
	int i;
	clearCmd();
	// std::cerr << "[TCPClient::Handler]" << std::endl;

  /* Send received string and receive again until end of transmission */
  for (recvMsgSize=1;recvMsgSize > 0;)      /* zero indicates end of transmission */
  {
    /* See if there is more data to receive */
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
      DieWithError("recv() failed");
		for (i=0;i<recvMsgSize;i++)
		{
			addChar(echoBuffer[i], response);
	   	if (*response) send(clntSocket, response, strlen(response), 0);
		}
			/* Echo message back to client */
			//if (send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize)
			//    DieWithError("send() failed");
  }

#ifdef WIN32
  closesocket(clntSocket);
#else
  close(clntSocket);    /* Close client socket */
#endif // WIN32
}

/** Validate a page identity
 */
bool TCPClient::Validate(char* dest, char* src)
{
  bool valid=true;
  bool pad=false;
  // Copy the target page number, validating as we go
  for (int i=0;i<5 && valid;i++)
  {
    // Get the next input character and validate it
    char ch=*src++;
    // If we hit a null, pad with 0 to the end and stay valid
    if (pad || ch==0)
    {
      pad=true;
      ch='0';
    }
    else
    {
      switch (i)
      {
      case 0: // m - magazine
        if ((ch<'1' || ch>'8')  &&
          (ch!='*'))
        {
          valid=false;
        }
        break;
      case 1: // pp - page number
      case 2:
        // Change case?
        if (ch>='a' && ch<='f')
        {
          ch-='a'-'A';
        }
        if ((ch<'0' || ch>'9') &&
           (ch<'A' || ch>'F') &&
           (ch!='*'))
        {
          valid=false;
        }
        /* fallthrough */
      case 3: // aa - internal subpage (related to subcode but NOT the same thing)
      case 4:
        if ((ch<'0' || ch>'9') &&
           (ch!='*'))
        {
          valid=false;
        }
      } // switch

    }
    *dest++=ch;
  } // for each character
  return valid;
}
