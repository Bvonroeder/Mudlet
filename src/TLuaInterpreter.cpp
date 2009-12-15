/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <QDebug>
#include <QDir>
#include <QString>
#include "TLuaInterpreter.h"
#include "TForkedProcess.h"
#include "TTrigger.h"
#include "HostManager.h"
#include "mudlet.h"
#include "TDebug.h"
#include <list>
#include <string>
#include "TEvent.h"

extern "C" 
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

extern QStringList gSysErrors;

using namespace std;

map<lua_State*, Host*> TLuaInterpreter::luaInterpreterMap;

TLuaInterpreter::TLuaInterpreter( Host * pH, int id )
:mpHost( pH )
,mHostID( id )
,purgeTimer(this)
{
  		pGlobalLua = 0;		
     
    connect(this,SIGNAL(signalEchoMessage(int, QString)), this,SLOT(slotEchoMessage(int,QString)));//,Qt::DirectConnection);
    connect(this,SIGNAL(signalNewCommand(int,QString)), this,SLOT(slotNewCommand(int,QString)));//,Qt::QueuedConnection);
    
    connect(this,SIGNAL(signalOpenUserWindow(int,QString)), this,SLOT(slotOpenUserWindow(int,QString)));
    connect(this,SIGNAL(signalEchoUserWindow(int,QString,QString)), this,SLOT(slotEchoUserWindow(int,QString,QString)));
    connect(this,SIGNAL(signalEnableTimer(int,QString)),this,SLOT(slotEnableTimer(int,QString)));
    connect(this,SIGNAL(signalDisableTimer(int,QString)),this,SLOT(slotDisableTimer(int,QString)));
    connect(this,SIGNAL(signalClearUserWindow(int,QString)),this,SLOT(slotClearUserWindow(int,QString)));
    
    connect(this, SIGNAL(signalSelect(int, QString, int)), this, SLOT(slotSelect(int,QString,int)));
    connect(this, SIGNAL(signalSelectSection(int, int,int)), this, SLOT(slotSelectSection(int,int,int)));
    connect(this, SIGNAL(signalTempTimer(int, double,QString,QString)), this, SLOT(slotTempTimer(int,double,QString,QString)));
    
    connect(this, SIGNAL(signalReplace(int, QString)), this, SLOT(slotReplace(int,QString)));    
    connect(this, SIGNAL(signalSetFgColor(int, int,int,int)), this, SLOT(slotSetFgColor(int,int,int,int)));    
    connect(this, SIGNAL(signalSetBgColor(int, int,int,int)), this, SLOT(slotSetBgColor(int,int,int,int)));        
    connect(&purgeTimer, SIGNAL(timeout()), this, SLOT(slotPurge()));
    initLuaGlobals();

    purgeTimer.start(2000);
}

lua_State * TLuaInterpreter::getLuaExecutionUnit( int unit )
{
    switch( unit )
    {
        case 1:
            return pGlobalLua;
        case 2:
            return pGlobalLua;
        case 3:
            return pGlobalLua;
        case 4:
            return pGlobalLua;
        case 5:
            return pGlobalLua;
    };
    qDebug()<<"MUDLET ERROR: TLuaInterpreter::getLuaExecutionUnit() execution unit undefined";
    return 0;
}

void TLuaInterpreter::slotDeleteSender() {
    objectsToDelete.append(sender());
}

void TLuaInterpreter::slotPurge() {
    while (!objectsToDelete.isEmpty()) {
        delete objectsToDelete.takeFirst();
    }
}


int TLuaInterpreter::Wait( lua_State *L )
{
  int n = lua_gettop( L );
  if(n!=1)
  {
      lua_pushstring( L, "wrong number of arguments" );
      lua_error( L );
      return 1;
  }

  int luaSleepMsec;
  if( ! lua_isnumber( L, 1 ) ) 
  {
      lua_pushstring( L, "wrong argument type" );
      lua_error( L );
      return 1;
  }
  else
  {
      luaSleepMsec = lua_tointeger( L, 1 );
  }
  msleep( luaSleepMsec );//FIXME thread::sleep()
  return 0;
}

int TLuaInterpreter::raiseEvent( lua_State * L )
{
    TEvent * pE = new TEvent;
    
    int n = lua_gettop( L );
    for( int i=1; i<=n; i++)
    {
        if( lua_isnumber( L, i ) )
        {
            pE->mArgumentList.append( QString::number(lua_tonumber( L, i ) ) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        }
        else if( lua_isstring( L, i ) )
        {
            pE->mArgumentList.append( QString(lua_tostring( L, i )) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->raiseEvent( pE );
    return 0;
}

// cursorPositionInLine = select( text ) if not found -1
int TLuaInterpreter::select( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
          lua_pushstring( L, "wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    string luaSendText="";
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, s );
        s++;
    }
    int luaNumOfMatch;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaNumOfMatch = lua_tointeger( L, s );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    if( n == 2 )
    {
        int pos = pHost->mpConsole->select( QString( luaSendText.c_str() ), luaNumOfMatch );
        lua_pushnumber( L, pos );
        return 1;
    }
    else
    {
        QString _name(a1.c_str());
        int pos = mudlet::self()->selectString( _name, QString( luaSendText.c_str() ), luaNumOfMatch );
        lua_pushnumber( L, pos );
        return 1;
    }
}

int TLuaInterpreter::selectCurrentLine( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->selectCurrentLine( luaSendText );
    return 0;
}

int TLuaInterpreter::isAnsiFgColor( lua_State * L )
{
    int ansiFg;

    std::string console = "main";

    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        ansiFg = lua_tointeger( L, 1 );
    }

    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    result = pHost->mpConsole->getFgColor( console );
    typedef std::list<int>::iterator IT;
    IT it=result.begin();
    if( result.size() < 3 ) return 0;
    if( ansiFg < 0 ) return 0;
    if( ansiFg > 16 ) return 0;


    QColor c;
    switch( ansiFg )
    {
        case 0: c = pHost->mFgColor;  break;
        case 1: c = pHost->mLightBlack; break;
        case 2: c = pHost->mBlack; break;
        case 3: c = pHost->mLightRed; break;
        case 4: c = pHost->mRed; break;
        case 5: c = pHost->mLightGreen; break;
        case 6: c = pHost->mGreen; break;
        case 7: c = pHost->mLightYellow; break;
        case 8: c = pHost->mYellow; break;
        case 9: c = pHost->mLightBlue; break;
        case 10: c = pHost->mBlue; break;
        case 11: c = pHost->mLightMagenta; break;
        case 12: c = pHost->mMagenta; break;
        case 13: c = pHost->mLightCyan; break;
        case 14: c = pHost->mCyan; break;
        case 15: c = pHost->mLightWhite; break;
        case 16: c = pHost->mWhite; break;
    }

    int val = *it;
    if( val == c.red() )
    {
        it++;
        val = *it;
        if( val == c.green() )
        {
            it++;
            val = *it;
            if( val == c.blue() )
            {
                lua_pushboolean( L, 1 );
                return 1;
            }
        }
    }

    lua_pushboolean( L, 0 );
    return 1;
}

int TLuaInterpreter::isAnsiBgColor( lua_State * L )
{
    int ansiFg;

    std::string console = "main";

    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        ansiFg = lua_tointeger( L, 1 );
    }

    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( ! pHost ) return 0;
    result = pHost->mpConsole->getBgColor( console );
    typedef std::list<int>::iterator IT;
    IT it=result.begin();
    if( result.size() < 3 ) return 0;
    if( ansiFg < 0 ) return 0;
    if( ansiFg > 16 ) return 0;


    QColor c;
    switch( ansiFg )
    {
        case 0: c = pHost->mBgColor;  break;
        case 1: c = pHost->mLightBlack; break;
        case 2: c = pHost->mBlack; break;
        case 3: c = pHost->mLightRed; break;
        case 4: c = pHost->mRed; break;
        case 5: c = pHost->mLightGreen; break;
        case 6: c = pHost->mGreen; break;
        case 7: c = pHost->mLightYellow; break;
        case 8: c = pHost->mYellow; break;
        case 9: c = pHost->mLightBlue; break;
        case 10: c = pHost->mBlue; break;
        case 11: c = pHost->mLightMagenta; break;
        case 12: c = pHost->mMagenta; break;
        case 13: c = pHost->mLightCyan; break;
        case 14: c = pHost->mCyan; break;
        case 15: c = pHost->mLightWhite; break;
        case 16: c = pHost->mWhite; break;
    }

    int val = *it;
    if( val == c.red() )
    {
        it++;
        val = *it;
        if( val == c.green() )
        {
            it++;
            val = *it;
            if( val == c.blue() )
            {
                lua_pushboolean( L, 1 );
                return 1;
            }
        }
    }

    lua_pushboolean( L, 0 );
    return 1;
}

int TLuaInterpreter::getFgColor( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }
    QString _name(luaSendText.c_str());
    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    result = pHost->mpConsole->getFgColor( luaSendText );
    typedef std::list<int>::iterator IT;
    for( IT it=result.begin(); it!=result.end(); it++ )
    {
        int pos = *it;
        lua_pushnumber( L, pos );
    }
    return result.size();
}

int TLuaInterpreter::getBgColor( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }

    std::list<int> result;
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    result = pHost->mpConsole->getBgColor( luaSendText );
    typedef std::list<int>::iterator IT;
    for( IT it=result.begin(); it!=result.end(); it++ )
    {
        int pos = *it;
        lua_pushnumber( L, pos );
    }
    return result.size();
}

