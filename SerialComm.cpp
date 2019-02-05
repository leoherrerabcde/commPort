// SerialComm.cpp: implementation of the CFlag class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SerialComm.h"
#include "Gps.h"

//---------------------------------------------------------------------------
//
//  Module: CommC++.c
//
//  Purpose:
//     The sample application demonstrates the usage of the COMM
//     API.  It implements the new COMM API of Windows 3.1.
//
//     NOTE:  no escape sequences are translated, only
//            the necessary control codes (LF, CR, BS, etc.)
//
//  Functions:
//     Descriptions are contained in the function headers.
//  LRESULT NEAR CreateTTYInfo( HWND hWnd )
//  BOOL NEAR DestroyTTYInfo( HWND hWnd ) /(LLama a CloseConnection/)
//  BOOL NEAR ProcessTTYCharacter( HWND hWnd, BYTE bOut )
//  BOOL NEAR OpenConnection( HWND hWnd )
//  BOOL NEAR CloseConnection( HWND hWnd )
//  BOOL NEAR SetupConnection( HWND hWnd )
//  int NEAR ReadCommBlock( HWND hWnd, LPSTR lpszBlock, int nMaxLength )
//  BOOL NEAR WriteCommBlock( HWND hWnd, BYTE *pByte,DWORD dwBytesToWrite )
//  DWORD FAR PASCAL CommWatchProc( LPSTR lpData )  ***JVM Secondary Thread



//#include "stdlib.h"
//#include "..\..\include\c8051io.h"
//#include "..\..\include\c8051sr.h"

#pragma data_seg(".MYSEG")

  NPTTYINFO gnpTTYINFO = NULL;

#pragma data_seg()

/*extern near struct special_function_bits SFB;
extern HANDLE    ghMod;   // DLL's module handle
extern unsigned int		BaudRateCOMM1;
extern unsigned char	Numero_Puerta;
extern CGps				gcGps;*/

std::string bufferIn;

//---------------------------------------------------------------------------
//  LRESULT NEAR CreateTTYInfo( HWND hWnd )
//
//  Description:
//     Creates the tty information structure and sets
//     menu option availability.  Returns -1 if unsuccessful.
//
//  Parameters:
//     HWND  hWnd
//        Handle to main window.
//
//  Win-32 Porting Issues:
//     - Needed to initialize TERMWND( npTTYInfo ) for secondary thread.
//     - Needed to create/initialize overlapped structures used in reads &
//       writes to COMM device.
//
//---------------------------------------------------------------------------

