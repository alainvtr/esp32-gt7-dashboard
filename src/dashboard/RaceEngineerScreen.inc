// 06/11/2026 01:45 - Bouton FAIT (conseil applique ; pour la repartition de freinage, reglee en piste, FAIT fait aussi la RAZ) ; appuyer de nouveau l'annule ; 3 boutons en bas
// 30/10/2026 15:28 - Creation : ecran de l'ingenieur de course (480x320) ouvert par un appui sur la cellule HANDLING a l'arret : transmission, bilan F/L/M/S avant / maintenant, comparaison, conseil, boutons RAZ MESURES et RETOUR AU DASHBOARD
// Ce fichier est inclus DANS la classe SHCustomProtocol (apres RadarTheme.inc,
// dont il reprend la geometrie de la cellule HANDLING pour la zone d'appui).

// Bandeau du haut : titre, voiture, choix de la transmission (4 boutons).
static constexpr int ENG_TITLE_X = 8;
static constexpr int ENG_TITLE_Y = 18;
static constexpr int ENG_CAR_X = 150;
static constexpr int ENG_DRIVE_BUTTON_COUNT = 4;
static constexpr int ENG_DRIVE_BUTTON_WIDTH = 56;
static constexpr int ENG_DRIVE_BUTTON_HEIGHT = 30;
static constexpr int ENG_DRIVE_BUTTON_GAP = 4;
static constexpr int ENG_DRIVE_BUTTON_Y = 3;
static constexpr int ENG_DRIVE_BUTTON_X0 = SCREEN_WIDTH - 8 -
	ENG_DRIVE_BUTTON_COUNT * ENG_DRIVE_BUTTON_WIDTH -
	(ENG_DRIVE_BUTTON_COUNT - 1) * ENG_DRIVE_BUTTON_GAP;

// Tableau : en-tetes sur 2 lignes, puis une ligne par phase (police 2, 16 px).
static constexpr int ENG_HEADER_Y = 46;
static constexpr int ENG_SUBHEADER_Y = 62;
static constexpr int ENG_ROW_Y = 84;
static constexpr int ENG_ROW_STEP = 21;
static constexpr int ENG_COL_PHASE_X = 12;
static constexpr int ENG_COL_BEFORE_BALANCE_X = 48;
static constexpr int ENG_COL_BEFORE_LOCK_X = 108;
static constexpr int ENG_COL_NOW_BALANCE_X = 200;
static constexpr int ENG_COL_NOW_LOCK_X = 262;
static constexpr int ENG_COL_TIME_X = 350;
static constexpr int ENG_COL_FEW_X = 400;
static constexpr int ENG_SEPARATOR_Y = 166;

// Comparaison, conseil (diagnostic, action, note) et boutons du bas.
static constexpr int ENG_COMPARE_Y = 180;
static constexpr int ENG_DIAGNOSIS_Y = 203;
static constexpr int ENG_ACTION_Y = 224;
static constexpr int ENG_NOTE_Y = 245;
static constexpr int ENG_BUTTON_Y = 264;
static constexpr int ENG_BUTTON_HEIGHT = 50;
static constexpr int ENG_DONE_X = 8;
static constexpr int ENG_DONE_WIDTH = 110;
static constexpr int ENG_RESET_X = 126;
static constexpr int ENG_RESET_WIDTH = 150;
static constexpr int ENG_BACK_X = 284;
static constexpr int ENG_BACK_WIDTH = SCREEN_WIDTH - 8 - ENG_BACK_X;

// Boutons de l'ecran (numeros passes par settingsButtonAtTouch / activateSettingsButton).
static constexpr int ENG_BUTTON_RESET = ENG_DRIVE_BUTTON_COUNT;
static constexpr int ENG_BUTTON_BACK = ENG_DRIVE_BUTTON_COUNT + 1;
static constexpr int ENG_BUTTON_DONE = ENG_DRIVE_BUTTON_COUNT + 2;

static int engineerDriveButtonX(int index)
{
	return ENG_DRIVE_BUTTON_X0 + index * (ENG_DRIVE_BUTTON_WIDTH + ENG_DRIVE_BUTTON_GAP);
}

// Zone d'appui qui ouvre l'ecran : la cellule HANDLING du theme Radar.
bool touchInsideHandlingCell() const
{
	return touchInside(RIGHT_X, cellY(HANDLING_CELL), COLUMN_WIDTH, COLUMN_LAST_CELL_HEIGHT);
}

void openRaceEngineer()
{
	if (!engineer.hasCar() || engineer.carCode() != currentCarCode)
		engineer.selectCar(currentCarCode);
	showSettingsScreen(SettingsScreen::Engineer);
}

// RAZ MESURES : la session en cours devient "avant" (apres un reglage fait en
// piste, typiquement la repartition de freinage), et on repart de zero.
void resetRaceEngineerSession()
{
	// Mesures, puis remise a zero (le FAIT eventuel est retenu par endSession).
	engineer.endSession(RaceEngineer::summarize(handling));
	handling.resetSession();
	handling.reset();
}

