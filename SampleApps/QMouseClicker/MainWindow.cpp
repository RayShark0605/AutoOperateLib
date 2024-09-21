#include "MainWindow.h"
#include <QLayout>


MainWindow::MainWindow(QWidget* parent) :QWidget(parent)
{
	SetupUI();

	if (hotkeyManager)
	{
		delete hotkeyManager;
		hotkeyManager = nullptr;
	}

	if (actionTrigger)
	{
		delete actionTrigger;
		actionTrigger = nullptr;
	}

	hotkeyManager = new AO_HotkeyManager();
	hotkeyManager->RegisterHotkey(1, MOD_CONTROL, '1', [this]() {StartClick(); });
	hotkeyManager->RegisterHotkey(2, MOD_CONTROL, '2', [this]() {StopClick(); });

	actionTrigger = new AO_ActionTrigger();
	actionTrigger->RegisterCallback(AO_ActionType::MouseMove, [this](const AO_ActionRecord& action) {
		this->OnMouseMove(action);
		});

}
MainWindow::~MainWindow()
{
	isClicking = false;
	hotkeyManager->UnregisterHotkey(1);
	hotkeyManager->UnregisterHotkey(2);

	delete hotkeyManager;
	delete actionTrigger;
}
void MainWindow::OnMouseMove(const AO_ActionRecord& action)
{
	if (posLabel)
	{
		currentPos = GetMousePos();
		const QString posText = QStringLiteral("当前坐标：(") + QString::number(currentPos.x) + ", " + QString::number(currentPos.y) + ")";
		QMetaObject::invokeMethod(posLabel, "setText", Qt::QueuedConnection, Q_ARG(QString, posText));
	}
	if (colorPixel)
	{
		currentColor = GetScreenPixelColor(currentPos);
		QMetaObject::invokeMethod(colorPixel, "setStyleSheet", Qt::QueuedConnection, Q_ARG(QString, QString("background-color: rgb(%1, %2, %3);").arg(currentColor.r).arg(currentColor.g).arg(currentColor.b)));
	}
	if (colorLabel)
	{
		QString colorText = QStringLiteral("当前颜色：R=") + QString::number(currentColor.r) + ", G=" + QString::number(currentColor.g) + ", B=" + QString::number(currentColor.b);
		QMetaObject::invokeMethod(colorLabel, "setText", Qt::QueuedConnection, Q_ARG(QString, colorText));
	}
}
void MainWindow::SetupUI()
{
	QVBoxLayout* layout = new QVBoxLayout(this);

	layout->addStretch();

	QLabel* infoLabel = new QLabel(QStringLiteral("开始连点：Ctrl + 1\n结束连点：Ctrl + 2"), this);
	QFont labelFont = infoLabel->font();
	labelFont.setPointSize(15);
	infoLabel->setFont(labelFont);
	layout->addWidget(infoLabel, 0, Qt::AlignHCenter);

	layout->addStretch();

	currentPos = GetMousePos();
	currentColor = GetScreenPixelColor(currentPos);

	QHBoxLayout* statusBarLayout = new QHBoxLayout(this);
	if (posLabel)
	{
		delete posLabel;
		posLabel = nullptr;
	}
	posLabel = new QLabel(QStringLiteral("当前坐标：(") + QString::number(currentPos.x) + ", " + QString::number(currentPos.y) + ")", this);
	labelFont = posLabel->font();
	labelFont.setPointSize(10);
	posLabel->setFont(labelFont);
	statusBarLayout->addWidget(posLabel, 0, Qt::AlignLeft);

	QHBoxLayout* colorLayout = new QHBoxLayout(this);
	if (colorPixel)
	{
		delete colorPixel;
		colorPixel = nullptr;
	}
	colorPixel = new QLabel(this);
	colorPixel->setFixedSize(12, 12);
	colorPixel->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(currentColor.r).arg(currentColor.g).arg(currentColor.b));
	colorLayout->addWidget(colorPixel);

	if (colorLabel)
	{
		delete colorLabel;
		colorLabel = nullptr;
	}
	colorLabel = new QLabel(QStringLiteral("当前颜色：R=") + QString::number(currentColor.r) + ", G=" + QString::number(currentColor.g) + ", B=" + QString::number(currentColor.b), this);
	labelFont = colorLabel->font();
	labelFont.setPointSize(10);
	colorLabel->setFont(labelFont);
	colorLayout->addWidget(colorLabel);

	statusBarLayout->addStretch();
	statusBarLayout->addLayout(colorLayout);
	layout->addLayout(statusBarLayout);

	setLayout(layout);

	setStyleSheet("background-color: white;");
	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	setBaseSize(400, 200);
	setWindowTitle(QStringLiteral("鼠标连点器"));
}
void MainWindow::StartClick()
{
	if (isClicking)
	{
		return;
	}
	isClicking = true;
	std::thread clickThread(&MainWindow::Click, this);
	clickThread.detach();
}
void MainWindow::StopClick()
{
	isClicking = false;
}
void MainWindow::Click()
{
	while (isClicking)
	{
		MouseLeftClick();
	}
}