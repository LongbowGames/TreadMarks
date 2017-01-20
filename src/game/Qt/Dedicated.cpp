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

#include "Dedicated.h"
#include "../TankGame.h"

#include <QScrollBar>

Dedicated::Dedicated()
{
	setWindowTitle("Tread Marks Dedicated Server");

	pLayout = new QGridLayout;

	pTextMain = new QTextEdit;
	pTextMain->setReadOnly(true);
	pLayout->addWidget(pTextMain, 0, 0, 1, 3);

	pTextSub = new QTextEdit;
	pTextSub->setReadOnly(true);
	pLayout->addWidget(pTextSub, 1, 0, 1, 3);

	pEditChat = new QLineEdit;
	pLayout->addWidget(pEditChat, 2, 0, 1, 2);

	pBtnChat = new QPushButton;
	pBtnChat->setDefault(true);
	pBtnChat->setText("Send Chat Message");
	pLayout->addWidget(pBtnChat, 2, 2);

	pEditBytesOut = new QLineEdit;
	pEditBytesOut->setDisabled(true);
	pLayout->addWidget(pEditBytesOut, 3, 0);

	pEditBytesIn = new QLineEdit;
	pEditBytesIn->setDisabled(true);
	pLayout->addWidget(pEditBytesIn, 4, 0);

	pEditCpsOut = new QLineEdit;
	pEditCpsOut->setDisabled(true);
	pLayout->addWidget(pEditCpsOut, 3, 1);

	pEditCpsIn = new QLineEdit;
	pEditCpsIn->setDisabled(true);
	pLayout->addWidget(pEditCpsIn, 4, 1);

	pBtnStartMap = new QPushButton;
	pBtnStartMap->setText("Re-Start Map");
	pLayout->addWidget(pBtnStartMap, 3, 2, 2, 1, Qt::AlignCenter);


	pCentralWidget = new QWidget;
	pCentralWidget->setLayout(pLayout);
	setCentralWidget(pCentralWidget);

	connect(pBtnChat, &QPushButton::clicked, this, &Dedicated::clickChat);
	connect(pBtnStartMap, &QPushButton::clicked, this, &Dedicated::clickStartMap);

	Reinit();
}

void Dedicated::closeEvent(QCloseEvent* event)
{
	CTankGame::Get().GetGameState()->Quit = true;
	QMainWindow::closeEvent(event);
}

void Dedicated::Reinit()
{
	if(sMainOutput.length() != LastMainLen){
		pTextMain->setText(sMainOutput);
		QScrollBar* sb = pTextMain->verticalScrollBar();
		sb->setValue(sb->maximum());
		LastMainLen = sMainOutput.length();
	}
	if(sSubOutput.length() != LastSubLen){
		pTextSub->setText(sSubOutput);
		QScrollBar* sb = pTextSub->verticalScrollBar();
		sb->setValue(sb->maximum());
		LastSubLen = sSubOutput.length();
	}
	pEditBytesOut->setText("Bytes Out: " + QString::number((int)CTankGame::Get().GetGameState()->LastNetBytesOut));
	pEditBytesIn->setText("Bytes In: " + QString::number((int)CTankGame::Get().GetGameState()->LastNetBytesIn));
	pEditCpsOut->setText("CPS Out: " + QString::number((int)CTankGame::Get().GetGameState()->NetCPSOut));
	pEditCpsIn->setText("CPS In: " + QString::number((int)CTankGame::Get().GetGameState()->NetCPSIn));
}

#define MAX_OUTPUT_BUF 5000
void Dedicated::OutputFunc(const char *text, int level){

	if(!text) return;
	char buf[1024];
	int i = 0, j = 0;
	while(text[i] != 0 && j < 1000){
		if(text[i] != '\n'){
			buf[j++] = text[i++];
		}else{
			buf[j++] = '\r';
			buf[j++] = '\n';
			i++;
		}
	}
	buf[j] = 0;

	if(level <= 0) sSubOutput.append(buf);
	else sMainOutput.append(buf);
	if(sSubOutput.length() > MAX_OUTPUT_BUF) sSubOutput = sSubOutput.right(MAX_OUTPUT_BUF - 1000);
	if(sMainOutput.length() > MAX_OUTPUT_BUF) sMainOutput = sMainOutput.right(MAX_OUTPUT_BUF - 1000);
	Reinit();
}

void Dedicated::clickChat()
{
	auto buf = pEditChat->text();
	if(buf.length() > 1 && buf[0] == '/')
	{
		buf = buf.mid(1);
		CTankGame::Get().ProcessServerCommand(buf.toUtf8().data());
	}
	else
	{
		CTankGame::Get().GetVW()->ChatMessage(buf.toUtf8().data());
	}
	pEditChat->setText("");
}

void Dedicated::clickStartMap()
{
	CTankGame::Get().GetGameState()->ReStartMap = 1;
}