int TLuaInterpreter::wrapLine( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1 = "main";
    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
          lua_pushstring( L, "wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }

    int luaNumOfMatch;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaNumOfMatch = lua_tointeger( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->luaWrapLine( a1, luaNumOfMatch );
    return 0;
}



int TLuaInterpreter::spawn( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    return TForkedProcess::startProcess(pHost->getLuaInterpreter(), L);
}



// cursorPositionInLine = selectCaptureGroup( groupNumber ) if not found -1
int TLuaInterpreter::selectCaptureGroup( lua_State * L )
{
    int luaNumOfMatch;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaNumOfMatch = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    if( luaNumOfMatch < 1 )
    {
        lua_pushnumber( L, -1 );
        return 1;
    }
    luaNumOfMatch--; //we want capture groups to start with 1 instead of 0
    if( luaNumOfMatch < pHost->getLuaInterpreter()->mCaptureGroupList.size() )
    {
        TLuaInterpreter * pL = pHost->getLuaInterpreter();
        std::list<std::string>::iterator its = pL->mCaptureGroupList.begin();
        std::list<int>::iterator iti = pL->mCaptureGroupPosList.begin();
        
        for( int i=0; iti!=pL->mCaptureGroupPosList.end(); ++iti,++i )
        {
            if( i >= luaNumOfMatch ) break;
        }
        for( int i=0; its!=pL->mCaptureGroupList.end(); ++its,++i)
        {
            if( i >= luaNumOfMatch ) break;
        }
        
        int begin = *iti;
        std::string & s = *its;
        int length = s.size();
        //cout << "selectSection("<<begin<<", "<<length<<")"<<endl;
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"selectCaptureGroup("<<begin<<", "<<length<<")\n">>0;}
        int pos = pHost->mpConsole->selectSection( begin, length ); 
        lua_pushnumber( L, pos );
    }
    else
    {
        lua_pushnumber( L, -1 );
    }
    return 1;
}

// luaTable result[line_number, content] = getLines( from_cursorPos, to_cursorPos )
int TLuaInterpreter::getLines( lua_State * L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tointeger( L, 1 );
    }      
    
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo=lua_tointeger( L, 2 );
    }      
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QStringList strList = pHost->mpConsole->getLines( luaFrom, luaTo );
    
    lua_newtable(L);
    for( int i=0; i<strList.size(); i++ )
    {
        lua_pushnumber( L, i+1 );
        lua_pushstring( L, strList[i].toLatin1().data() );
        lua_settable(L, -3);
    }
    return 1;
}

// luaTable result[line_number, content] = getLines( from_cursorPos, to_cursorPos )
int TLuaInterpreter::getBufferTable( lua_State * L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tointeger( L, 1 );
    }      
    
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo=lua_tointeger( L, 2 );
    }      
    /*Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QStringList strList = pHost->getBufferTable( luaFrom, luaTo );
    if( mudlet::debugMode ) qDebug()<<"TLuaInterpreter::getBufferTable() strList="<<strList;
    lua_newtable(L);
    for( int i=0; i<strList.size(); i++ )
    {
        lua_pushnumber( L, i+1 );
        lua_pushstring( L, strList[i].toLatin1().data() );
        lua_settable(L, -3);
    } */
    return 0;
}

int TLuaInterpreter::loadRawFile( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->loadRawFile( luaSendText );
    return 0;
}

int TLuaInterpreter::getCurrentLine( lua_State * L )
{
    string luaSendText="";
    if( lua_gettop( L ) == 0 )
    {
        luaSendText = "main";
    }
    else
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaSendText = lua_tostring( L, 1 );
        }
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString line = pHost->mpConsole->getCurrentLine( luaSendText );
    lua_pushstring( L, line.toLatin1().data() );
    return 1;
}

int TLuaInterpreter::setMiniConsoleFontSize( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int luaNumOfMatch;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaNumOfMatch = lua_tointeger( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->setMiniConsoleFontSize( luaSendText, luaNumOfMatch );
    return 0;
}

// returns current y position of the user cursor
int TLuaInterpreter::getLineNumber( lua_State * L )
{
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    int lineNumber = pHost->mpConsole->getLineNumber();
    lua_pushnumber( L, lineNumber );
    return 1;
}

int TLuaInterpreter::copy( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->copy();
    return 0;
}
int TLuaInterpreter::cut( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->cut();
    return 0;
}
int TLuaInterpreter::paste( lua_State * L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }
    else
        luaWindowName = "main";

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( luaWindowName == "main" )
        pHost->mpConsole->paste();
    else
       mudlet::self()->pasteWindow( pHost, windowName );
    return 0;
}

int TLuaInterpreter::setWindowWrap( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    int luaFrom;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tointeger( L, 2 );
    }      
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString name = luaSendText.c_str();
    mudlet::self()->setWindowWrap( pHost, name, luaFrom );
    return 0;
}

int TLuaInterpreter::setWindowWrapIndent( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    int luaFrom;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tointeger( L, 2 );
    }      
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString name = luaSendText.c_str();
    mudlet::self()->setWindowWrapIndent( pHost, name, luaFrom );
    return 0;
}

int TLuaInterpreter::getLineCount( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    int lineNumber = pHost->mpConsole->getLineCount();
    lua_pushnumber( L, lineNumber );
    return 1;
}

int TLuaInterpreter::getColumnNumber( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    int lineNumber = pHost->mpConsole->getColumnNumber();
    lua_pushnumber( L, lineNumber );
    return 1; 
}

int TLuaInterpreter::getStopWatchTime( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double time = pHost->getStopWatchTime( watchID );
    lua_pushnumber( L, time );
    return 1;
}

int TLuaInterpreter::createStopWatch( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double watchID = pHost->createStopWatch();
    lua_pushnumber( L, watchID );
    return 1;
}

int TLuaInterpreter::stopStopWatch( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double time = pHost->stopStopWatch( watchID );
    lua_pushnumber( L, time );
    return 1;
}

int TLuaInterpreter::startStopWatch( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    bool b = pHost->startStopWatch( watchID );
    lua_pushboolean( L, b );
    return 1;
}

int TLuaInterpreter::resetStopWatch( lua_State * L )
{
    int watchID;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        watchID = lua_tointeger( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    bool b = pHost->resetStopWatch( watchID );
    lua_pushboolean( L, b );
    return 1;
}




// cusorPositionInLine = selectSection( from_cursorPos, to_cursorPos ) -1 on not found
int TLuaInterpreter::selectSection( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
          lua_pushstring( L, "wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    int luaFrom;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tointeger( L, s );
        s++;
    }      
    
    int luaTo;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo=lua_tointeger( L, s );
    }      

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 

    bool ret;
    if( n > 2 )
    {
        QString _name = a1.c_str();
        ret = mudlet::self()->selectSection( _name, luaFrom, luaTo );
    }
    else
    {
        ret = pHost->mpConsole->selectSection( luaFrom, luaTo );
    }
    lua_pushboolean( L, ret );
    return 1;
}


int TLuaInterpreter::moveCursor( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
          lua_pushstring( L, "wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    int luaFrom;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, s );
        s++;
    }

    int luaTo;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo=lua_tointeger( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    if( a1 == "main" || n < 3 )
        lua_pushboolean( L, pHost->mpConsole->moveCursor( luaFrom, luaTo ) );
    else
    {
        QString windowName = a1.c_str();
        lua_pushboolean( L, mudlet::self()->moveCursor( windowName, luaFrom, luaTo ) );
    }
    return 1;
}

int TLuaInterpreter::setConsoleBufferSize( lua_State * L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    if( n > 2 )
    {
        if( ! lua_isstring( L, s ) )
        {
          lua_pushstring( L, "wrong argument type" );
          lua_error( L );
          return 1;
        }
        else
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    int luaFrom;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, s );
        s++;
    }

    int luaTo;
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo=lua_tointeger( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    if( a1 == "main" || n < 3 )
    {
        pHost->mpConsole->buffer.setBufferSize( luaFrom, luaTo );
    }
    else
    {
        QString windowName = a1.c_str();
        mudlet::self()->setConsoleBufferSize( pHost, windowName, luaFrom, luaTo );
    }
    return 0;
}

int TLuaInterpreter::getBufferLine( lua_State * L )
{
    int luaLine;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaLine = lua_tointeger( L, 1 );
    }
    
    /*Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString line = pHost->getBufferLine( luaLine );
    if( mudlet::debugMode ) qDebug()<<"TLuaInterpreter::getBufferLine() line="<<line;
    lua_pushstring( L, line.toLatin1().data() );*/
    return 0;
}

// replace( sessionID, replace_with )
int TLuaInterpreter::replace( lua_State * L )
{
    string a1 = "";
    string a2 = "";
    int n = lua_gettop( L );
    int s = 1;
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a1 = lua_tostring( L, s );
        s++;
    }

    QString _name( a1.c_str() );
    string luaSendText="";
    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a2 = lua_tostring( L, s );
        }
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    if( n == 1 )
        pHost->mpConsole->replace( QString(a1.c_str()) );
    else
        mudlet::self()->replace( _name, QString(a2.c_str()) );
    return 0;
}

int TLuaInterpreter::deleteLine( lua_State * L )
{
    string name="";
    if( lua_gettop( L ) == 1 )
    {
        if( ! lua_isstring( L, 1 ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            name = lua_tostring( L, 1 );
        }
    }

    QString _name( name.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    if( name == "" )
        pHost->mpConsole->skipLine();
    else
        mudlet::self()->deleteLine( _name );
    return 0;
}

// enableTimer( sess, timer_name )
int TLuaInterpreter::enableTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    bool error = pHost->getTimerUnit()->enableTimer( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    bool error = pHost->getTimerUnit()->disableTimer( text );
    lua_pushboolean( L, error );
    return 1;
}

int TLuaInterpreter::enableKey( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    bool error = pHost->getKeyUnit()->enableKey( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableKey( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    bool error = pHost->getKeyUnit()->disableKey( text );
    lua_pushboolean( L, error );
    return 1;
}

int TLuaInterpreter::enableAlias( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getAliasUnit()->enableAlias( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableAlias( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    bool error = pHost->getAliasUnit()->disableAlias( text );
    lua_pushboolean( L, error );
    return 1;
}

int TLuaInterpreter::killAlias( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    lua_pushboolean( L, pHost->getAliasUnit()->killAlias( text ) );
    return 1;
}

// enableTimer( sess, timer_name )
int TLuaInterpreter::enableTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    bool error = pHost->getTriggerUnit()->enableTrigger( text );
    lua_pushboolean( L, error );
    return 1;
}

// disableTimer( session, timer_name )
int TLuaInterpreter::disableTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    bool error = pHost->getTriggerUnit()->disableTrigger( text );
    lua_pushboolean( L, error );
    return 1;
}


int TLuaInterpreter::killTimer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    //pHost->disableTimer( text );
    lua_pushboolean( L, pHost->killTimer( text ) );
    return 1;
}

int TLuaInterpreter::killTrigger( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    lua_pushboolean( L, pHost->killTrigger( text ) );
    return 1;
}

// openUserWindow( session, string window_name )
int TLuaInterpreter::openUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    mudlet::self()->openWindow( pHost, text );
    return 0;
}

int TLuaInterpreter::createMiniConsole( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int x,y,width,height;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 4 );
    }
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 5 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name(luaSendText.c_str());
    lua_pushboolean( L, mudlet::self()->createMiniConsole( pHost, name, x, y, width, height ) );
    return 1;
}

