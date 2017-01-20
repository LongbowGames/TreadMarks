// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __DEDICATED_H__
#define __DEDICATED_H__

#include <QMainWindow>
#include <QLayout>
#include <QTextBrowser>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class Dedicated : public QMainWindow
{
	Q_OBJECT

public:
	Dedicated();

	void Reinit();
	void OutputFunc(const char *text, int level);

protected:
	void closeEvent(QCloseEvent* event);

	void clickChat();
	void clickStartMap();

private:
	QWidget*		pCentralWidget;
	QGridLayout*	pLayout;
	QTextEdit		*pTextMain, *pTextSub;
	QLineEdit		*pEditChat, *pEditBytesOut, *pEditCpsOut, *pEditBytesIn, *pEditCpsIn;
	QPushButton		*pBtnStartMap, *pBtnChat;

	int LastMainLen = -1, LastSubLen = -1;
	QString			sMainOutput, sSubOutput;
};

#endif