LRESULT NEAR CreateTTYInfo( HWND hWnd )
{
   NPTTYINFO   npTTYInfo ;

   if (NULL == (npTTYInfo =
                   (NPTTYINFO) LocalAlloc( LPTR, sizeof( TTYINFO ) )))
      return ( (LRESULT) -1 ) ;

   // initialize TTY info structure

   //BaudRateCOMM1 = 9600;
   //Numero_Puerta = 1;
   COMDEV( npTTYInfo )        = 0 ;
   CONNECTED( npTTYInfo )     = TRUE ;/*JVM*/
   // JVM CURSORSTATE( npTTYInfo )   = CS_HIDE ;
   LOCALECHO( npTTYInfo )     = FALSE ;
   AUTOWRAP( npTTYInfo )      = TRUE ;
   //if ((Numero_Puerta) < 1 || (Numero_Puerta) >4) Numero_Puerta = 2;
   PORT( npTTYInfo )          = Numero_Puerta ; /*JVM*/
   BAUDRATE( npTTYInfo )      = BaudRateCOMM1;	// JVM CBR_9600 ;
   BYTESIZE( npTTYInfo )      = 8 ;
   //FLOWCTRL( npTTYInfo )      = FC_RTSCTS ;
   FLOWCTRL( npTTYInfo )      = 0; //FC_XONXOFF ;
   PARITY( npTTYInfo )        = NOPARITY ;
   STOPBITS( npTTYInfo )      = ONESTOPBIT ;
   XONXOFF( npTTYInfo )       = FALSE ;
   //JVM XSIZE( npTTYInfo )         = 0 ;
   //JVM YSIZE( npTTYInfo )         = 0 ;
   //JVM XSCROLL( npTTYInfo )       = 0 ;
   //JVM YSCROLL( npTTYInfo )       = 0 ;
   //JVM XOFFSET( npTTYInfo )       = 0 ;
   //JVM YOFFSET( npTTYInfo )       = 0 ;
   //JVM COLUMN( npTTYInfo )        = 0 ;
   //JVM ROW( npTTYInfo )           = 0 ;
   //JVM HTTYFONT( npTTYInfo )      = NULL ;
   //JVM FGCOLOR( npTTYInfo )       = RGB( 0, 0, 0 ) ;
   USECNRECEIVE( npTTYInfo )  = TRUE ;/*JVM FALSE??**/
   DISPLAYERRORS( npTTYInfo ) = TRUE ;
   WRITE_OS( npTTYInfo ).Offset = 0 ;
   WRITE_OS( npTTYInfo ).OffsetHigh = 0 ;
   READ_OS( npTTYInfo ).Offset = 0 ;
   READ_OS( npTTYInfo ).OffsetHigh = 0 ;
   TERMWND( npTTYInfo ) =       hWnd ;

   // create I/O event used for overlapped reads / writes

   READ_OS( npTTYInfo ).hEvent = CreateEvent( NULL,    // no security
                                              TRUE,    // explicit reset req
                                              FALSE,   // initial event reset
                                              NULL ) ; // no name
   if (READ_OS( npTTYInfo ).hEvent == NULL)
   {
      LocalFree( npTTYInfo ) ;
      return ( -1 ) ;
   }
   WRITE_OS( npTTYInfo ).hEvent = CreateEvent( NULL,    // no security
                                               TRUE,    // explicit reset req
                                               FALSE,   // initial event reset
                                               NULL ) ; // no name
   if (NULL == WRITE_OS( npTTYInfo ).hEvent)
   {
      CloseHandle( READ_OS( npTTYInfo ).hEvent ) ;
      LocalFree( npTTYInfo ) ;
      return ( -1 ) ;
   }

   // clear screen space

  // JVM  _fmemset( SCREEN( npTTYInfo ), ' ', MAXROWS * MAXCOLS ) ;

   // setup default font information

   // set TTYInfo handle before any further message processing.

   //SETNPTTYINFO( hWnd, npTTYInfo ) ;
   SETNPTTYINFO( npTTYInfo ) ;

   // reset the character information, etc.

   return ( (LRESULT) TRUE ) ;

} // end of CreateTTYInfo()

//---------------------------------------------------------------------------
//  BOOL NEAR DestroyTTYInfo( HWND hWnd )
//
//  Description:
//     Destroys block associated with TTY window handle.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - Needed to clean up event objects created during initialization.
//
//---------------------------------------------------------------------------

BOOL NEAR DestroyTTYInfo( HWND hWnd )
{
   NPTTYINFO npTTYInfo ;

   //if (NULL == (npTTYInfo = GETNPTTYINFO( hWnd )))
   if ( NULL == ( npTTYInfo = GETNPTTYINFO ) )
      return ( FALSE ) ;

   // force connection closed (if not already closed)

   if (CONNECTED( npTTYInfo ))
      CloseConnection( hWnd ) ;

   // clean up event objects

   CloseHandle( READ_OS( npTTYInfo ).hEvent ) ;
   CloseHandle( WRITE_OS( npTTYInfo ).hEvent ) ;

   LocalFree( npTTYInfo ) ;
   return ( TRUE ) ;

} // end of DestroyTTYInfo()*/