int TLuaInterpreter::createLabel( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int x,y,width,height;
    bool fillBackground=false;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 4 );
    }
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 5 );
    }
    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fillBackground = lua_toboolean( L, 6 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name(luaSendText.c_str());
    lua_pushboolean( L, mudlet::self()->createLabel( pHost, name, x, y, width, height, fillBackground ) );
    return 1;
}

int TLuaInterpreter::createButton( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    int x,y,width,height;
    bool fillBackground=false;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 2 );
    }
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y = lua_tonumber( L, 3 );
    }
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        width = lua_tonumber( L, 4 );
    }
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        height = lua_tonumber( L, 5 );
    }
    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        fillBackground = lua_toboolean( L, 6 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name(luaSendText.c_str());
    //TODO FIXME
    mudlet::self()->createLabel( pHost, name, x, y, width, height, fillBackground );
    return 0;
}


int TLuaInterpreter::createBuffer( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->createBuffer( pHost, text );
    return 0;
}

int TLuaInterpreter::clearUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    mudlet::self()->clearWindow( pHost, text );
    
    return 0;
}

int TLuaInterpreter::closeUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->hideWindow( pHost, text );

    return 0;
}

int TLuaInterpreter::hideUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    //mudlet::self()->hideWindow( pHost, text );
    pHost->mpConsole->hideWindow( text );

    return 0;
}

int TLuaInterpreter::setBorderTop( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderTopHeight = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::setBorderBottom( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderBottomHeight = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::setBorderLeft( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderLeftWidth = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::setBorderRight( lua_State *L )
{
    int x1;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mBorderRightWidth = x1;
    int x,y;
    x = pHost->mpConsole->width();
    y = pHost->mpConsole->height();
    QSize s = QSize(x,y);
    QResizeEvent event(s, s);
    QApplication::sendEvent( pHost->mpConsole, &event);
    return 0;
}

int TLuaInterpreter::resizeUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double x1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 2 );
    }
    double y1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y1 = lua_tonumber( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->resizeWindow( pHost, text, static_cast<int>(x1), static_cast<int>(y1) );

    return 0;
}

int TLuaInterpreter::moveWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double x1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 2 );
    }
    double y1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y1 = lua_tonumber( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];

    QString text(luaSendText.c_str());
    mudlet::self()->moveWindow( text, static_cast<int>(x1), static_cast<int>(y1) );
    return 0;
}


int TLuaInterpreter::setBackgroundColor( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double x1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x1 = lua_tonumber( L, 2 );
    }
    double y1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y1 = lua_tonumber( L, 3 );
    }
    double x2;
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x2 = lua_tonumber( L, 4 );
    }
    double y2;
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        y2 = lua_tonumber( L, 5 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    mudlet::self()->setBackgroundColor( text, static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2) );

    return 0;
}

int TLuaInterpreter::calcFontWidth( int size )
{
    QFont font = QFont("Bitstream Vera Sans Mono", size, QFont::Courier);
    return QFontMetrics( font ).width( QChar('W') );
}

int TLuaInterpreter::calcFontHeight( int size )
{
    QFont font = QFont("Bitstream Vera Sans Mono", size, QFont::Courier);
    int fontDescent = QFontMetrics( font ).descent();
    int fontAscent = QFontMetrics( font ).ascent();
    return fontAscent + fontDescent;
}

int TLuaInterpreter::calcFontSize( lua_State *L )
{
    int x = 0;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        x = lua_tonumber( L, 1 );
    }

    lua_pushnumber( L, calcFontWidth( x ) );
    lua_pushnumber( L, calcFontHeight( x ) );
    return 2;
}

