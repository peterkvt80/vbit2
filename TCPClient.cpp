#include <stdio.h>      /* for printf() and fprintf() */
#include <string.h>


#include "TCPClient.h"

using namespace vbit;

static int
vbi_unham8			(unsigned int		c)
{
	return _vbi_hamm8_inv[(uint8_t) c];
}

TCPClient::TCPClient() :
	_pCmd(_cmd)
{
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
	switch (cmd[0])
	{
	case 'T' :; 
		if (response) strcpy(response,"T not implemented\n");
		break;
		
	case 'Y' : 
		if (response) strcpy(response,"VBIT620\n");
		break;
	case 0x0e:
		if (response) strcpy(response,"This of course does not work. No CR in Softel \n\r");
		break;
	}
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
	static int charCount=0;

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
			std::cerr << "[TCPClient::addChar] finished cmd=" << _cmd << std::endl;
			command(_cmd, response);
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

  close(clntSocket);    /* Close client socket */
}