// FAIT n'existe que s'il y a un reglage a appliquer (ou un FAIT a annuler).
bool engineerDoneAvailable(const RaceEngineer::Advice &advice) const
{
	return engineer.appliedPending() || advice.kind == RaceEngineer::ADVICE_SETTING;
}

uint16_t engineerTendencyColor(float balance) const
{
	if (!isfinite(balance)) return TFT_DARKGREY;
	if (balance < 0.0f || balance > HANDLING_OVERSTEER_RATIO) return tft.color565(255, 70, 60);
	if (balance < HANDLING_UNDERSTEER_RATIO) return tft.color565(90, 160, 255);
	return TFT_WHITE;
}

void drawEngineerValue(float value, bool signedValue, int x, int y, uint16_t color)
{
	char text[12];
	if (!isfinite(value))
		snprintf(text, sizeof(text), "--");
	else
		snprintf(text, sizeof(text), signedValue ? "%+.2f" : "%.2f", value);
	tft.setTextColor(color, TFT_BLACK);
	tft.drawString(text, x, y, 2);
}

void drawRaceEngineerScreen(int pressedButton)
{
	const uint16_t grey = tft.color565(140, 140, 140);
	const uint16_t amber = tft.color565(240, 170, 40);
	const RaceEngineer::Summary now = RaceEngineer::summarize(handling);
	const RaceEngineer::Advice advice = engineer.analyze(now);

	// Bandeau du haut.
	tft.setTextDatum(ML_DATUM);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.drawString("INGENIEUR", ENG_TITLE_X, ENG_TITLE_Y, 4);
	tft.setTextColor(grey, TFT_BLACK);
	tft.drawString(String("Car ") + engineer.carCode(), ENG_CAR_X, ENG_TITLE_Y, 2);
	static const char *const driveLabels[ENG_DRIVE_BUTTON_COUNT] = {"FR", "MR", "RR", "FF/4WD"};
	for (int i = 0; i < ENG_DRIVE_BUTTON_COUNT; ++i)
	{
		const bool active = engineer.drivetrain() == static_cast<RaceEngineer::Drivetrain>(i + 1);
		drawSettingsButton(engineerDriveButtonX(i), ENG_DRIVE_BUTTON_Y, ENG_DRIVE_BUTTON_WIDTH,
			ENG_DRIVE_BUTTON_HEIGHT, driveLabels[i], pressedButton == i, active);
	}

	// En-tetes du tableau.
	tft.setTextDatum(ML_DATUM);
	tft.setTextColor(grey, TFT_BLACK);
	tft.drawString("AVANT", ENG_COL_BEFORE_BALANCE_X, ENG_HEADER_Y, 2);
	tft.drawString("MAINTENANT", ENG_COL_NOW_BALANCE_X, ENG_HEADER_Y, 2);
	tft.drawString("TEMPS", ENG_COL_TIME_X, ENG_HEADER_Y, 2);
	tft.drawString("Y/N", ENG_COL_BEFORE_BALANCE_X, ENG_SUBHEADER_Y, 2);
	tft.drawString("R/F", ENG_COL_BEFORE_LOCK_X, ENG_SUBHEADER_Y, 2);
	tft.drawString("Y/N", ENG_COL_NOW_BALANCE_X, ENG_SUBHEADER_Y, 2);
	tft.drawString("R/F", ENG_COL_NOW_LOCK_X, ENG_SUBHEADER_Y, 2);

	// Une ligne par phase : avant (gris), maintenant (couleur de l'equilibre), temps.
	for (int p = 0; p < RaceEngineer::PHASE_COUNT; ++p)
	{
		const int y = ENG_ROW_Y + p * ENG_ROW_STEP;
		const RaceEngineer::PhaseSummary &phase = now.phase[p];
		const RaceEngineer::BeforePhase before = engineer.before(p);
		const bool lineTargeted = advice.line == p && advice.kind == RaceEngineer::ADVICE_SETTING;

		tft.setTextColor(lineTargeted ? amber : TFT_WHITE, TFT_BLACK);
		const char phaseText[2] = {RaceEngineer::phaseName(p), '\0'};
		tft.drawString(phaseText, ENG_COL_PHASE_X, y, 2);
		drawEngineerValue(before.balance, false, ENG_COL_BEFORE_BALANCE_X, y, grey);
		drawEngineerValue(before.lock, true, ENG_COL_BEFORE_LOCK_X, y, grey);
		drawEngineerValue(phase.balanceTimeS > 0.0f ? phase.balance : NAN, false,
			ENG_COL_NOW_BALANCE_X, y,
			phase.valid() ? engineerTendencyColor(phase.balance) : grey);
		drawEngineerValue(phase.lockTimeS > 0.0f ? phase.lock : NAN, true,
			ENG_COL_NOW_LOCK_X, y,
			phase.lockTimeS >= ENGINEER_MIN_LOCK_TIME_S ? TFT_WHITE : grey);

		tft.setTextColor(phase.valid() ? TFT_WHITE : grey, TFT_BLACK);
		tft.drawString(String(static_cast<int>(phase.balanceTimeS + 0.5f)) + " s",
			ENG_COL_TIME_X, y, 2);
		if (!phase.valid())
		{
			tft.setTextColor(amber, TFT_BLACK);
			tft.drawString("peu", ENG_COL_FEW_X, y, 2);
		}
	}
	tft.drawFastHLine(ENG_TITLE_X, ENG_SEPARATOR_Y, SCREEN_WIDTH - 2 * ENG_TITLE_X, grey);

	// Effet du conseil de la session precedente.
	char comparison[80];
	const RaceEngineer::Verdict verdict = engineer.compare(now, comparison, sizeof(comparison));
	if (verdict != RaceEngineer::VERDICT_NONE)
	{
		uint16_t color = grey;
		if (verdict == RaceEngineer::VERDICT_BETTER) color = tft.color565(110, 200, 90);
		else if (verdict == RaceEngineer::VERDICT_WORSE) color = tft.color565(255, 70, 60);
		tft.setTextColor(color, TFT_BLACK);
		tft.drawString(comparison, ENG_TITLE_X, ENG_COMPARE_Y, 2);
	}

	// Conseil : diagnostic, action, note.
	const uint16_t diagnosisColor = advice.kind == RaceEngineer::ADVICE_OK
		? tft.color565(110, 200, 90)
		: amber;
	tft.setTextColor(diagnosisColor, TFT_BLACK);
	tft.drawString(advice.diagnosis, ENG_TITLE_X, ENG_DIAGNOSIS_Y, 2);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.drawString(advice.action, ENG_TITLE_X, ENG_ACTION_Y, 2);
	tft.setTextColor(grey, TFT_BLACK);
	if (engineer.appliedPending())
	{
		// Le reglage marque FAIT remplace la note : il sera compare au retour en piste.
		tft.setTextColor(tft.color565(110, 200, 90), TFT_BLACK);
		tft.drawString(String("FAIT : ") + engineer.appliedLabel() + ", compare au retour en piste",
			ENG_TITLE_X, ENG_NOTE_Y, 2);
	}
	else
	{
		tft.drawString(advice.note, ENG_TITLE_X, ENG_NOTE_Y, 2);
	}

	if (engineerDoneAvailable(advice))
		drawSettingsButton(ENG_DONE_X, ENG_BUTTON_Y, ENG_DONE_WIDTH, ENG_BUTTON_HEIGHT,
			engineer.appliedPending() ? "ANNULER" : "FAIT", pressedButton == ENG_BUTTON_DONE,
			engineer.appliedPending());
	drawSettingsButton(ENG_RESET_X, ENG_BUTTON_Y, ENG_RESET_WIDTH, ENG_BUTTON_HEIGHT,
		"RAZ MESURES", pressedButton == ENG_BUTTON_RESET);
	drawSettingsButton(ENG_BACK_X, ENG_BUTTON_Y, ENG_BACK_WIDTH, ENG_BUTTON_HEIGHT,
		"RETOUR AU DASHBOARD", pressedButton == ENG_BUTTON_BACK);
	tft.setTextDatum(TL_DATUM);
}

