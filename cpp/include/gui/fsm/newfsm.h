/***********************************************************************
*                                                                      *
* (C) 2007, Frank Lemke, Computer Architecture Group,                  *
* University of Mannheim, Germany                                      *
*                                                                      *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or    *
* (at your option) any later version.                                  *
*                                                                      *
* This program is distributed in the hope that it will be useful,      *
* but WITHOUT ANY WARRANTY; without even the implied warranty of       *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU General Public License for more details.                         *
*                                                                      *
* You should have received a copy of the GNU General Public License    *
* along with this program; if not, write to the Free Software          *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 *
* USA                                                                  *
*                                                                      *
* For informations regarding this file contact                         *
*			      office@mufasa.informatik.uni-mannheim.de *
*                                                                      *
***********************************************************************/

#ifndef NEWFSM_H_
#define NEWFSM_H_

// Includes
//--------------

//-- Std
#include <string>
using namespace std;

//-- Qt
#include <QtGui>
#include <QtCore>

//-- Core
class Fsm;


#include <ui_setupfsm.h>



class NewFsm : public QDialog, public Ui_FSMsetup { 

  Q_OBJECT

 public:
  NewFsm(Fsm * fsm,QWidget* parent =NULL, Qt::WFlags f = Qt::Dialog);

  void inText(QString qs);
  void outText(QString qs);

 private:

  Fsm * fsm;

  int incount;
  int outcount;

  void connections();
  void reject();
  void accept();

 private slots:
  void deleteOutput();
  void deleteInput();
  void newInput();
  void newOutput();
  void moveInputUp();
  void moveInputDown(); 
  void moveOutputUp();
  void moveOutputDown();

};


#endif