int TLuaInterpreter::startLogging( lua_State *L )
{
    bool logOn = true;
    if( ! lua_isboolean( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        logOn = lua_toboolean( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mpConsole->mLogToLogFile = ! logOn;
    pHost->mpConsole->slot_toggleLogging();
    return 0;
}

int TLuaInterpreter::setBackgroundImage( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    string luaName="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    QString name(luaName.c_str());
    mudlet::self()->setBackgroundImage( text, name );

    return 0;
}

int TLuaInterpreter::setLabelClickCallback( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    string luaName="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 2 );
    }

    TEvent * pE = new TEvent;

    int n = lua_gettop( L );
    for( int i=3; i<=n; i++)
    {
        if( lua_isnumber( L, i ) )
        {
            pE->mArgumentList.append( QString::number(lua_tonumber( L, i ) ) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_NUMBER );
        }
        else if( lua_isstring( L, i ) )
        {
            pE->mArgumentList.append( QString(lua_tostring( L, i )) );
            pE->mArgumentTypeList.append( ARGUMENT_TYPE_STRING );
        }
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    QString name(luaName.c_str());
    mudlet::self()->setLabelClickCallback( pHost, text, name, pE );

    return 0;
}

int TLuaInterpreter::setTextFormat( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    double r1;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r1 = lua_tonumber( L, 2 );
    }
    double g1;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        g1 = lua_tonumber( L, 3 );
    }
    double b1;
    if( ! lua_isnumber( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b1 = lua_tonumber( L, 4 );
    }
    double r2;
    if( ! lua_isnumber( L, 5 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        r2 = lua_tonumber( L, 5 );
    }
    double g2;
    if( ! lua_isnumber( L, 6 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        g2 = lua_tonumber( L, 6 );
    }
    double b2;
    if( ! lua_isnumber( L, 7 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b2 = lua_tonumber( L, 7 );
    }
    double bold;
    if( ! lua_isnumber( L, 8 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        bold = lua_tonumber( L, 8 );
    }
    double underline;
    if( ! lua_isnumber( L, 9 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        underline = lua_tonumber( L, 9 );
    }
    double italics;
    if( ! lua_isnumber( L, 10 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        italics = lua_tonumber( L, 10 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    if( text == "main" )
    {
        TConsole * pC = pHost->mpConsole;
        pC->mFormatCurrent.bgR = r1;
        pC->mFormatCurrent.bgG = g1;
        pC->mFormatCurrent.bgB = b1;
        pC->mFormatCurrent.fgR = r2;
        pC->mFormatCurrent.fgG = g2;
        pC->mFormatCurrent.fgB = b2;
        pC->mFormatCurrent.bold = bold;
        pC->mFormatCurrent.underline = underline;
        pC->mFormatCurrent.italics = italics;
        return true;
    }
    else
    {
        mudlet::self()->setTextFormat( text, static_cast<int>(r1), static_cast<int>(g1), static_cast<int>(b1), static_cast<int>(r2),static_cast<int>(g2), static_cast<int>(b2), static_cast<bool>(bold), static_cast<bool>(underline), static_cast<bool>(italics) );
    }

    return 0;
}

int TLuaInterpreter::showUserWindow( lua_State *L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString text(luaSendText.c_str());
    lua_pushboolean( L, pHost->mpConsole->showWindow( text ));
    return 1;
}

int TLuaInterpreter::reset( lua_State *L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString name = luaWindowName.c_str();
    if( luaWindowName.size() < 1 || luaWindowName == "main" )
    {
        pHost->mpConsole->reset();
    }
    else
    {
        mudlet::self()->resetFormat( name );
    }
    return 0;
}

int TLuaInterpreter::echoUserWindow( lua_State *L )
{
    string luaWindowName="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaWindowName = lua_tostring( L, 1 );
    }
    
    string luaSendText="";
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString text(luaSendText.c_str());
    QString windowName(luaWindowName.c_str());
    mudlet::self()->echoWindow( pHost, windowName, text );
    return 0;
}
int TLuaInterpreter::playSoundFile( lua_State * L )
{
    string luaSendText="";
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaSendText = lua_tostring( L, 1 );
    }
    QSound::play( QString( luaSendText.c_str() ) );
}

int TLuaInterpreter::moveCursorEnd( lua_State *L )
{
    string luaWindowName="";
    if( lua_isstring( L, 1 ) )
    {
        luaWindowName = lua_tostring( L, 1 );
    }
    else
        luaWindowName = "main";

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( luaWindowName == "main" )
        pHost->mpConsole->moveCursorEnd();
    else
       mudlet::self()->moveCursorEnd( windowName );
    return 0;
}

int TLuaInterpreter::getLastLineNumber( lua_State *L )
{
    string luaWindowName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    int number;
    if( luaWindowName == "main" )
        number = pHost->mpConsole->getLastLineNumber();
    else
        number = mudlet::self()->getLastLineNumber( windowName );
    lua_pushnumber( L, number );
    return 1;
}



int TLuaInterpreter::getMudletHomeDir( lua_State * L )
{
    QString home = QDir::homePath();
    home.append( "/.config/mudlet/profiles/" );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name = pHost->getName();
    home.append( name );
    QString erg = QDir::toNativeSeparators( home );
    lua_pushstring( L, erg.toLatin1().data() );
    return 1;
}

int TLuaInterpreter::disconnect( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.disconnect();
    return 0;
}

int TLuaInterpreter::reconnect( lua_State * L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    pHost->mTelnet.connectIt( pHost->getUrl(), pHost->getPort() );
    return 0;
}

int TLuaInterpreter::setTriggerStayOpen( lua_State *L )
{
    string luaWindowName;
    double b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_tonumber( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    pHost->getTriggerUnit()->setTriggerStayOpen( QString( luaWindowName.c_str() ), static_cast<int>(b) );
    return 0;
}

int TLuaInterpreter::setBold( lua_State *L )
{
    string luaWindowName;
    bool b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isboolean( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( windowName.size() > 0 )
        mudlet::self()->setBold( windowName, b );
    else
        pHost->mpConsole->setBold( b );
    return 0;
}

int TLuaInterpreter::setItalics( lua_State *L )
{
        string luaWindowName;
    bool b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isboolean( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( windowName.size() > 0 )
        mudlet::self()->setItalics( windowName, b );
    else
        pHost->mpConsole->setItalics( b );
    return 0;
}
int TLuaInterpreter::setUnderline( lua_State *L )
{
        string luaWindowName;
    bool b;
    int s = 1;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            luaWindowName = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isboolean( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        b = lua_toboolean( L, s );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    if( windowName.size() > 0 )
        mudlet::self()->setUnderline( windowName, b );
    else
        pHost->mpConsole->setUnderline( b );
    return 0;
}

int TLuaInterpreter::debug( lua_State *L )
{
    int nbargs = lua_gettop(L);
    QString luaDebugText="";
    for (int i=0; i<nbargs; i++)
    {
        luaDebugText += (nbargs > 1 ? " [" + QString::number(i) + "] " : " ") + lua_tostring( L, i+1 );
    }
    return 0;
}

int TLuaInterpreter::hideToolBar( lua_State *L )
{
    string luaWindowName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    pHost->getActionUnit()->hideToolBar( windowName );
    return 0;
}

int TLuaInterpreter::showToolBar( lua_State *L )
{
    string luaWindowName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaWindowName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString windowName(luaWindowName.c_str());
    pHost->getActionUnit()->showToolBar( windowName );
    return 0;
}

int TLuaInterpreter::getButtonState( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    int state;
    state = pHost->mpConsole->getButtonState();
    lua_pushnumber( L, state );
    return 1;
}




int TLuaInterpreter::getNetworkLatency( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    double number;
    number = pHost->mTelnet.networkLatency;
    lua_pushnumber( L, number );
    return 1;
}

int TLuaInterpreter::getMainWindowSize( lua_State *L )
{
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QSize size = pHost->mpConsole->mpMainFrame->size();
    lua_pushnumber( L, size.width() );
    lua_pushnumber( L, size.height()-pHost->mpConsole->mpCommandLine->height() );
    return 2;
}

// tempTimer(int session, float seconds, string function to call, string name) // one shot timer.
int TLuaInterpreter::tempTimer( lua_State *L )
{
    double luaTimeout;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTimeout = lua_tonumber( L, 1 );
    }      
    
    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempTimer( luaTimeout, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}

// tempTrigger( string regex, string function to call ) // one shot timer.
int TLuaInterpreter::tempTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaRegex = lua_tostring( L, 1 );
    }      
    
    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 2 );
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _reg = luaRegex.c_str();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempTrigger( _reg, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}


// temporary color trigger. args: ansiFGColorCode, ansiBgColorCode, luaCode
int TLuaInterpreter::tempColorTrigger( lua_State *L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFrom = lua_tointeger( L, 1 );
    }
    int luaTo;
    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTo = lua_tointeger( L, 2 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempColorTrigger( luaFrom, luaTo, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}


// triggerID = tempLineTrigger( from, howmany, func )
int TLuaInterpreter::tempLineTrigger( lua_State *L )
{
    int luaFrom;
    if( ! lua_isnumber( L, 1 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFrom = lua_tointeger( L, 1 );
    }      
    int luaTo;
    if( ! lua_isnumber( L, 2 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaTo = lua_tointeger( L, 2 );
    }      
    
    string luaFunction;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 3 );
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempLineTrigger( luaFrom, luaTo, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}


// tempTrigger( string regex, string function to call ) // one shot timer.
int TLuaInterpreter::tempRegexTrigger( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) ) 
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaRegex = lua_tostring( L, 1 );
    }      
    
    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaFunction = lua_tostring( L, 2 );
    }
    
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _luaFunction = luaFunction.c_str();
    QString _luaRegex = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempRegexTrigger( _luaRegex, _luaFunction );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::tempAlias( lua_State *L )
{
    string luaRegex;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 1 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 2 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _luaFunction = luaFunction.c_str();
    QString _luaRegex = luaFunction.c_str();
    int timerID = pLuaInterpreter->startTempAlias( _luaRegex, _luaFunction );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::exists( lua_State * L )
{
    string _name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _name = lua_tostring( L, 1 );
    }
    string _type;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _type = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    int cnt = 0;
    QString type = _type.c_str();
    type = type.toLower();
    QString name = _name.c_str();
    if( type == "timer")
    {
        QMap<QString, TTimer *>::const_iterator it1 = pHost->getTimerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTimerUnit()->mLookupTable.end() && it1.key() == name )
        {
            cnt++;
            it1++;
        }
    }
    else if( type == "trigger")
    {
        QMap<QString, TTrigger *>::const_iterator it1 = pHost->getTriggerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTriggerUnit()->mLookupTable.end() && it1.key() == name )
        {
            cnt++;
            it1++;
        }
    }
    else if( type == "alias")
    {
        QMap<QString, TAlias *>::const_iterator it1 = pHost->getAliasUnit()->mLookupTable.find( name );
        while( it1 != pHost->getAliasUnit()->mLookupTable.end() && it1.key() == name )
        {
            cnt++;
            it1++;
        }
    }
    lua_pushnumber( L, cnt );
    return 1;
}

int TLuaInterpreter::isActive( lua_State * L )
{
    string _name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _name = lua_tostring( L, 1 );
    }
    string _type;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        _type = lua_tostring( L, 2 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    int cnt = 0;
    QString type = _type.c_str();
    type = type.toLower();
    QString name = _name.c_str();
    if( type == "timer")
    {
        QMap<QString, TTimer *>::const_iterator it1 = pHost->getTimerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTimerUnit()->mLookupTable.end() && it1.key() == name )
        {
            if( it1.value()->isActive() )
            {
                cnt++;
            }
            it1++;
        }
    }
    else if( type == "trigger")
    {
        QMap<QString, TTrigger *>::const_iterator it1 = pHost->getTriggerUnit()->mLookupTable.find( name );
        while( it1 != pHost->getTriggerUnit()->mLookupTable.end() && it1.key() == name )
        {
            if( it1.value()->isActive() )
            {
                cnt++;
            }
            it1++;
        }
    }
    else if( type == "alias")
    {
        QMap<QString, TAlias *>::const_iterator it1 = pHost->getAliasUnit()->mLookupTable.find( name );
        while( it1 != pHost->getAliasUnit()->mLookupTable.end() && it1.key() == name )
        {
            if( it1.value()->isActive() )
            {
                cnt++;
            }
            it1++;
        }
    }
    lua_pushnumber( L, cnt );
    return 1;
}


int TLuaInterpreter::permAlias( lua_State *L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 1 );
    }

    string luaParent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaParent = lua_tostring( L, 2 );
    }

    string luaRegex;
    if( ! lua_isstring( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRegex = lua_tostring( L, 3 );
    }


    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _luaName = luaName.c_str();
    QString _luaParent = luaParent.c_str();
    QString _luaFunction = luaFunction.c_str();
    QString _luaRegex = luaRegex.c_str();
    int timerID = pLuaInterpreter->startPermAlias( _luaName, _luaParent, _luaRegex, _luaFunction );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::permTimer( lua_State * L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 1 );
    }
    string luaParent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaParent = lua_tostring( L, 2 );
    }

    double luaTimeout;
    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaTimeout = lua_tonumber( L, 3 );
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = luaName.c_str();
    QString _parent = luaParent.c_str();
    QString _fun = luaFunction.c_str();
    int timerID = pLuaInterpreter->startPermTimer( _name, _parent, luaTimeout, _fun );
    lua_pushnumber( L, timerID );
    return 1;
}

int TLuaInterpreter::permSubstringTrigger( lua_State * L )
{
    string name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }

    string parent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        parent = lua_tostring( L, 2 );
    }
    QStringList _regList;
    if( ! lua_istable( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, 3 ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString regex = lua_tostring( L, -1 );
                _regList << regex;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = name.c_str();
    QString _parent = parent.c_str();
    QString _luaFunction = luaFunction.c_str();
    int ret = pLuaInterpreter->startPermSubstringTrigger( _name,
                                                          _parent,
                                                          _regList,
                                                          _luaFunction );
    lua_pushnumber( L, ret );
    return 1;
}

int TLuaInterpreter::permBeginOfLineStringTrigger( lua_State * L )
{
    string name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }

    string parent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        parent = lua_tostring( L, 2 );
    }
    QStringList _regList;
    if( ! lua_istable( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, 3 ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString regex = lua_tostring( L, -1 );
                _regList << regex;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = name.c_str();
    QString _parent = parent.c_str();
    QString _luaFunction = luaFunction.c_str();
    int ret = pLuaInterpreter->startPermBeginOfLineStringTrigger( _name,
                                                                  _parent,
                                                                  _regList,
                                                                  _luaFunction );
    lua_pushnumber( L, ret );
    return 1;
}

int TLuaInterpreter::permRegexTrigger( lua_State *L )
{
    string name;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        name = lua_tostring( L, 1 );
    }

    string parent;
    if( ! lua_isstring( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        parent = lua_tostring( L, 2 );
    }
    QStringList _regList;
    if( ! lua_istable( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        lua_pushnil( L );
        while( lua_next( L, 3 ) != 0 )
        {
            // key at index -2 and value at index -1
            if( lua_type(L, -1) == LUA_TSTRING )
            {
                QString regex = lua_tostring( L, -1 );
                _regList << regex;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    string luaFunction;
    if( ! lua_isstring( L, 4 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaFunction = lua_tostring( L, 4 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    TLuaInterpreter * pLuaInterpreter = pHost->getLuaInterpreter();
    QString _name = name.c_str();
    QString _parent = parent.c_str();
    QString _luaFunction = luaFunction.c_str();
    int ret = pLuaInterpreter->startPermRegexTrigger( _name,
                                                      _parent,
                                                      _regList,
                                                      _luaFunction );
    lua_pushnumber( L, ret );
    return 1;
}

int TLuaInterpreter::setBorderColor( lua_State *L )
{
    int luaRed;
    int luaGreen;
    int luaBlue;
    if( ! lua_isnumber( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRed = lua_tointeger( L, 1 );
    }

    if( ! lua_isnumber( L, 2 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaGreen=lua_tointeger( L, 2 );
    }

    if( ! lua_isnumber( L, 3 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaBlue = lua_tointeger( L, 3 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QPalette framePalette;
    framePalette.setColor( QPalette::Text, QColor(0,0,0) );
    framePalette.setColor( QPalette::Highlight, QColor(55,55,255) );
    framePalette.setColor( QPalette::Window, QColor( luaRed, luaGreen, luaBlue, 255 ) );
    pHost->mpConsole->mpMainFrame->setPalette( framePalette );
    return 0;
}

int TLuaInterpreter::setFgColor( lua_State *L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    int luaRed;
    int luaGreen;
    int luaBlue;
    if( n > 3 )
    {
        if( lua_isstring( L, s ) )
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaRed = lua_tointeger( L, s );
        s++;
    }      
    
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaGreen=lua_tointeger( L, s );
        s++;
    }      
    
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaBlue = lua_tointeger( L, s );
    }

    QString _name( a1.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    if( n < 4 )
        pHost->mpConsole->setFgColor( luaRed, luaGreen, luaBlue );
    else
        mudlet::self()->setFgColor( _name, luaRed, luaGreen, luaBlue );
    return 0;
}

int TLuaInterpreter::setBgColor( lua_State *L )
{
    int s = 1;
    int n = lua_gettop( L );
    string a1;
    int luaRed;
    int luaGreen;
    int luaBlue;
    if( n > 3 )
    {
        if( lua_isstring( L, s ) )
        {
            a1 = lua_tostring( L, s );
            s++;
        }
    }
    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaRed = lua_tointeger( L, s );
        s++;
    }

    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaGreen=lua_tointeger( L, s );
        s++;
    }

    if( ! lua_isnumber( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaBlue = lua_tointeger( L, s );
    }

    QString _name( a1.c_str() );
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    if( n < 4 )
        pHost->mpConsole->setBgColor( luaRed, luaGreen, luaBlue );
    else
        mudlet::self()->setBgColor( _name, luaRed, luaGreen, luaBlue );
    return 0;
}


int TLuaInterpreter::insertText( lua_State *L )
{
    string a1;
    string a2;
    int n = lua_gettop( L );
    int s = 1;
    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        a1 = lua_tostring( L, s );
        s++;
    }
    QString _name( a1.c_str() );

    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a2 = lua_tostring( L, s );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    if( n == 1 )
        pHost->mpConsole->insertText( QString(a1.c_str()) );
    else
        mudlet::self()->insertText( _name, QString( a2.c_str() ) );
    return 0;
}

int TLuaInterpreter::insertHTML( lua_State *L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->mpConsole->insertHTML( QString(luaSendText.c_str()) );    
    return 0;
}

int TLuaInterpreter::Echo( lua_State *L )
{
    string a1;
    string a2;
    int s = 1;
    int n = lua_gettop( L );

    if( ! lua_isstring( L, s ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        a1 = lua_tostring( L, s );
        s++;
    }
    if( n > 1 )
    {
        if( ! lua_isstring( L, s ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            a2 = lua_tostring( L, s );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString txt;
    QString name;
    if( n == 1 )
    {
        txt = a1.c_str();
        pHost->mpConsole->echo( txt );
    }
    else
    {
        txt = a2.c_str();
        name = a1.c_str();
        mudlet::self()->echoWindow( pHost, name, txt );
    }

    return 0;
}

int TLuaInterpreter::pasteWindow( lua_State *L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaName = lua_tostring( L, 1 );
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    QString name( luaName.c_str());
    mudlet::self()->pasteWindow( pHost, name );    
    return 0;
}

int TLuaInterpreter::appendBuffer( lua_State *L )
{
    string luaName;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    {
        luaName = lua_tostring( L, 1 );
    }

    Host * pHost = TLuaInterpreter::luaInterpreterMap[L];
    QString name( luaName.c_str());
    mudlet::self()->appendBuffer( pHost, name );
    return 0;
}

int TLuaInterpreter::Send( lua_State * L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    bool wantPrint = true;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isboolean( L, 2 ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            wantPrint = lua_toboolean( L, 2 );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->send( QString( luaSendText.c_str() ), wantPrint, false );
    return 0;
}

int TLuaInterpreter::sendRaw( lua_State * L )
{
    string luaSendText;
    if( ! lua_isstring( L, 1 ) )
    {
        lua_pushstring( L, "wrong argument type" );
        lua_error( L );
        return 1;
    }
    else
    { 
        luaSendText = lua_tostring( L, 1 );
    }
    bool wantPrint = true;
    if( lua_gettop( L ) > 1 )
    {
        if( ! lua_isboolean( L, 2 ) )
        {
            lua_pushstring( L, "wrong argument type" );
            lua_error( L );
            return 1;
        }
        else
        {
            wantPrint = lua_toboolean( L, 2 );
        }
    }
    Host * pHost = TLuaInterpreter::luaInterpreterMap[L]; 
    pHost->send( QString(luaSendText.c_str()), wantPrint, true );
    return 0;
}


bool TLuaInterpreter::compileAndExecuteScript( QString & code )
{
    if( mudlet::debugMode )
    {
        qDebug("TLuaInterpreter: compiling following code:");
        qDebug("--------------------------------------------snip<");
        qDebug() << code;
        qDebug(">snip--------------------------------------------");
    }
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }
    
    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) ) 
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ) qDebug()<<"LUA ERROR: code did not compile: ERROR:"<<e.c_str();
        emit signalEchoMessage( mHostID, QString( e.c_str() ) );
    }
    
    lua_pop( L, lua_gettop( L ) );
    
    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::compileScript( QString & code )
{
    if( mudlet::debugMode )
    {
        qDebug("TLuaInterpreter: compiling following code:");
        qDebug("--------------------------------------------snip<");
        qDebug() << code;
        qDebug(">snip--------------------------------------------");
    }
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }
    
    /*lua_newtable( L );      
    
    // set values
    for( int i=0; i<matches.size(); i++ )
    {
        lua_pushnumber( L, i+1 ); // Lua indexes start with 1
        lua_pushstring( L, matches[i].toLatin1().data() );
        lua_settable( L, -3 );
    }
    lua_setglobal( L, "matches" );*/
    
    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) ) 
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: code did not compile: ERROR:"<<e.c_str()<<"\n">>0;}
    }
    else 
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA: code compiled without errors. OK\n">>0;}
    }
    lua_pop( L, lua_gettop( L ) );
    
    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::compile( QString & code )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }
    
    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) ) 
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: code did not compile: ERROR:"<<e.c_str()<<"\n">>0;}
    }
    else
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA: code compiled without errors. OK\n" >> 0;}
    }
    lua_pop( L, lua_gettop( L ) );
    
    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::compile( QString & code, QString & errorMsg )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    int error = luaL_dostring( L, code.toLatin1().data() );
    QString n;
    if( error != 0 )
    {
        string e = "Lua syntax error:";
        if( lua_isstring( L, 1 ) )
        {
            e.append( lua_tostring( L, 1 ) );
        }
        errorMsg = "<b><font color='blue'>";
        errorMsg.append( e.c_str() );
        errorMsg.append("</b></font>");
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"\n "<<e.c_str()<<"\n">>0;}
    }
    else
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"\nLUA: code compiled without errors. OK\n" >> 0;}
    }
    lua_pop( L, lua_gettop( L ) );

    if( error == 0 ) return true;
    else return false;
}