//-------------------------------------------------------------------
//  BOOL NEAR ProcessTTYCharacter( HWND hWnd, BYTE bOut )
//
//  Description:
//     This simply writes a character to the port and echos it
//     to the TTY screen if fLocalEcho is set.  Some minor
//     keyboard mapping could be performed here.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//     BYTE bOut
//        byte from keyboard
//
//---------------------------------------------------------------------------

BOOL NEAR ProcessTTYCharacter( HWND hWnd, BYTE bOut )
{
   NPTTYINFO  npTTYInfo ;

   //if (NULL == (npTTYInfo = GETNPTTYINFO( hWnd )))
   if (NULL == (npTTYInfo = GETNPTTYINFO))
      return ( FALSE ) ;

   if (!CONNECTED( npTTYInfo ))
      return ( FALSE ) ;

   // a robust app would take appropriate steps if WriteCommBlock failed
   WriteCommBlock( hWnd, ( LPSTR ) &bOut, 1 ) ;
   return ( TRUE ) ;

} // end of ProcessTTYCharacter()

//---------------------------------------------------------------------------
//  BOOL NEAR OpenConnection( HWND hWnd )
//
//  Description:
//     Opens communication port specified in the TTYINFO struct.
//     It also sets the CommState and notifies the window via
//     the fConnected flag in the TTYINFO struct.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - OpenComm() is not supported under Win-32.  Use CreateFile()
//       and setup for OVERLAPPED_IO.
//     - Win-32 has specific communication timeout parameters.
//     - Created the secondary thread for event notification.
//
//---------------------------------------------------------------------------

BOOL NEAR OpenConnection( HWND hWnd , unsigned char	Numero_Puerta)
{
   char       szPort[ 15 ];
   BOOL       fRetVal ;
   // JVM HCURSOR    hOldCursor, hWaitCursor ;
   // JVM HMENU      hMenu ;
   NPTTYINFO  npTTYInfo ;

   HANDLE        hCommWatchThread ;
   DWORD         dwThreadID ;
   COMMTIMEOUTS  CommTimeOuts ;

   //if (NULL == (npTTYInfo = GETNPTTYINFO( hWnd )))
   if (NULL == (npTTYInfo = GETNPTTYINFO))
      return ( FALSE ) ;

   // show the hourglass cursor

   // HACK!  This checks for the PORT number defined by
   // the combo box selection.  If it is greater than the
   // maximum number of ports, assume TELNET.

   // LHE 22 Octubre 2008
   PORT( npTTYInfo )          = Numero_Puerta ;
   // FIN LHE 22 Octubre 2008

   if (PORT( npTTYInfo ) > MAXPORTS)
      lstrcpy( szPort, "\\\\.\\TELNET" ) ;
   else
   {
      // load the COM prefix string and append port number

      //JVM LoadString( GETHINST( hWnd ), IDS_COMPREFIX, szTemp, sizeof( szTemp ) ) ;
      wsprintf( szPort, "%s%d", "COM", PORT( npTTYInfo ) ) ;
   }

   // open COMM device

   if ((COMDEV( npTTYInfo ) =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL |
                  FILE_FLAG_OVERLAPPED, // overlapped I/O
                  NULL )) == (HANDLE) -1 )
      return ( FALSE ) ;
   else
   {
      // get any early notifications

      SetCommMask( COMDEV( npTTYInfo ), EV_RXCHAR ) ; /*JVM TX???*/

      // setup device buffers

      SetupComm( COMDEV( npTTYInfo ), 512, 512 ) ;/*JVM Largo Buffers*/

      // purge any information in the buffer

      PurgeComm( COMDEV( npTTYInfo ), PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

      // set up for overlapped I/O

      CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.ReadTotalTimeoutConstant = 0 ; /*OJO ex 1000*/
      CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.WriteTotalTimeoutConstant = 1000 ;
      SetCommTimeouts( COMDEV( npTTYInfo ), &CommTimeOuts ) ;
   }

   fRetVal = SetupConnection( hWnd ) ;

   if (fRetVal)
   {
      CONNECTED( npTTYInfo ) = TRUE ;

      // Create a secondary thread
      // to watch for an event.

      if (NULL == (hCommWatchThread =
                      CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE) CommWatchProc,
                                    (LPVOID) npTTYInfo,
                                    0, &dwThreadID )))
      {
         CONNECTED( npTTYInfo ) = FALSE ;
         CloseHandle( COMDEV( npTTYInfo ) ) ;
         fRetVal = FALSE ;
      }
      else
      {
         THREADID( npTTYInfo ) = dwThreadID ;
         HTHREAD( npTTYInfo ) = hCommWatchThread ;
		 SetThreadPriority(hCommWatchThread, (int) THREAD_PRIORITY_ABOVE_NORMAL);

         // assert DTR

         EscapeCommFunction( COMDEV( npTTYInfo ), SETDTR ) ;

         //JVM SetTTYFocus( hWnd ) ;

      }
   }
   else
   {
      CONNECTED( npTTYInfo ) = FALSE ;
      CloseHandle( COMDEV( npTTYInfo ) ) ;
   }

   return ( fRetVal ) ;

} // end of OpenConnection()

