/***************************************************************************
    
copyright (C) 2002-2005 by Tomas Mecir (kmuddy@kmuddy.com)
copyright (c) 2008-2009 by Heiko Koehn (koehnheiko@googlemail.com)
 
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef CTELNET_H
#define CTELNET_H
#include <QObject>
#include <list>
#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <queue>
#include <QQueue>
#include <QTextCodec>
#include <QHostAddress>
#include <QTcpSocket>
#include <QHostInfo>
#include <zlib.h>
#include <QTimer>
#include <QTime>
#include <QColor>

const char TN_SE = 240;
const char TN_NOP = 241;
const char TN_DM = 242;
const char TN_B = 243;
const char TN_IP = 244;
const char TN_AO = 245;
const char TN_AYT = 246;
const char TN_EC = 247;
const char TN_EL = 248;
const char TN_GA = 249;
const char TN_SB = 250;
const char TN_WILL = 251;
const char TN_WONT = 252;
const char TN_DO = 253;
const char TN_DONT = 254;
const char TN_IAC = 255;

const char OPT_ECHO = 1;
const char OPT_SUPPRESS_GA = 3;
const char OPT_STATUS = 5;
const char OPT_TIMING_MARK = 6;
const char OPT_TERMINAL_TYPE = 24;
const char OPT_NAWS = 31;
const char OPT_COMPRESS = 85;
const char OPT_COMPRESS2 = 86;
const char OPT_MSP = 90;
const char OPT_MXP = 91;
const char OPT_ATCP = 200;
const char TNSB_IS = 0;
const char TNSB_SEND = 1;




class mudlet;
class Host;


class cTelnet : public QObject
{
Q_OBJECT

public: 
                      cTelnet( Host * pH );
                     ~cTelnet();
  void                connectIt(const QString &address, int port);
  void                disconnect();
  bool                sendData ( QString & data );
  void                setCommandEcho( bool cmdEcho );
  void                setLPMudStyle ( bool lpmustyle );
  void                setNegotiateOnStartup( bool startupneg );
  void                setDisplayDimensions();
  void                encodingChanged(QString encoding);
  void                set_USE_IRE_DRIVER_BUGFIX( bool b ){ mUSE_IRE_DRIVER_BUGFIX=b; }
  void                set_LF_ON_GA( bool b ){ mLF_ON_GA=b; }
  void                recordReplay();
  void                loadReplay( QString & );
  void                _loadReplay();
  bool                mResponseProcessed;
  QTime               networkLatencyTime;
  double              networkLatency;
  bool                mGA_Driver;
  bool                mAlertOnNewData;

protected slots:
  
  void                readPipe();
  void                handle_socket_signal_hostFound(QHostInfo);
  void                handle_socket_signal_connected();
  void                handle_socket_signal_disconnected();
  void                handle_socket_signal_readyRead();
  void                handle_socket_signal_error();
  void                slot_timerPosting();
  void                slot_send_login();
  void                slot_send_pass();

private:
                      cTelnet(){;}    
  void                initStreamDecompressor();  
  int                 decompressBuffer( char * dirtyBuffer, int length );
  void                postMessage( QString msg );  
  void                reset();
  void                connectionFailed();
  bool                socketOutRaw(std::string & data);
  void                processTelnetCommand (const std::string &command);
  void                sendTelnetOption( char type, char option);
  //string getCurrentTime(); //NOTE: not w32 compatible
  void                gotRest( std::string & );
  void                gotPrompt( std::string & );
  void                postData();  
    
  bool                mUSE_IRE_DRIVER_BUGFIX;
  bool                mLF_ON_GA;
  Host *              mpHost;  
  QTcpSocket          socket;
  QHostAddress        mHostAddress;
  QTextCodec *        incomingDataCodec;
  QTextCodec *        outgoingDataCodec;
  QTextDecoder *      incomingDataDecoder;
  QTextEncoder *      outgoingDataDecoder;
  QString             hostName;
  int                 hostPort;
  QDataStream         mOfs;
  double              networkLatencyMin;
  double              networkLatencyMax;
  bool                mWaitingForResponse;
  std::queue<int>     mCommandQueue;
  int                 mCommands;
  z_stream            mZstream;  
  bool                mMCCP_version_1;
  bool                mMCCP_version_2;
  bool                mNeedDecompression; 
  bool                mWaitingForCompressedStreamToStart;
  std::string         command;
  bool                iac, iac2, insb;
  bool                myOptionState[256], hisOptionState[256];
  bool                announcedState[256];
  bool                heAnnouncedState[256];
  bool                triedToEnable[256];
  bool                recvdGA;

  int                 curX, curY;
  QString             termType;
  QString             encoding;
  QTimer *            mpPostingTimer;
  std::string         mMudData;
  bool                mIsTimerPosting;
  QTimer *            mTimerLogin;
  QTimer *            mTimerPass;
  QTime               timeOffset;
  QTime               mConnectionTime;
  int                 lastTimeOffset;
};

#endif


