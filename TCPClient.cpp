#include <stdio.h>      /* for printf() and fprintf() */
#include <string.h>



#include "TCPClient.h"

using namespace vbit;

void DieWithError(char *errorMessage);  /* Error handling function */


static char* pCmd;

static int _row; // Row counter

static char* pkt;

static int rowAddress; // The address of this row

void command(char* cmd, char* response)
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
}

// Normal command mode
#define MODENORMAL 0
#define MODESOFTELPAGEINIT 1
// Get row count
#define MODEGETROWCOUNT 3
// Get a row of data
#define MODEGETROW 4
// Display the row
#define MODESUBTITLEONAIR 5
// Clear down
#define MODESUBTITLEOFFAIR 6
#define MODESUBTITLEDATAHIGHNYBBLE 7
#define MODESUBTITLEDATALOWNYBBLE 8


/** AddChar
 * Add a char to the command buffer and call command when we get CR 
 * There is only a response if a command needs to send something back.
 * If the first character is a Nu4 character then the data is parsed as Nu4.
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
	int n;
	response[0]=0;
	switch (mode)
	{
	case MODENORMAL :
		pkt=_cmd;
		if (pCmd==_cmd) // On the first character we check if it is a Softel
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
				SubtitleOnair(response);
//				strcpy(response, "On Air");
				mode=MODENORMAL;			
				clearCmd();
				return;
			case 0x18 :
				// Remove the subtitle immediately
				SubtitleOffair();
				strcpy(response, "[addChar]Clear");
				clearCmd();
				mode=MODENORMAL;			
				return;
			}
		} // If first character
		if (ch!='\n' && ch!='\r')
		{
			*pCmd++=ch;
			if (response) response[0]=0;
		}
		else
		{
			// Got a complete command
			// printf("cmd=%s\n",_cmd);
			command(_cmd, response);
			clearCmd();
			mode=MODENORMAL;			
		}
		break;
	case MODESOFTELPAGEINIT:	// We get four more characters and then the page is set	
		// @todo If a nybble fails deham or isn't in range we should return nack
		*pCmd++=ch;
		charCount--;
		if (!charCount)
		{
			int page=SoftelPageInit(_cmd);
			sprintf(response,"[addChar]MODESOFTELPAGEINIT Set page=%03x",page);
			// Now that we are done, set up for the next command
			mode=MODENORMAL;
			clearCmd();
		}
		break;
	case MODEGETROWCOUNT:
		*pCmd++=ch;
		n=GetRowCount(_cmd);
	    sprintf(response,"[addChar]Row count=%d\n",n);
		mode=MODESUBTITLEDATAHIGHNYBBLE;
		_row=n;
		break;
	case MODESUBTITLEDATAHIGHNYBBLE:
		*pCmd++=ch;
		charCount=40;
		mode=MODESUBTITLEDATALOWNYBBLE;
		rowAddress=vbi_unham8(ch)*16; // @todo Check validity
		break;
	case MODESUBTITLEDATALOWNYBBLE:
		*pCmd++=ch;
		rowAddress+=vbi_unham8(ch); // @todo Check validity
	    sprintf(response,"[addChar]MODESUBTITLEDATALOWNYBBLE row=%d\n",rowAddress);
		mode=MODEGETROW;
		pkt=pCmd; // Save the start of this packet
		break;
	case MODEGETROW:
		*pCmd++=ch;
		charCount--;
		if (charCount<=0) // End of line?
		{
			sprintf(response,"[addChar] MODEGETROW_row=%d\n",_row);
			// Generate the teletext packet
			saveSubtitleRow(8,rowAddress,pkt);
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
 *  Commands will come in here.
 * They need to be accumulated and when we have a complete line, send it to a command interpreter.
 */
void TCPClient::Handler(int clntSocket)
{
  char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
	char response[RCVBUFSIZE];
  int recvMsgSize;                    /* Size of received message */
	int i;
	clearCmd();
	
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
	// printf("Done Handle TCP\n");
}