//---------------------------------------------------------------------------
//  BOOL NEAR SetupConnection( HWND hWnd )
//
//  Description:
//     This routines sets up the DCB based on settings in the
//     TTY info structure and performs a SetCommState().
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - Win-32 requires a slightly different processing of the DCB.
//       Changes were made for configuration of the hardware handshaking
//       lines.
//
//---------------------------------------------------------------------------

BOOL NEAR SetupConnection( HWND hWnd )
{
   BOOL       fRetVal ;
   BYTE       bSet ;
   DCB        dcb ;
   NPTTYINFO  npTTYInfo ;

   //if (NULL == (npTTYInfo = GETNPTTYINFO( hWnd )))
   if (NULL == (npTTYInfo = GETNPTTYINFO))
      return ( FALSE ) ;

   dcb.DCBlength = sizeof( DCB ) ;

   GetCommState( COMDEV( npTTYInfo ), &dcb ) ;

   dcb.BaudRate = BAUDRATE( npTTYInfo ) ;
   dcb.ByteSize = BYTESIZE( npTTYInfo ) ;
   dcb.Parity = PARITY( npTTYInfo ) ;
   dcb.StopBits = STOPBITS( npTTYInfo ) ;

   // setup hardware flow control

   bSet = (BYTE) ((FLOWCTRL( npTTYInfo ) & FC_DTRDSR) != 0) ;
   dcb.fOutxDsrFlow = bSet ;
   if (bSet)
      dcb.fDtrControl = DTR_CONTROL_HANDSHAKE ;
   else
      dcb.fDtrControl = DTR_CONTROL_ENABLE ;

   bSet = (BYTE) ((FLOWCTRL( npTTYInfo ) & FC_RTSCTS) != 0) ;
	dcb.fOutxCtsFlow = bSet ;
   if (bSet)
      dcb.fRtsControl = RTS_CONTROL_HANDSHAKE ;
   else
      dcb.fRtsControl = RTS_CONTROL_ENABLE ;

   // setup software flow control

   bSet = (BYTE) ((FLOWCTRL( npTTYInfo ) & FC_XONXOFF) != 0) ;

   dcb.fInX = dcb.fOutX = bSet ;
   dcb.XonChar = ASCII_XON ;
   dcb.XoffChar = ASCII_XOFF ;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = FALSE ;/*JVM*/

   fRetVal = SetCommState( COMDEV( npTTYInfo ), &dcb ) ;

   return ( fRetVal ) ;

} // end of SetupConnection()