int raceEngineerButtonAtTouch() const
{
	for (int i = 0; i < ENG_DRIVE_BUTTON_COUNT; ++i)
		if (touchInside(engineerDriveButtonX(i), ENG_DRIVE_BUTTON_Y,
				ENG_DRIVE_BUTTON_WIDTH, ENG_DRIVE_BUTTON_HEIGHT))
			return i;
	if (touchInside(ENG_DONE_X, ENG_BUTTON_Y, ENG_DONE_WIDTH, ENG_BUTTON_HEIGHT) &&
		engineerDoneAvailable(engineer.analyze(RaceEngineer::summarize(handling))))
		return ENG_BUTTON_DONE;
	if (touchInside(ENG_RESET_X, ENG_BUTTON_Y, ENG_RESET_WIDTH, ENG_BUTTON_HEIGHT))
		return ENG_BUTTON_RESET;
	if (touchInside(ENG_BACK_X, ENG_BUTTON_Y, ENG_BACK_WIDTH, ENG_BUTTON_HEIGHT))
		return ENG_BUTTON_BACK;
	return -1;
}

void activateRaceEngineerButton(int button)
{
	if (button >= 0 && button < ENG_DRIVE_BUTTON_COUNT)
	{
		engineer.setDrivetrain(static_cast<RaceEngineer::Drivetrain>(button + 1));
		drawSettingsScreen();
	}
	else if (button == ENG_BUTTON_RESET)
	{
		resetRaceEngineerSession();
		drawSettingsScreen();
	}
	else if (button == ENG_BUTTON_DONE)
	{
		if (engineer.appliedPending())
		{
			engineer.cancelApplied();
		}
		else
		{
			const RaceEngineer::Advice advice = engineer.analyze(RaceEngineer::summarize(handling));
			engineer.markApplied(advice);
			// Repartition de freinage, reglee en piste : nouvelle session tout de suite.
			if (advice.onTrack)
				resetRaceEngineerSession();
		}
		drawSettingsScreen();
	}
	else if (button == ENG_BUTTON_BACK)
	{
		closeSettings();
	}
}