void TLuaInterpreter::setMultiCaptureGroups( const std::list< std::list<std::string> > & captureList,
                                             const std::list< std::list<int> > & posList )
{
    mMultiCaptureGroupList = captureList;
    mMultiCaptureGroupPosList = posList;

    /*std::list< std::list<string> >::const_iterator mit = mMultiCaptureGroupList.begin();

    int k=1;
    for( ; mit!=mMultiCaptureGroupList.end(); mit++, k++ )
    {
        cout << "regex#"<<k<<" got:"<<endl;
        std::list<string>::const_iterator it = (*mit).begin();
        for( int i=1; it!=(*mit).end(); it++, i++ )
        {
            cout << i<<"#"<<"<"<<*it<<">"<<endl;
        }
        cout << "-----------------------------"<<endl;
    }*/
}

void TLuaInterpreter::setCaptureGroups( const std::list<std::string> & captureList, const std::list<int> & posList )
{
    mCaptureGroupList = captureList;
    mCaptureGroupPosList = posList;
    
    /*std::list<string>::iterator it2 = mCaptureGroupList.begin();
    std::list<int>::iterator it1 = mCaptureGroupPosList.begin();
    int i=0;
    for( ; it1!=mCaptureGroupPosList.end(); it1++, it2++, i++ )
    {
        cout << "group#"<<i<<" begin="<<*it1<<" len="<<(*it2).size()<<"word="<<*it2<<endl;    
    } */
}

void TLuaInterpreter::clearCaptureGroups()
{
    mCaptureGroupList.clear();
    mCaptureGroupPosList.clear();
    mMultiCaptureGroupList.clear();
    mMultiCaptureGroupPosList.clear();
    
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
    }
    
    lua_newtable( L );      
    lua_setglobal( L, "matches" );
    lua_newtable( L );
    lua_setglobal( L, "multimatches" );

    lua_pop( L, lua_gettop( L ) );
}


void TLuaInterpreter::adjustCaptureGroups( int x, int a )
{
    // adjust all capture group positions in line if data has been inserted by the user
    typedef std::list<int>::iterator I;
    for( I it=mCaptureGroupPosList.begin(); it!=mCaptureGroupPosList.end(); it++ )
    {
        if( *it >= x )
        {
            *it += a;
        }
    }
}

bool TLuaInterpreter::call( QString & function, QString & mName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    if( mCaptureGroupList.size() > 0 )
    {
        lua_newtable( L );      
        
        // set values
        int i=1; // Lua indexes start with 1 as a general convention
        std::list<std::string>::iterator it = mCaptureGroupList.begin();
        for( ; it!=mCaptureGroupList.end(); it++, i++ )
        {
            //if( (*it).length() < 1 ) continue; //have empty capture groups to be undefined keys i.e. machts[emptyCapGroupNumber] = nil otherwise it's = "" i.e. an empty string
            lua_pushnumber( L, i );
            lua_pushstring( L, (*it).c_str() );
            lua_settable( L, -3 );
        }
        lua_setglobal( L, "matches" );
    }
    
    lua_getglobal( L, function.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, function.toLatin1().data() );
    int error = lua_pcall( L, 0, LUA_MULTRET, 0 );     
    if( error != 0 )
    {
        int nbpossible_errors = lua_gettop(L);
        for (int i=1; i<=nbpossible_errors; i++)
        {
            string e = "";
            if(lua_isstring( L, i) ) 
            {
                e = "Lua error:";
                e+=lua_tostring( L, i );

                if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running script "<< mName << " (" << function <<") ERROR:"<<e.c_str()>>0;}
            }
        }
    }
    else
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA OK script "<<mName << " (" << function <<") ran without errors\n">>0;}
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 ) return true;
    else return false;
}