//---------------------------------------------------------------------------
//  BOOL NEAR CloseConnection( HWND hWnd )
//
//  Description:
//     Closes the connection to the port.  Resets the connect flag
//     in the TTYINFO struct.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - Needed to stop secondary thread.  SetCommMask() will signal the
//       WaitCommEvent() event and the thread will halt when the
//       CONNECTED() flag is clear.
//     - Use new PurgeComm() API to clear communications driver before
//       closing device.
//
//---------------------------------------------------------------------------

BOOL NEAR CloseConnection( HWND hWnd )
{
   NPTTYINFO  npTTYInfo ;

   //if (NULL == (npTTYInfo = GETNPTTYINFO( hWnd )))
   if (NULL == (npTTYInfo = GETNPTTYINFO))
      return ( FALSE ) ;

   // set connected flag to FALSE

   CONNECTED( npTTYInfo ) = FALSE ;

   // disable event notification and wait for thread
   // to halt

   SetCommMask( COMDEV( npTTYInfo ), 0 ) ;

   // block until thread has been halted

   while(THREADID(npTTYInfo) != 0);

   // drop DTR

   EscapeCommFunction( COMDEV( npTTYInfo ), CLRDTR ) ;

   // purge any outstanding reads/writes and close device handle

   PurgeComm( COMDEV( npTTYInfo ), PURGE_TXABORT | PURGE_RXABORT |
                                   PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
   CloseHandle( COMDEV( npTTYInfo ) ) ;

   return ( TRUE ) ;

} // end of CloseConnection()

//---------------------------------------------------------------------------
//  int NEAR ReadCommBlock( HWND hWnd, LPSTR lpszBlock, int nMaxLength )
//
//  Description:
//     Reads a block from the COM port and stuffs it into
//     the provided buffer.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//     LPSTR lpszBlock
//        block used for storage
//
//     int nMaxLength
//        max length of block to read
//
//  Win-32 Porting Issues:
//     - ReadComm() has been replaced by ReadFile() in Win-32.
//     - Overlapped I/O has been implemented.
//
//---------------------------------------------------------------------------

int NEAR ReadCommBlock( HWND hWnd, LPSTR lpszBlock, int nMaxLength )
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	DWORD      dwError;
	char       szError[ 10 ] ;
	NPTTYINFO  npTTYInfo ;

	//if (NULL == (npTTYInfo = GETNPTTYINFO( hWnd )))
	if (NULL == (npTTYInfo = GETNPTTYINFO))
		return ( FALSE ) ;

	// only try to read number of bytes in queue
	ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
	dwLength = min( (DWORD) nMaxLength, ComStat.cbInQue ) ;

	if (dwLength > 0)
	{
		ResetEvent(READ_OS( npTTYInfo ).hEvent);		/*JVM OJO*/
		fReadStat = ReadFile( COMDEV( npTTYInfo ), lpszBlock,
		                    dwLength, &dwLength, &READ_OS( npTTYInfo ) ) ;
		if (!fReadStat)
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				OutputDebugString("\n\rIO Pending");
				// We have to wait for read to complete.
				// This function will timeout according to the
				// CommTimeOuts.ReadTotalTimeoutConstant variable
				// Every time it times out, check for port errors
				while(!GetOverlappedResult( COMDEV( npTTYInfo ),
					&READ_OS( npTTYInfo ), &dwLength, TRUE ))
				{
					dwError = GetLastError();
					if(dwError == ERROR_IO_INCOMPLETE)
						// normal result if not finished
						continue;
					else
					{
						// an error occurred, try to recover
						wsprintf( szError, "<CE-%u>", dwError ) ;
						OutputDebugString(szError);
						//WriteTTYBlock( hWnd, szError, lstrlen( szError ) ) ;
						ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
						if ((dwErrorFlags > 0) && DISPLAYERRORS( npTTYInfo ))
						{
							wsprintf( szError, "<CE-%u>", dwErrorFlags ) ;
							OutputDebugString(szError);
							//WriteTTYBlock( hWnd, szError, lstrlen( szError ) ) ;
						}
						break;
					}

				}

			}
			else
			{
			    // some other error occurred

			    dwLength = 0 ;
				ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
				if ((dwErrorFlags > 0) && DISPLAYERRORS( npTTYInfo ))
				{
					wsprintf( szError, "<CE-%u>", dwErrorFlags ) ;
					OutputDebugString(szError);
					//WriteTTYBlock( hWnd, szError, lstrlen( szError ) ) ;
				}
			}
		}
	}

   return ( dwLength ) ;

} // end of ReadCommBlock()

