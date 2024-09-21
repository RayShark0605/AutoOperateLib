#include <string>

#include "MainWindow.h"
#include <QLayout>
#include <QFileDialog>
#include <QCoreApplication>
#include <QMessagebox>

using namespace std;

namespace internal
{
	void FilterRecords(vector<AO_ActionRecord>& records)
	{
		bool isFindFirstCtrlUp = false;
		for (auto it = records.begin(); it != records.end();)
		{
			if (!isFindFirstCtrlUp && it->actionType == AO_ActionType::KeyUp && it->data.keyboard.vkCode == 162)
			{
				isFindFirstCtrlUp = true;
				it = records.erase(it);
			}
			else if (isFindFirstCtrlUp && it->actionType == AO_ActionType::KeyUp && it->data.keyboard.vkCode == 49)
			{
				it = records.erase(it);
				break;
			}
			else
			{
				it++;
			}
		}

		bool isFindLast2Down = false;
		for (auto it = records.rbegin(); it != records.rend();)
		{
			if (!isFindLast2Down && it->actionType == AO_ActionType::KeyDown && it->data.keyboard.vkCode == 50)
			{
				isFindLast2Down = true;
				it = make_reverse_iterator(records.erase(next(it).base()));
			}
			else if (isFindLast2Down && it->actionType == AO_ActionType::KeyDown && it->data.keyboard.vkCode == 162)
			{
				it = make_reverse_iterator(records.erase(next(it).base()));
				break;
			}
			else
			{
				it++;
			}
		}
	}
}
const string windowName = "operation recorder";
const QString windowNameQString = QString::fromStdString(windowName);

