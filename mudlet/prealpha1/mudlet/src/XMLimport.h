/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn  ( KoehnHeiko@googlemail.com )      *
 *                                                                         *                                                                         *
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


#ifndef XMLIMPORT_H
#define XMLIMPORT_H

#include <QXmlStreamReader>
#include "Host.h"

class Host;
class TTrigger;
class TTimer;
class TAlias;
class TAction;
class TScript;
class TKey;

class XMLimport : public QXmlStreamReader
{

public:
              XMLimport( Host * );
    
    bool      importPackage( QIODevice *device );
    
private:
    
    void      readPackage();
    void      readUnknownPackage();
    
    void      readHostPackage();
    void      readTriggerPackage();
    void      readTimerPackage();
    void      readAliasPackage();
    void      readActionPackage();
    void      readScriptPackage();
    void      readKeyPackage();
    
    void      readUnknownHostElement();
    void      readUnknownTriggerElement();
    void      readUnknownTimerElement();
    void      readUnknownAliasElement();
    void      readUnknownActionElement();
    void      readUnknownScriptElement();
    void      readUnknownKeyElement();
    
    void      readHostPackage( Host * );
    void      readTriggerGroup( TTrigger * pParent );
    void      readTimerGroup( TTimer * pParent );
    void      readAliasGroup( TAlias * pParent );
    void      readActionGroup( TAction * pParent );
    void      readScriptGroup( TScript * pParent );
    void      readKeyGroup( TKey * pParent );
    
    
    void      readStringList( QStringList & );
    void      readIntegerList( QList<int> & );

    Host *    mpHost;
};

#endif