//---------------------------------------------------------------------------
//  BOOL NEAR WriteCommBlock( HWND hWnd, BYTE *pByte, DWORD dwBytesToWrite )
//
//  Description:
//     Writes a block of data to the COM port specified in the associated
//     TTY info structure.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//     BYTE *pByte
//        pointer to data to write to port
//
//  Win-32 Porting Issues:
//     - WriteComm() has been replaced by WriteFile() in Win-32.
//     - Overlapped I/O has been implemented.
//
//---------------------------------------------------------------------------

BOOL NEAR WriteCommBlock( HWND hWnd, LPSTR lpByte , DWORD dwBytesToWrite)
{

	BOOL        fWriteStat ;
	DWORD       dwBytesWritten ;
	NPTTYINFO   npTTYInfo ;
	DWORD       dwErrorFlags;
	DWORD   	dwError;
	COMSTAT     ComStat;
	char        szError[ 10 ] ;


	//if (NULL == (npTTYInfo = GETNPTTYINFO( hWnd )))
	if (NULL == (npTTYInfo = GETNPTTYINFO))
		return ( FALSE ) ;
	ResetEvent(WRITE_OS( npTTYInfo ).hEvent);		/*JVM OJO*/
	fWriteStat = WriteFile( COMDEV( npTTYInfo ), lpByte, dwBytesToWrite,
	                       &dwBytesWritten, &WRITE_OS( npTTYInfo ) ) ;

	// Note that normally the code will not execute the following
	// because the driver caches write operations. Small I/O requests
	// (up to several thousand bytes) will normally be accepted
	// immediately and WriteFile will return true even though an
	// overlapped operation was specified

	if (!fWriteStat)
	{
		if(GetLastError() == ERROR_IO_PENDING)
		{
			// We should wait for the completion of the write operation
			// so we know if it worked or not

			// This is only one way to do this. It might be beneficial to
			// place the writing operation in a separate thread
			// so that blocking on completion will not negatively
			// affect the responsiveness of the UI

			// If the write takes long enough to complete, this
			// function will timeout according to the
			// CommTimeOuts.WriteTotalTimeoutConstant variable.
			// At that time we can check for errors and then wait
			// some more.

			if (!GetOverlappedResult( COMDEV( npTTYInfo ),	/**JVM solo una vez aqui*/
				&WRITE_OS( npTTYInfo ), &dwBytesWritten, TRUE )) /*el resto de la espera se hara en thread comm*/
			{
				dwError = GetLastError();
				if(!(dwError == ERROR_IO_INCOMPLETE))	// normal result if not finished
				{
					// an error occurred, try to recover
					wsprintf( szError, "<CE-%u>", dwError ) ;
					OutputDebugString(szError);
					ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
					if ((dwErrorFlags > 0) && DISPLAYERRORS( npTTYInfo ))
					{
						wsprintf( szError, "<CE-%u>", dwErrorFlags ) ;
						OutputDebugString(szError);
					}

				}
			}
		}
		else
		{
			// some other error occurred

			ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
			if ((dwErrorFlags > 0) && DISPLAYERRORS( npTTYInfo ))
			{
				wsprintf( szError, "<CE-%u>", dwErrorFlags ) ;
				OutputDebugString(szError);
			}
			return ( FALSE );
		}
	}
	return ( TRUE ) ;

} // end of WriteCommBlock()