bool TLuaInterpreter::callConditionFunction( std::string & function, QString & mName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    lua_getfield( L, LUA_GLOBALSINDEX, function.c_str() );
    int error = lua_pcall( L, 0, 1, 0 );
    if( error != 0 )
    {
        int nbpossible_errors = lua_gettop(L);
        for (int i=1; i<=nbpossible_errors; i++)
        {
            string e = "";
            if(lua_isstring( L, i) )
            {
                e = "Lua error:";
                e+=lua_tostring( L, i );

                if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running script "<< mName << " (" << function.c_str() <<") ERROR:"<<e.c_str()<<"\n">>0;}
            }
        }
    }
    else
    {
        if( mudlet::debugMode ){ TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA OK script "<<mName << " (" << function.c_str() <<") ran without errors\n">>0;}
    }

    int ret = 0;
    int returnValues = lua_gettop( L );
    if( returnValues > 0 )
    {
        if( lua_isboolean( L, 1 ) )
        {
            ret = lua_toboolean( L, 1 );
        }
    }
    lua_pop( L, returnValues );
    if( (error == 0) && (ret>0) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TLuaInterpreter::callMulti( QString & function, QString & mName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    if( mMultiCaptureGroupList.size() > 0 )
    {
        int k=1; // Lua indexes start with 1 as a general convention
        std::list< std::list<std::string> >::iterator mit = mMultiCaptureGroupList.begin();
        lua_newtable( L );//multimatches
        for( ; mit!=mMultiCaptureGroupList.end(); mit++, k++ )
        {
            // multimatches{ trigger_idx{ table_matches{ ... } } }
            lua_pushnumber( L, k );
            lua_newtable( L );//regex-value => table matches
            int i=1; // Lua indexes start with 1 as a general convention
            std::list<std::string>::iterator it = (*mit).begin();
            for( ; it!=(*mit).end(); it++, i++ )
            {
                lua_pushnumber( L, i );
                lua_pushstring( L, (*it).c_str() );
                lua_settable( L, -3 );//match in matches
            }
            lua_settable( L, -3 );//matches in regex
        }
        lua_setglobal( L, "multimatches" );
    }

    lua_getglobal( L, function.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, function.toLatin1().data() );
    int error = lua_pcall( L, 0, LUA_MULTRET, 0 );
    if( error != 0 )
    {
        int nbpossible_errors = lua_gettop(L);
        for (int i=1; i<=nbpossible_errors; i++)
        {
            string e = "";
            if(lua_isstring( L, i) )
            {
                e = "Lua error:";
                e+=lua_tostring( L, i );

                if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::red))<<"LUA: ERROR running script "<< mName << " (" << function <<") ERROR:"<<e.c_str()<<"\n">>0;}
            }
        }
    }
    else
    {
        if( mudlet::debugMode ) {TDebug(QColor(Qt::white),QColor(Qt::darkGreen))<<"LUA OK script "<<mName << " (" << function <<") ran without errors\n">>0;}
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 ) return true;
    else return false;
}


bool TLuaInterpreter::callEventHandler( QString & function, TEvent * pE )
{
    lua_State * L = pGlobalLua;
    lua_getglobal( L, function.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, function.toLatin1().data() );
    for( int i=0; i<pE->mArgumentList.size(); i++ )
    {
        if( pE->mArgumentTypeList[i] == ARGUMENT_TYPE_NUMBER )
        {
            lua_pushnumber( L, pE->mArgumentList[i].toInt() );
        }
        else
        {
            lua_pushstring( L, pE->mArgumentList[i].toLatin1().data() );
        }
    }
    int error = lua_pcall( L, pE->mArgumentList.size(), LUA_MULTRET, 0 );
    if( error != 0 )
    {
        string e = "";
        if(lua_isstring( L, 1) ) 
        {
            e = "Lua error:";
            e+=lua_tostring( L, 1 );
        }
        if( mudlet::debugMode ) qDebug()<<"LUA: ERROR running script "<< function <<" ERROR:"<<e.c_str();
    }
    lua_pop( L, lua_gettop( L ) );
    if( error == 0 ) return true;
    else return false;
}


void TLuaInterpreter::set_lua_table( QString & tableName, QStringList & variableList )
{
    
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return;
    }
    lua_newtable(L);      
    for( int i=0; i<variableList.size(); i++ )
    {
        lua_pushnumber( L, i+1 ); // Lua indexes start with 1
        lua_pushstring( L, variableList[i].toLatin1().data() );
        lua_settable( L, -3 );
    }
    lua_setglobal( L, tableName.toLatin1().data() );
    lua_pop( pGlobalLua, lua_gettop( pGlobalLua ) );
}

void TLuaInterpreter::set_lua_string( const QString & varName, QString & varValue )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return;
    }
    
    //lua_pushstring( L, varName.toLatin1().data() );
    lua_pushstring( L, varValue.toLatin1().data() );
    lua_setglobal( L, varName.toLatin1().data() );   
    //lua_setfield( L, LUA_GLOBALSINDEX, s )
    lua_pop( pGlobalLua, lua_gettop( pGlobalLua ) );
        
//lua_settable( L, LUA_GLOBALSINDEX );
}

QString TLuaInterpreter::get_lua_string( QString & stringName )
{
    lua_State * L = pGlobalLua;
    if( ! L )
    {
        qDebug()<< "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return QString( "LUA CRITICAL ERROR" );
    }
        
    lua_getglobal( L, stringName.toLatin1().data() );
    lua_getfield( L, LUA_GLOBALSINDEX, stringName.toLatin1().data() );
    return QString( lua_tostring( L, 1 ) );
}



void TLuaInterpreter::threadLuaInterpreterExec( string code )
{
    /* cout << "TLuaMainThread::threadLuaInterpreterExec(code) executing following code:" << endl;
     cout << "--------------------------------------------snip<" <<endl;
     cout << code << endl;
     cout << ">snip--------------------------------------------" <<endl;*/
     lua_State * L = pGlobalLua;
     int error = luaL_dostring(L,code.c_str());
     QString n;
     if( error != 0 )
     {
        string e = "no error message available from Lua";
        if( lua_isstring( L, 1 ) ) 
        {
            e = "Lua error:";
            e += lua_tostring( L, 1 );
        }
        emit signalEchoMessage( mHostID, QString( e.c_str() ) );
        qDebug()<< "LUA_ERROR:"<<e.c_str(); 
     }

     cout << "cRunningScript::threadLuaInterpreterExec() done" << endl;
}



void TLuaInterpreter::startLuaSessionInterpreter()
{
    //connect(this,SIGNAL(signalOpenWindow(int,QString)), this,SLOT(slotOpenWindow(int,QString)));
    //connect(this,SIGNAL(signalEchoWindow(int,QString,QString)), this,SLOT(slotEchoWindow(int,QString,QString)));
    //connect(this,SIGNAL(signalClearWindow(int,QString)), this,SLOT(slotClearWindow(int,QString))); 
    //connect(this,SIGNAL(signalNewTrigger(QString,QString, int, QString)), this,SLOT(slotNewTrigger(QString,QString, int, QString)));
    //connect(this,SIGNAL(signalAddTimer(int,int,QString,QString)),this,SLOT(slotAddTimer(int,int,QString,QString)));
    //connect(this,SIGNAL(signalDeleteTrigger(int,QString)), this,SLOT(slotDeleteTrigger(int,QString)));
    
    
    //connect(this,SIGNAL(signalEchoMessage(int,QString)), this,SLOT(slotEchoMessage(int,QString)));//,Qt::DirectConnection);
    //connect(this,SIGNAL(signalNewEcho(int,QString)), this,SLOT(slotNewEcho(int,QString)));
    //connect(this,SIGNAL(signalNewCommand(int,QString)), this,SLOT(slotNewCommand(int,QString)));//,Qt::QueuedConnection);
    
    mpLuaSessionThread = new TLuaMainThread(this);
    mpLuaSessionThread->start(); //calls initLuaGlobals() to initialize the interpreter for this session
}