MainWindow::MainWindow(QWidget* parent) :QWidget(parent)
{
	SetupUI();

	if (actionRecorder)
	{
		delete actionRecorder;
		actionRecorder = nullptr;
	}
	actionRecorder = new AO_ActionRecorder();

	if (hotkeyManager)
	{
		delete hotkeyManager;
		hotkeyManager = nullptr;
	}
	hotkeyManager = new AO_HotkeyManager();
	hotkeyManager->RegisterHotkey(1, MOD_CONTROL, '1', [this]() {Start(); });
	hotkeyManager->RegisterHotkey(2, MOD_CONTROL, '2', [this]() {End(); });

	if (actionSimulator)
	{
		delete actionSimulator;
		actionSimulator = nullptr;
	}
	actionSimulator = new AO_ActionSimulator();
}
MainWindow::~MainWindow()
{
	if (hotkeyManager)
	{
		hotkeyManager->UnregisterHotkey(1);
		hotkeyManager->UnregisterHotkey(2);
		delete hotkeyManager;
		hotkeyManager = nullptr;
	}
	if (actionRecorder)
	{
		delete actionRecorder;
		actionRecorder = nullptr;
	}
	if (actionSimulator)
	{
		delete actionSimulator;
		actionSimulator = nullptr;
	}
}
void MainWindow::SetupUI()
{
	QVBoxLayout* layout = new QVBoxLayout(this);

	tabWidget = new QTabWidget(this);

	QWidget* recordWidget = new QWidget(this);
	QVBoxLayout* recordWidgetLayout = new QVBoxLayout(recordWidget);
	recordWidgetLayout->addStretch();
	QLabel* infoLabel = new QLabel(QStringLiteral("��ʼ��¼��Ctrl + 1\n������¼��Ctrl + 2"), this);
	QFont labelFont = infoLabel->font();
	labelFont.setPointSize(15);
	infoLabel->setFont(labelFont);
	recordWidgetLayout->addWidget(infoLabel, 0, Qt::AlignHCenter);
	recordWidgetLayout->addStretch();
	recordWidget->setLayout(recordWidgetLayout);
	tabWidget->addTab(recordWidget, QStringLiteral("¼�Ʋ���"));

	QWidget* replayWidget = new QWidget(this);
	QVBoxLayout* replayWidgetLayout = new QVBoxLayout(replayWidget);

	QVBoxLayout* selectReplayFileLayout = new QVBoxLayout(this);
	selectReplayFileLayout->addWidget(new QLabel(QStringLiteral("�ط��ļ���")));
	QHBoxLayout* lineEditButtonLayout = new QHBoxLayout(this);
	replayFilePathLineEdit = new QLineEdit();
	replayFilePathLineEdit->setReadOnly(true);
	lineEditButtonLayout->addWidget(replayFilePathLineEdit);
	browseReplayFileButton = new QPushButton(QStringLiteral("���..."), this);
	QObject::connect(browseReplayFileButton, &QPushButton::clicked, this, &MainWindow::OpenActionFileDialog);
	lineEditButtonLayout->addWidget(browseReplayFileButton);
	selectReplayFileLayout->addLayout(lineEditButtonLayout);
	selectReplayFileLayout->addStretch();
	infoLabel = new QLabel(QStringLiteral("��ʼ�طţ�Ctrl + 1\n�����طţ�Ctrl + 2"), this);
	labelFont = infoLabel->font();
	labelFont.setPointSize(15);
	infoLabel->setFont(labelFont);
	selectReplayFileLayout->addWidget(infoLabel, 0, Qt::AlignHCenter);
	selectReplayFileLayout->addStretch();
	replayWidgetLayout->addLayout(selectReplayFileLayout);

	replayWidget->setLayout(replayWidgetLayout);
	tabWidget->addTab(replayWidget, QStringLiteral("�طŲ���"));

	layout->addWidget(tabWidget);
	setLayout(layout);

	setStyleSheet("background-color: white;");
	setMinimumWidth(300);
	resize(400, 400);
	setWindowTitle(windowNameQString);

	status = CurrentStatus::IDLE;
}
void MainWindow::OpenActionFileDialog()
{
	const QString filter = "Action Files (*.action);;All Files (*)";
	QString fileName = QFileDialog::getOpenFileName(nullptr, QStringLiteral("ѡ��������ļ�"), QCoreApplication::applicationDirPath(), filter);
	if (!fileName.isEmpty())
	{
		fileName.replace('\\', '/');
		QMetaObject::invokeMethod(replayFilePathLineEdit, "setText", Qt::QueuedConnection, Q_ARG(QString, fileName));
	}
}
void MainWindow::ReplayCompleted()
{
	status = CurrentStatus::IDLE;
	
	MessageBoxW(nullptr, L"�ط��ѽ�����", StringToWString(windowName).data(), MB_OK | MB_TOPMOST);

	QMetaObject::invokeMethod(tabWidget, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
	QMetaObject::invokeMethod(browseReplayFileButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
	QMetaObject::invokeMethod(this, "showNormal", Qt::QueuedConnection);
	
}
void MainWindow::Start()
{
	if (status != CurrentStatus::IDLE)
	{
		return;
	}
	if (tabWidget->currentIndex() == 0)
	{
		QMetaObject::invokeMethod(this, "showMinimized", Qt::QueuedConnection);
		status = CurrentStatus::RECORDING;
		actionRecorder->StartRecording();
	}
	else if (tabWidget->currentIndex() == 1)
	{
		const string actionFilePath = replayFilePathLineEdit->text().toUtf8().constData();
		if (!IsFileExists(actionFilePath))
		{
			const string info = actionFilePath + " �ļ������ڣ�";
			MessageBoxW(nullptr, StringToWString(info).data(), L"����", MB_OK | MB_ICONERROR | MB_TOPMOST);
			return;
		}
		const vector<AO_ActionRecord> records = LoadRecordsFromTxtFile(actionFilePath);

		QMetaObject::invokeMethod(tabWidget, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
		QMetaObject::invokeMethod(browseReplayFileButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
		QMetaObject::invokeMethod(this, "showMinimized", Qt::QueuedConnection);
		status = CurrentStatus::REPLAYING;
		actionSimulator->Start(records, [&]() {
			ReplayCompleted();
			});
	}
}
void MainWindow::End()
{
	if (status == CurrentStatus::IDLE)
	{
		return;
	}
	if (status == CurrentStatus::RECORDING)
	{
		if (tabWidget->currentIndex() != 0)
		{
			return;
		}
		actionRecorder->StopRecording();

		const string currentExePath = GetCurrentExeDir();
		const string timeString = GetCurrentTimeAsString();
		const string savePath = currentExePath + "/" + timeString + ".action";
		vector<AO_ActionRecord> records = actionRecorder->GetRecords();
		internal::FilterRecords(records);

		if (SaveRecordsToTxtFile(records, savePath))
		{
			const string infoString = "¼�ƵĲ����ѱ������ļ� " + savePath + " �У�";
			MessageBoxW(nullptr, StringToWString(infoString).data(), L"¼���ѽ���", MB_OK | MB_TOPMOST);
		}
		QMetaObject::invokeMethod(this, "showNormal", Qt::QueuedConnection);
		status = CurrentStatus::IDLE;
	}
	else if (status == CurrentStatus::REPLAYING)
	{
		if (tabWidget->currentIndex() != 1)
		{
			return;
		}
		actionSimulator->Stop();
		QMetaObject::invokeMethod(tabWidget, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
		QMetaObject::invokeMethod(browseReplayFileButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
		QMetaObject::invokeMethod(this, "showNormal", Qt::QueuedConnection);
		status = CurrentStatus::IDLE;
	}
}

