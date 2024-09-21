#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>

#include "../../AutoOperateLib/Base.h"

enum class CurrentStatus
{
	IDLE,
	RECORDING,
	REPLAYING,
};

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();


private:
	QLineEdit* replayFilePathLineEdit = nullptr;
	QPushButton* browseReplayFileButton = nullptr;
	QTabWidget* tabWidget = nullptr;

	AO_HotkeyManager* hotkeyManager = nullptr;
	AO_ActionRecorder* actionRecorder = nullptr;
	AO_ActionSimulator* actionSimulator = nullptr;

	CurrentStatus status = CurrentStatus::IDLE;

	void SetupUI();
	void OpenActionFileDialog();
	void ReplayCompleted();
	void Start();
	void End();
};