// this function initializes the Lua Session interpreter. 
// on initialization of a new session *or* in case of an interpreter reset by the user.
void TLuaInterpreter::initLuaGlobals()
{
    pGlobalLua = lua_open();
    TLuaInterpreter::luaInterpreterMap[pGlobalLua]=mpHost;
    
    luaL_openlibs( pGlobalLua );
  
    lua_pushstring( pGlobalLua, "SESSION" );
    lua_pushnumber( pGlobalLua, mHostID );
    lua_settable( pGlobalLua, LUA_GLOBALSINDEX );
 
    lua_pushstring( pGlobalLua, "SCRIPT_NAME" );
    lua_pushstring( pGlobalLua, "Global Lua Session Interpreter" );
    lua_settable( pGlobalLua, LUA_GLOBALSINDEX );

    lua_pushstring( pGlobalLua, "SCRIPT_ID" );
    lua_pushnumber( pGlobalLua, -1 ); // ID 1 is used to indicate that this is the global Lua interpreter
    lua_settable( pGlobalLua, LUA_GLOBALSINDEX );

    lua_register( pGlobalLua, "wait", TLuaInterpreter::Wait );
    lua_register( pGlobalLua, "expandAlias", TLuaInterpreter::Send );
    lua_register( pGlobalLua, "echo", TLuaInterpreter::Echo );
    lua_register( pGlobalLua, "selectString", TLuaInterpreter::select );
    lua_register( pGlobalLua, "selectSection", TLuaInterpreter::selectSection );
    lua_register( pGlobalLua, "replace", TLuaInterpreter::replace );
    lua_register( pGlobalLua, "setBgColor", TLuaInterpreter::setBgColor );
    lua_register( pGlobalLua, "setFgColor", TLuaInterpreter::setFgColor );
    lua_register( pGlobalLua, "tempTimer", TLuaInterpreter::tempTimer );
    lua_register( pGlobalLua, "tempTrigger", TLuaInterpreter::tempTrigger );
    lua_register( pGlobalLua, "tempRegexTrigger", TLuaInterpreter::tempRegexTrigger );
    lua_register( pGlobalLua, "openUserWindow", TLuaInterpreter::openUserWindow );
    lua_register( pGlobalLua, "echoUserWindow", TLuaInterpreter::echoUserWindow );
    lua_register( pGlobalLua, "enableTimer", TLuaInterpreter::enableTimer );
    lua_register( pGlobalLua, "disableTimer", TLuaInterpreter::disableTimer );
    lua_register( pGlobalLua, "enableKey", TLuaInterpreter::enableKey );
    lua_register( pGlobalLua, "disableKey", TLuaInterpreter::disableKey );
    lua_register( pGlobalLua, "clearUserWindow", TLuaInterpreter::clearUserWindow );
    lua_register( pGlobalLua, "clearWindow", TLuaInterpreter::clearUserWindow );
    lua_register( pGlobalLua, "killTimer", TLuaInterpreter::killTimer );
    lua_register( pGlobalLua, "moveCursor", TLuaInterpreter::moveCursor );
    lua_register( pGlobalLua, "getLines", TLuaInterpreter::getLines );
    lua_register( pGlobalLua, "getLineNumber", TLuaInterpreter::getLineNumber );
    lua_register( pGlobalLua, "insertHTML", TLuaInterpreter::insertHTML );
    lua_register( pGlobalLua, "insertText", TLuaInterpreter::insertText );
    lua_register( pGlobalLua, "enableTrigger", TLuaInterpreter::enableTrigger );
    lua_register( pGlobalLua, "disableTrigger", TLuaInterpreter::disableTrigger );
    lua_register( pGlobalLua, "killTrigger", TLuaInterpreter::killTrigger );
    lua_register( pGlobalLua, "getLineCount", TLuaInterpreter::getLineCount );
    lua_register( pGlobalLua, "getColumnNumber", TLuaInterpreter::getColumnNumber );
    //lua_register( pGlobalLua, "getBufferTable", TLuaInterpreter::getBufferTable );
    //lua_register( pGlobalLua, "getBufferLine", TLuaInterpreter::getBufferLine );
    lua_register( pGlobalLua, "send", TLuaInterpreter::sendRaw );
    lua_register( pGlobalLua, "selectCaptureGroup", TLuaInterpreter::selectCaptureGroup );
    lua_register( pGlobalLua, "tempLineTrigger", TLuaInterpreter::tempLineTrigger );
    lua_register( pGlobalLua, "raiseEvent", TLuaInterpreter::raiseEvent );
    lua_register( pGlobalLua, "deleteLine", TLuaInterpreter::deleteLine );
    lua_register( pGlobalLua, "copy", TLuaInterpreter::copy );
    lua_register( pGlobalLua, "cut", TLuaInterpreter::cut );
    lua_register( pGlobalLua, "paste", TLuaInterpreter::paste );
    lua_register( pGlobalLua, "pasteWindow", TLuaInterpreter::pasteWindow );
    //lua_register( pGlobalLua, "userWindowLineWrap", TLuaInterpreter::userWindowLineWrap );
    lua_register( pGlobalLua, "debug", TLuaInterpreter::debug );
    lua_register( pGlobalLua, "setWindowWrap", TLuaInterpreter::setWindowWrap );
    lua_register( pGlobalLua, "setWindowWrapIndent", TLuaInterpreter::setWindowWrapIndent );
    lua_register( pGlobalLua, "resetFormat", TLuaInterpreter::reset );
    lua_register( pGlobalLua, "moveCursorEnd", TLuaInterpreter::moveCursorEnd );
    lua_register( pGlobalLua, "getLastLineNumber", TLuaInterpreter::getLastLineNumber );
    lua_register( pGlobalLua, "getNetworkLatency", TLuaInterpreter::getNetworkLatency );
    lua_register( pGlobalLua, "createMiniConsole", TLuaInterpreter::createMiniConsole );
    lua_register( pGlobalLua, "createLabel", TLuaInterpreter::createLabel );
    lua_register( pGlobalLua, "hideWindow", TLuaInterpreter::hideUserWindow );
    lua_register( pGlobalLua, "showWindow", TLuaInterpreter::showUserWindow );
    lua_register( pGlobalLua, "createBuffer", TLuaInterpreter::createBuffer );
    lua_register( pGlobalLua, "createStopWatch", TLuaInterpreter::createStopWatch );
    lua_register( pGlobalLua, "getStopWatchTime", TLuaInterpreter::getStopWatchTime );
    lua_register( pGlobalLua, "stopStopWatch", TLuaInterpreter::stopStopWatch );
    lua_register( pGlobalLua, "startStopWatch", TLuaInterpreter::startStopWatch );
    lua_register( pGlobalLua, "resetStopWatch", TLuaInterpreter::resetStopWatch );
    lua_register( pGlobalLua, "closeUserWindow", TLuaInterpreter::closeUserWindow );
    lua_register( pGlobalLua, "resizeWindow", TLuaInterpreter::resizeUserWindow );
    lua_register( pGlobalLua, "getNetworkLatency", TLuaInterpreter::getNetworkLatency );
    lua_register( pGlobalLua, "appendBuffer", TLuaInterpreter::appendBuffer );
    lua_register( pGlobalLua, "debug", TLuaInterpreter::debug );
    lua_register( pGlobalLua, "setBackgroundImage", TLuaInterpreter::setBackgroundImage );
    lua_register( pGlobalLua, "setBackgroundColor", TLuaInterpreter::setBackgroundColor );
    lua_register( pGlobalLua, "createButton", TLuaInterpreter::createButton );
    lua_register( pGlobalLua, "setLabelClickCallback", TLuaInterpreter::setLabelClickCallback );
    lua_register( pGlobalLua, "moveWindow", TLuaInterpreter::moveWindow );
    lua_register( pGlobalLua, "setTextFormat", TLuaInterpreter::setTextFormat );
    lua_register( pGlobalLua, "getMainWindowSize", TLuaInterpreter::getMainWindowSize );
    lua_register( pGlobalLua, "getCurrentLine", TLuaInterpreter::getCurrentLine );
    lua_register( pGlobalLua, "setMiniConsoleFontSize", TLuaInterpreter::setMiniConsoleFontSize );
    lua_register( pGlobalLua, "selectCurrentLine", TLuaInterpreter::selectCurrentLine );
    lua_register( pGlobalLua, "spawn", TLuaInterpreter::spawn );
    lua_register( pGlobalLua, "getButtonState", TLuaInterpreter::getButtonState );
    lua_register( pGlobalLua, "showToolBar", TLuaInterpreter::showToolBar );
    lua_register( pGlobalLua, "hideToolBar", TLuaInterpreter::hideToolBar );
    lua_register( pGlobalLua, "loadRawFile", TLuaInterpreter::loadRawFile );
    lua_register( pGlobalLua, "setBold", TLuaInterpreter::setBold );
    lua_register( pGlobalLua, "setItalics", TLuaInterpreter::setItalics );
    lua_register( pGlobalLua, "setUnderline", TLuaInterpreter::setUnderline );
    lua_register( pGlobalLua, "disconnect", TLuaInterpreter::disconnect );
    lua_register( pGlobalLua, "reconnect", TLuaInterpreter::reconnect );
    lua_register( pGlobalLua, "getMudletHomeDir", TLuaInterpreter::getMudletHomeDir );
    lua_register( pGlobalLua, "setTriggerStayOpen", TLuaInterpreter::setTriggerStayOpen );
    lua_register( pGlobalLua, "wrapLine", TLuaInterpreter::wrapLine );
    lua_register( pGlobalLua, "getFgColor", TLuaInterpreter::getFgColor );
    lua_register( pGlobalLua, "getBgColor", TLuaInterpreter::getBgColor );
    lua_register( pGlobalLua, "tempColorTrigger", TLuaInterpreter::tempColorTrigger );
    lua_register( pGlobalLua, "isAnsiFgColor", TLuaInterpreter::isAnsiFgColor );
    lua_register( pGlobalLua, "isAnsiBgColor", TLuaInterpreter::isAnsiBgColor );
    lua_register( pGlobalLua, "playSoundFile", TLuaInterpreter::playSoundFile );
    lua_register( pGlobalLua, "setBorderTop", TLuaInterpreter::setBorderTop );
    lua_register( pGlobalLua, "setBorderBottom", TLuaInterpreter::setBorderBottom );
    lua_register( pGlobalLua, "setBorderLeft", TLuaInterpreter::setBorderLeft );
    lua_register( pGlobalLua, "setBorderRight", TLuaInterpreter::setBorderRight );
    lua_register( pGlobalLua, "setBorderColor", TLuaInterpreter::setBorderColor );
    lua_register( pGlobalLua, "setConsoleBufferSize", TLuaInterpreter::setConsoleBufferSize );
    lua_register( pGlobalLua, "startLogging", TLuaInterpreter::startLogging );
    lua_register( pGlobalLua, "calcFontSize", TLuaInterpreter::calcFontSize );
    lua_register( pGlobalLua, "permRegexTrigger", TLuaInterpreter::permRegexTrigger );
    lua_register( pGlobalLua, "permSubstringTrigger", TLuaInterpreter::permSubstringTrigger );
    lua_register( pGlobalLua, "permTimer", TLuaInterpreter::permTimer );
    lua_register( pGlobalLua, "permAlias", TLuaInterpreter::permAlias );
    lua_register( pGlobalLua, "exists", TLuaInterpreter::exists );
    lua_register( pGlobalLua, "isActive", TLuaInterpreter::isActive );
    lua_register( pGlobalLua, "enableAlias", TLuaInterpreter::enableAlias );
    lua_register( pGlobalLua, "permBeginOfLineStringTrigger", TLuaInterpreter::permBeginOfLineStringTrigger );
    lua_register( pGlobalLua, "tempAlias", TLuaInterpreter::tempAlias );
    lua_register( pGlobalLua, "disableAlias", TLuaInterpreter::disableAlias );
    lua_register( pGlobalLua, "killAlias", TLuaInterpreter::killAlias );


    QString n;
    int error = luaL_dostring( pGlobalLua, "require \"rex_pcre\"" );

    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) )
        {
            e = "Lua error:";
            e+=lua_tostring( pGlobalLua, 1 );
        }
        //QString msg = "[FAILED] cannot find Lua module rex_pcre. Some functions may not be available.";
        QString msg = e.c_str();
        gSysErrors << msg;
    }
    else
    {
        QString msg = "[INFO] found Lua module rex_pcre";
        gSysErrors << msg;
    }
    QString path = QDir::homePath()+"/.config/mudlet/LuaGlobal.lua";
    error = luaL_dofile( pGlobalLua, path.toLatin1().data() );
    if( error != 0 )
    {
        string e = "no error message available from Lua";
        if( lua_isstring( pGlobalLua, 1 ) ) 
        {
            e = "[CRITICAL ERROR] LuaGlobal.lua compile error - please report";
            e += lua_tostring( pGlobalLua, 1 );
        }
        gSysErrors << e.c_str();
    }
    else
    {
        gSysErrors << "[INFO] LuaGlobal.lua loaded successfully.";
    }

    lua_pop( pGlobalLua, lua_gettop( pGlobalLua ) );
    
    //FIXME make function call in destructor lua_close(L);
}

void TLuaInterpreter::slotEchoMessage(int hostID, QString msg)
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    mudlet::self()->print( pHost, msg );
}


