#pragma once
#include <QWidget>
#include <QStatusBar>
#include <QLabel>

#include "../../AutoOperateLib/Base.h"

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

private:
	QLabel* posLabel = nullptr;
	QLabel* colorPixel = nullptr;
	QLabel* colorLabel = nullptr;

	AO_HotkeyManager* hotkeyManager = nullptr;
	AO_ActionTrigger* actionTrigger = nullptr;

	AO_Point currentPos;
	AO_Color currentColor;
	bool isClicking = false;

	void OnMouseMove(const AO_ActionRecord& action);
	void SetupUI();
	void StartClick();
	void StopClick();
	void Click();
};