//************************************************************************
//  DWORD FAR PASCAL CommWatchProc( LPSTR lpData )
//
//  Description:
//     A secondary thread that will watch for COMM events.
//
//  Parameters:
//     LPSTR lpData
//        32-bit pointer argument
//
//  Win-32 Porting Issues:
//     - Added this thread to watch the communications device and
//       post notifications to the associated window.
//
//************************************************************************

DWORD FAR PASCAL CommWatchProc( LPSTR lpData )
{
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	//DWORD      dwLength;
	DWORD      dwError;
	char       szError[ 10 ] ;
   DWORD       dwEvtMask, dwBytesWritten ;
   NPTTYINFO   npTTYInfo = (NPTTYINFO) lpData ;
   OVERLAPPED  os ;
	int        nLength ;
   BYTE       abIn[ MAXBLOCK + 1] ;

   memset( &os, 0, sizeof( OVERLAPPED ) ) ;

   // create I/O event used for overlapped read

   os.hEvent = CreateEvent( NULL,    // no security
                            TRUE,    // explicit reset req
                            FALSE,   // initial event reset
                            NULL ) ; // no name
   if (os.hEvent == NULL)
   {
      MessageBox( NULL, "Failed to create event for thread!", "TTY Error!",
                  MB_ICONEXCLAMATION | MB_OK ) ;
      return ( FALSE ) ;
   }

   if (!SetCommMask( COMDEV( npTTYInfo ), EV_RXCHAR | EV_TXEMPTY ))
      return ( FALSE ) ;

   while ( CONNECTED( npTTYInfo ) )
   {
		dwEvtMask = 0 ;

		WaitCommEvent( COMDEV( npTTYInfo ), &dwEvtMask, NULL );

		if ((dwEvtMask & EV_RXCHAR) == EV_RXCHAR)
		{

			do
		   {
				//if (nLength = ReadCommBlock( hTTYWnd, (LPSTR) abIn, MAXBLOCK ))
				if (nLength = ReadCommBlock( NULL, (LPSTR) abIn, MAXBLOCK ))
				{
					/*---------------------------*/
					/*Código GPS */
					/*---------------------------*/
					abIn[nLength]='\0';
					bufferIn += ( ( LPSTR ) abIn );
					//gstrDataComm += (LPSTR) abIn;
					//ProccessDataGps ( CString );
		      	}
		   }
		   while ( nLength > 0 ) ;
		}
		if ((dwEvtMask & EV_TXEMPTY) == EV_TXEMPTY)
		{
			//__TxCharCOMM1 = TRUE;
		//	MessageBox(ghMod, "Char Was Sent","titulo", MB_OK);
			while (!GetOverlappedResult( COMDEV( npTTYInfo ),	/*aqui se espera termino envio Tx*/
				&WRITE_OS( npTTYInfo ), &dwBytesWritten, TRUE ))
			{
				dwError = GetLastError();
				if((dwError == ERROR_IO_INCOMPLETE))
					// normal result if not finished
					continue;
				else
				{
					// an error occurred, try to recover
					wsprintf( szError, "<CE-%u>", dwError ) ;
					OutputDebugString(szError);
					ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
					if ((dwErrorFlags > 0) && DISPLAYERRORS( npTTYInfo ))
					{
						wsprintf( szError, "<CE-%u>", dwErrorFlags ) ;
						OutputDebugString(szError);
					}
					break;
				}
				//__TxFrameCOMM1 = TRUE; /*señaliza que se tramsimitio frame completo*/
				//__TxCharCOMM1 = TRUE;

			}

		}
   }

   // get rid of event handle

   CloseHandle( os.hEvent ) ;

   // clear information in structure (kind of a "we're done flag")

   THREADID( npTTYInfo ) = 0 ;
   HTHREAD( npTTYInfo ) = NULL ;

   return( TRUE ) ;

} // end of CommWatchProc()