void TLuaInterpreter::slotNewCommand(int hostID, QString cmd)
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    pHost->send( cmd );
}

void TLuaInterpreter::slotOpenUserWindow(int hostID, QString windowName )
{
}

void TLuaInterpreter::slotClearUserWindow(int hostID, QString windowName )
{
}

void TLuaInterpreter::slotEnableTimer(int hostID, QString windowName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    pHost->enableTimer( windowName );
}

void TLuaInterpreter::slotDisableTimer(int hostID, QString windowName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    pHost->disableTimer( windowName );
}

void TLuaInterpreter::slotReplace(int hostID, QString text)
{
}

void TLuaInterpreter::slotEchoUserWindow(int hostID, QString windowName, QString text )
{
}

void TLuaInterpreter::slotTempTimer( int hostID, double timeout, QString function, QString timerName )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );
    QTime time(0,0,0,0);
    int msec = static_cast<int>(timeout * 1000);
    QTime time2 = time.addMSecs( msec );
    TTimer * pT;
    pT = new TTimer( timerName, time2, pHost );
    pT->setName( timerName );
    pT->setTime( time2 );
    //qDebug()<<"setting time of tempTimer to "<<time2.minute()<<":"<<time2.second()<<":"<<time2.msec()<<" timeout="<<timeout;
    pT->setScript( function );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTimer( true );
    pT->registerTimer();    
}

int TLuaInterpreter::startPermTimer( QString & name, QString & parent, double timeout, QString & function )
{
    QTime time( 0, 0, 0, 0 );
    int msec = static_cast<int>(timeout * 1000);
    QTime time2 = time.addMSecs( msec );
    TTimer * pT;
    if( parent.isEmpty() )
    {
        pT = new TTimer( "a", time2, mpHost );
    }
    else
    {
        TTimer * pP = mpHost->getTimerUnit()->findTimer( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTimer( pP, mpHost );
    }

    pT->setTime( time2 );
    pT->setIsFolder( false );
    pT->setIsTempTimer( false );
    pT->registerTimer();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( name );//darf erst nach isTempTimer gesetzt werde, damit setName() schneller ist
    pT->setIsActive( false );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return id;
}

int TLuaInterpreter::startTempTimer( double timeout, QString & function )
{
    QTime time( 0, 0, 0, 0 );
    int msec = static_cast<int>(timeout * 1000);
    QTime time2 = time.addMSecs( msec );
    TTimer * pT;
    pT = new TTimer( "a", time2, mpHost );
    pT->setTime( time2 );
    pT->setIsFolder( false );
    pT->setIsTempTimer( true );
    pT->registerTimer();    
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );//darf erst nach isTempTimer gesetzt werde, damit setName() schneller ist
    pT->setIsActive( true );
    pT->enableTimer( id );
    return id;                  
}

int TLuaInterpreter::startPermAlias( QString & name, QString & parent, QString & regex, QString & function )
{
    TAlias * pT;

    if( parent.isEmpty() )
    {
        pT = new TAlias("a", mpHost );
    }
    else
    {
        TAlias * pP = mpHost->getAliasUnit()->findAlias( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TAlias( pP, mpHost );
        pT->setRegexCode( regex );
    }
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempAlias( false );
    pT->registerAlias();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( name );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return id;
}

int TLuaInterpreter::startTempAlias( QString & regex, QString & function )
{
    TAlias * pT;
    pT = new TAlias("a", mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempAlias( true );
    pT->registerAlias();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startTempTrigger( QString & regex, QString & function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;
    QList<int> propertyList;
    propertyList << REGEX_SUBSTRING;// substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();    
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;                  
}

int TLuaInterpreter::startTempLineTrigger( int from, int howmany, QString & function )
{
    TTrigger * pT;
//    QStringList sList;
//    QList<int> propertyList;
//    propertyList << REGEX_SUBSTRING;// substring trigger is default
//    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT = new TTrigger( 0, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->setIsLineTrigger( true );
    pT->setStartOfLineDelta( from );
    pT->setLineDelta( howmany );
    pT->registerTrigger();    
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;                  
}

int TLuaInterpreter::startTempColorTrigger( int fg, int bg, QString & function )
{
    TTrigger * pT;
//    QStringList sList;
//    QList<int> propertyList;
//    propertyList << REGEX_SUBSTRING;// substring trigger is default
//    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT = new TTrigger( 0, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->setupTmpColorTrigger( fg, bg );

    pT->registerTrigger();
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;
}

int TLuaInterpreter::startTempRegexTrigger( QString & regex, QString & function )
{
    TTrigger * pT;
    QStringList sList;
    sList<<regex;
    
    QList<int> propertyList;
    propertyList << REGEX_PERL;// substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT->setIsFolder( false );
    pT->setIsActive( true );
    pT->setIsTempTrigger( true );
    pT->registerTrigger();    
    pT->setScript( function );
    int id = pT->getID();
    pT->setName( QString::number( id ) );
    return id;                  
}

int TLuaInterpreter::startPermRegexTrigger( QString & name, QString & parent, QStringList & regexList, QString & function )
{
    TTrigger * pT;
    QList<int> propertyList;
    for( int i=0; i<regexList.size(); i++ )
    {
        propertyList << REGEX_PERL;
    }
    if( parent.isEmpty() )
    {
        pT = new TTrigger( "a", regexList, propertyList, (regexList.size()>1), mpHost );
    }
    else
    {
        TTrigger * pP = mpHost->getTriggerUnit()->findTrigger( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTrigger( pP, mpHost );
        pT->setRegexCodeList( regexList, propertyList );
    }
    pT->setIsFolder( (regexList.size()==0) );
    pT->setIsActive( true );
    pT->setIsTempTrigger( false );
    pT->registerTrigger();
    pT->setScript( function );
    pT->setName( name );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return 1;

}

int TLuaInterpreter::startPermBeginOfLineStringTrigger( QString & name, QString & parent, QStringList & regexList, QString & function )
{
    TTrigger * pT;
    QList<int> propertyList;
    for( int i=0; i<regexList.size(); i++ )
    {
        propertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
    }
    if( parent.isEmpty() )
    {
        pT = new TTrigger( "a", regexList, propertyList, (regexList.size()>1), mpHost );
    }
    else
    {
        TTrigger * pP = mpHost->getTriggerUnit()->findTrigger( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTrigger( pP, mpHost );
        pT->setRegexCodeList( regexList, propertyList );
    }
    pT->setIsFolder( (regexList.size()==0) );
    pT->setIsActive( true );
    pT->setIsTempTrigger( false );
    pT->registerTrigger();
    pT->setScript( function );
    pT->setName( name );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return 1;

}

int TLuaInterpreter::startPermSubstringTrigger( QString & name, QString & parent, QStringList & regexList, QString & function )
{
    TTrigger * pT;
    QList<int> propertyList;
    for( int i=0; i<regexList.size(); i++ )
    {
        propertyList << REGEX_SUBSTRING;
    }
    if( parent.isEmpty() )
    {
        pT = new TTrigger( "a", regexList, propertyList, (regexList.size()>1), mpHost );
    }
    else
    {
        TTrigger * pP = mpHost->getTriggerUnit()->findTrigger( parent );
        if( !pP )
        {
            return -1;//parent not found
        }
        pT = new TTrigger( pP, mpHost );
        pT->setRegexCodeList( regexList, propertyList );
    }
    pT->setIsFolder( (regexList.size()==0) );
    pT->setIsActive( true );
    pT->setIsTempTrigger( false );
    pT->registerTrigger();
    pT->setScript( function );
    pT->setName( name );
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return 1;

}

void TLuaInterpreter::slotSelect( int hostID, QString text, int numOfMatch )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    
}
void TLuaInterpreter::slotSelectSection(int hostID, int from, int to )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    
}

void TLuaInterpreter::slotSetFgColor(int hostID, int r, int g, int b )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    
}

void TLuaInterpreter::slotSetBgColor(int hostID, int r, int g, int b )
{
    Host * pHost = HostManager::self()->getHostFromHostID( hostID );  
    pHost->mpConsole->setBgColor( r, g, b );
}

void TLuaInterpreter::parseATCP( QString & atcpString )
{
    lua_State * L = pGlobalLua;
    qDebug()<<atcpString;
    QStringList atcpArgs;
    //QStringList tableKeys = (QStringList() << "vitals" << "exits" << "roomname" << "charname" << "chartitle");
    char* type;
    QString arg2;
    if (atcpString.startsWith("Char.Vitals"))
    {
        type = "vitals"; // 1 for Vitals
        //QString tempString = atcpString;
        //qDebug()<<"Test: "<<tempString.remove(0,12);
        arg2 = atcpString.remove(0,12);
    } 
    else if (atcpString.startsWith("Room.Exits"))
    {
        type = "exits"; // 2 for exits
        arg2 = atcpString.remove(0,11);
    }
    else if (atcpString.startsWith("Room.Brief"))
    {
        type = "roomname";
        arg2 = atcpString.remove(0,11);
    }
    else if (atcpString.startsWith("Char.Name"))
    {
        type = "charname";
        arg2 = atcpString.remove(0,10);
    }
    else
    {
        qDebug()<<"Unanticipated ATCP: "<<atcpString;
    }
    //else if (atcpString.startsWith(
    //QString arg1 = atcpArgs[0];
    //char* arg2 = atcpArgs[1].toLatin1().data(); 
    
    if ((type !="charname")) 
    {
        loadATCPTable(type, arg2.toLatin1().data());
    } 
    else if (type == "charname")
    {
        //qDebug()<<"Got a character name";
        QStringList multiLine = arg2.split('\n');
        loadATCPTable("charname",multiLine[0].toLatin1().data());
        loadATCPTable("chartitle",multiLine[1].toLatin1().data());
    }
}

void TLuaInterpreter::loadATCPTable( char* type, char* args )
{
    lua_State * L = pGlobalLua;
    lua_getglobal( L, "atcp" );
    //qDebug()<<"Args: "<<args;
    //Load table onto stack
    lua_pushstring( L, type ); //Key
    lua_pushstring( L, args ); // Value
    lua_rawset( L, -3 ); // -3 = atcp, -2 = key, -1 = value.
    lua_pop(L,1);//Get rid of the table.
    lua_getglobal( L, "handleAtcpEvent" );
    lua_pushstring( L, type );
    lua_pushstring( L, args );
    lua_call( L, 2, 0);
}