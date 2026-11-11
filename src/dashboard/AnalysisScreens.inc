// 08/11/2026 20:45 - BOITE : 4 lignes de verdict (vitesse max, etagement, virage lent, passages), une action chiffree en valeurs de rapport, tableau AVANT / MAINTENANT reduit ; MOTEUR : rapports recus du jeu et retombee a chaque passage (a comparer a la fiche GT7)
// 06/11/2026 02:05 - BOITE : etagement (chute de regime a chaque passage, virage le plus lent) et bouton MOTEUR ; nouvel ecran MOTEUR de saisie des regimes de couple max et de puissance max (par voiture)
// 06/11/2026 01:45 - Creation : ecrans BOITE (appui sur le rapport engage) et SUSPENSION (appui sur TYRE TEMP), ouverts a l'arret comme l'ingenieur de course, avec RAZ et RETOUR AU DASHBOARD
// Ce fichier est inclus DANS la classe SHCustomProtocol, apres RadarTheme.inc
// (zones d'appui) et RaceEngineerScreen.inc (mise en page ENG_*, partagee).

// Zones d'appui sur le tableau de bord.
static constexpr int GEAR_TOUCH_HALF_WIDTH = 50;
static constexpr int GEAR_TOUCH_Y = GEAR_LABEL_Y - 10;
static constexpr int GEAR_TOUCH_HEIGHT = 85;	// libelle GEAR + chiffre du rapport
static constexpr int TYRE_TOUCH_X = BOTTOM_BAR_SEPARATOR_2_X;
static constexpr int TYRE_TOUCH_MARGIN = 3;

// Boutons du bas, communs aux deux ecrans : RAZ et RETOUR AU DASHBOARD (SUSPENSION),
// RAZ, MOTEUR et RETOUR AU DASHBOARD (BOITE, memes positions que l'ingenieur).
static constexpr int ANALYSIS_RESET_X = 8;
static constexpr int ANALYSIS_RESET_WIDTH = 200;
static constexpr int ANALYSIS_BACK_X = 222;
static constexpr int ANALYSIS_BACK_WIDTH = SCREEN_WIDTH - 8 - ANALYSIS_BACK_X;
static constexpr int GEARBOX_RESET_WIDTH = 150;
static constexpr int GEARBOX_ENGINE_X = 166;
static constexpr int GEARBOX_ENGINE_WIDTH = 110;
static constexpr int ANALYSIS_BUTTON_RESET = 0;
static constexpr int ANALYSIS_BUTTON_BACK = 1;
static constexpr int ANALYSIS_BUTTON_ENGINE = 2;

// Ecran BOITE : un verdict par sujet, l'action a faire, puis un petit tableau
// AVANT / MAINTENANT.
static constexpr int GEARBOX_INFO_Y = 44;
static constexpr int GEARBOX_VERDICT_Y = 64;
static constexpr int GEARBOX_VERDICT_STEP = 17;
static constexpr int GEARBOX_VERDICT_TEXT_X = 100;
static constexpr int GEARBOX_ACTION_Y = 136;
static constexpr int GEARBOX_NOTE_Y = 154;
static constexpr int GEARBOX_SEPARATOR_Y = 166;
static constexpr int GEARBOX_HEADER_Y = 176;
static constexpr int GEARBOX_ROW_Y = 192;
static constexpr int GEARBOX_ROW_STEP = 16;
static constexpr int GEARBOX_COL_BEFORE_X = 300;
static constexpr int GEARBOX_COL_NOW_X = 390;
// Ecran MOTEUR : rapports recus et retombee a chaque passage, sous les reglages.
static constexpr int ENGINE_RATIOS_Y = 214;
static constexpr int ENGINE_SHIFTS_Y = 232;
static constexpr int ENGINE_VALUES_X = 92;
static constexpr int ENGINE_RATIO_STEP = 48;
static constexpr int ENGINE_SHIFT_STEP = 64;

// Ecran MOTEUR : 2 lignes de reglage (couple max, puissance max), 4 boutons chacune.
static constexpr int ENGINE_ROW_COUNT = 2;
static constexpr int ENGINE_STEP_COUNT = 4;
static constexpr int ENGINE_ROW_STEP = 84;	// ecart vertical entre les 2 lignes
static constexpr int ENGINE_LABEL_Y = 52;	// 1re ligne
static constexpr int ENGINE_BUTTON_Y = 66;
static constexpr int ENGINE_BUTTON_HEIGHT = 50;
static constexpr int ENGINE_VALUE_X = 166;
static constexpr int ENGINE_VALUE_WIDTH = 148;
static constexpr int ENGINE_BUTTON_OK = ENGINE_ROW_COUNT * ENGINE_STEP_COUNT;

// Boutons -1000 / -100 de part et d'autre de la valeur, +100 / +1000 a droite.
// (Fonctions plutot que tableaux constexpr membres, qui demanderaient une
// definition hors de la classe en C++11.)
static int engineButtonY(int row) { return ENGINE_BUTTON_Y + row * ENGINE_ROW_STEP; }
static int engineButtonX(int step) { return step == 0 ? 8 : step == 1 ? 92 : step == 2 ? 318 : 394; }
static int engineButtonWidth(int step) { return step == 0 || step == 3 ? 78 : 70; }
static int engineStep(int step)
{
	return step == 0 ? -ENGINE_RPM_LARGE_STEP : step == 1 ? -ENGINE_RPM_SMALL_STEP
		: step == 2 ? ENGINE_RPM_SMALL_STEP : ENGINE_RPM_LARGE_STEP;
}
static const char *engineStepLabel(int step)
{
	return step == 0 ? "-1000" : step == 1 ? "-100" : step == 2 ? "+100" : "+1000";
}

// Ecran SUSPENSION : une ligne par roue (+ caisse), une colonne par situation.
static constexpr int SUSP_HEADER_Y = 52;
static constexpr int SUSP_ROW_Y = 74;
static constexpr int SUSP_ROW_STEP = 20;
static constexpr int SUSP_COL_X0 = 62;
static constexpr int SUSP_COL_STEP = 59;
static constexpr int SUSP_TIME_Y = SUSP_ROW_Y + SuspensionTrace::CHANNEL_COUNT * SUSP_ROW_STEP + 4;
static constexpr int SUSP_NOTE_1_Y = 214;
static constexpr int SUSP_NOTE_2_Y = 236;

bool touchInsideGearZone() const
{
	return touchInside(RADAR_CENTER_X - GEAR_TOUCH_HALF_WIDTH, GEAR_TOUCH_Y,
		2 * GEAR_TOUCH_HALF_WIDTH, GEAR_TOUCH_HEIGHT);
}

bool touchInsideTyreZone() const
{
	return touchInside(TYRE_TOUCH_X, BOTTOM_BAR_Y - TYRE_TOUCH_MARGIN,
		BOTTOM_BAR_X + BOTTOM_BAR_WIDTH - TYRE_TOUCH_X, BOTTOM_BAR_HEIGHT + 2 * TYRE_TOUCH_MARGIN);
}

bool isAnalysisScreen() const
{
	return settingsScreen == SettingsScreen::Engineer ||
		settingsScreen == SettingsScreen::Gearbox ||
		settingsScreen == SettingsScreen::Engine ||
		settingsScreen == SettingsScreen::Suspension;
}

// Voiture en piste chargee dans l'ingenieur (transmission, regimes moteur).
void selectEngineerCar()
{
	if (!engineer.hasCar() || engineer.carCode() != currentCarCode)
		engineer.selectCar(currentCarCode);
}

void openGearbox()
{
	selectEngineerCar();
	showSettingsScreen(SettingsScreen::Gearbox);
}

// Titre a gauche, voiture a droite (meme bandeau que l'ingenieur).
void drawAnalysisTitle(const char *title)
{
	tft.setTextDatum(ML_DATUM);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.drawString(title, ENG_TITLE_X, ENG_TITLE_Y, 4);
	tft.setTextColor(tft.color565(140, 140, 140), TFT_BLACK);
	tft.drawString(String("Car ") + currentCarCode, ENG_CAR_X + 60, ENG_TITLE_Y, 2);
}

void drawAnalysisButtons(const char *resetLabel, int pressedButton)
{
	if (settingsScreen == SettingsScreen::Gearbox)
	{
		drawSettingsButton(ANALYSIS_RESET_X, ENG_BUTTON_Y, GEARBOX_RESET_WIDTH, ENG_BUTTON_HEIGHT,
			resetLabel, pressedButton == ANALYSIS_BUTTON_RESET);
		drawSettingsButton(GEARBOX_ENGINE_X, ENG_BUTTON_Y, GEARBOX_ENGINE_WIDTH, ENG_BUTTON_HEIGHT,
			"MOTEUR", pressedButton == ANALYSIS_BUTTON_ENGINE);
		drawSettingsButton(ENG_BACK_X, ENG_BUTTON_Y, ENG_BACK_WIDTH, ENG_BUTTON_HEIGHT,
			"RETOUR AU DASHBOARD", pressedButton == ANALYSIS_BUTTON_BACK);
	}
	else
	{
		drawSettingsButton(ANALYSIS_RESET_X, ENG_BUTTON_Y, ANALYSIS_RESET_WIDTH, ENG_BUTTON_HEIGHT,
			resetLabel, pressedButton == ANALYSIS_BUTTON_RESET);
		drawSettingsButton(ANALYSIS_BACK_X, ENG_BUTTON_Y, ANALYSIS_BACK_WIDTH, ENG_BUTTON_HEIGHT,
			"RETOUR AU DASHBOARD", pressedButton == ANALYSIS_BUTTON_BACK);
	}
	tft.setTextDatum(TL_DATUM);
}

int analysisButtonAtTouch() const
{
	if (settingsScreen == SettingsScreen::Engine)
		return engineButtonAtTouch();
	if (settingsScreen == SettingsScreen::Gearbox)
	{
		if (touchInside(ANALYSIS_RESET_X, ENG_BUTTON_Y, GEARBOX_RESET_WIDTH, ENG_BUTTON_HEIGHT))
			return ANALYSIS_BUTTON_RESET;
		if (touchInside(GEARBOX_ENGINE_X, ENG_BUTTON_Y, GEARBOX_ENGINE_WIDTH, ENG_BUTTON_HEIGHT))
			return ANALYSIS_BUTTON_ENGINE;
		if (touchInside(ENG_BACK_X, ENG_BUTTON_Y, ENG_BACK_WIDTH, ENG_BUTTON_HEIGHT))
			return ANALYSIS_BUTTON_BACK;
		return -1;
	}
	if (touchInside(ANALYSIS_RESET_X, ENG_BUTTON_Y, ANALYSIS_RESET_WIDTH, ENG_BUTTON_HEIGHT))
		return ANALYSIS_BUTTON_RESET;
	if (touchInside(ANALYSIS_BACK_X, ENG_BUTTON_Y, ANALYSIS_BACK_WIDTH, ENG_BUTTON_HEIGHT))
		return ANALYSIS_BUTTON_BACK;
	return -1;
}

void activateAnalysisButton(int button)
{
	if (settingsScreen == SettingsScreen::Engine)
	{
		activateEngineButton(button);
		return;
	}
	if (button == ANALYSIS_BUTTON_ENGINE && settingsScreen == SettingsScreen::Gearbox)
	{
		showSettingsScreen(SettingsScreen::Engine);
		return;
	}
	if (button == ANALYSIS_BUTTON_RESET)
	{
		if (settingsScreen == SettingsScreen::Gearbox)
			gearbox.endSession(currentCarCode);
		else
			suspension.reset();
		drawSettingsScreen();
	}
	else if (button == ANALYSIS_BUTTON_BACK)
	{
		closeSettings();
	}
}

// ---------------------------------------------------------------------------
// BOITE
// ---------------------------------------------------------------------------
// Texte d'une valeur, "--" si absente ; format printf a une valeur.
static String analysisText(float value, const char *format)
{
	if (!isfinite(value)) return "--";
	char text[16];
	snprintf(text, sizeof(text), format, value);
	return text;
}

void drawGearboxRow(int row, const char *label, const String &before, const String &now)
{
	const int y = GEARBOX_ROW_Y + row * GEARBOX_ROW_STEP;
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.drawString(label, ENG_TITLE_X + 4, y, 2);
	tft.setTextColor(tft.color565(140, 140, 140), TFT_BLACK);
	tft.drawString(before, GEARBOX_COL_BEFORE_X, y, 2);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.drawString(now, GEARBOX_COL_NOW_X, y, 2);
}

void drawGearboxScreen(int pressedButton)
{
	static const char *const topicLabels[GearboxTrace::TOPIC_COUNT] = {
		"Vitesse max", "Etagement", "Virage lent", "Passages"};
	const uint16_t grey = tft.color565(140, 140, 140);
	const uint16_t amber = tft.color565(240, 170, 40);
	const uint16_t green = tft.color565(110, 200, 90);
	const GearboxTrace::Summary now = gearbox.summary();
	GearboxTrace::Summary before;
	const bool hasBefore = gearbox.before(currentCarCode, before);
	const GearboxTrace::EngineBand &engine = engineer.engineBand();
	const GearboxTrace::Report report = GearboxTrace::report(now, engine);

	drawAnalysisTitle("BOITE");
	tft.setTextColor(grey, TFT_BLACK);
	tft.drawString(String("Passage ") + (now.shiftRpm > 0 ? String(now.shiftRpm) : String("--")) +
		"  calc max " + (now.calcMaxSpeed > 0 ? String(now.calcMaxSpeed) : String("--")) +
		"  couple " + (engine.torqueRpm > 0 ? String(engine.torqueRpm) : String("--")) +
		"  puiss. " + (engine.powerRpm > 0 ? String(engine.powerRpm) : String("--")),
		ENG_TITLE_X, GEARBOX_INFO_Y, 2);

	// Un verdict par sujet : vert OK, ambre a corriger, gris inconnu.
	for (int t = 0; t < GearboxTrace::TOPIC_COUNT; ++t)
	{
		const int y = GEARBOX_VERDICT_Y + t * GEARBOX_VERDICT_STEP;
		const GearboxTrace::Verdict &verdict = report.topic[t];
		tft.setTextColor(t == report.actionTopic ? amber : TFT_WHITE, TFT_BLACK);
		tft.drawString(topicLabels[t], ENG_TITLE_X, y, 2);
		uint16_t color = grey;
		if (verdict.level == GearboxTrace::LEVEL_OK) color = green;
		else if (verdict.level == GearboxTrace::LEVEL_WARN) color = amber;
		tft.setTextColor(color, TFT_BLACK);
		tft.drawString(verdict.text, GEARBOX_VERDICT_TEXT_X, y, 2);
	}

	// L'action (un seul reglage a la fois) et sa justification.
	tft.setTextColor(report.actionTopic >= 0 ? TFT_WHITE : green, TFT_BLACK);
	tft.drawString(String("> ") + report.action, ENG_TITLE_X, GEARBOX_ACTION_Y, 2);
	tft.setTextColor(grey, TFT_BLACK);
	tft.drawString(report.note, ENG_TITLE_X + 12, GEARBOX_NOTE_Y, 2);
	tft.drawFastHLine(ENG_TITLE_X, GEARBOX_SEPARATOR_Y, SCREEN_WIDTH - 2 * ENG_TITLE_X, grey);

	tft.drawString("AVANT", GEARBOX_COL_BEFORE_X, GEARBOX_HEADER_Y, 2);
	tft.drawString("MAINTENANT", GEARBOX_COL_NOW_X, GEARBOX_HEADER_Y, 2);
	const GearboxTrace::Summary empty;
	const GearboxTrace::Summary &b = hasBefore ? before : empty;
	char label[40];
	char gear[8];
	drawGearboxRow(0, "Tours valides",
		hasBefore ? String(b.laps) : String("--"), String(now.laps));
	drawGearboxRow(1, "Vitesse max du tour (km/h)",
		analysisText(b.maxSpeedKmh, "%.0f"), analysisText(now.maxSpeedKmh, "%.0f"));
	snprintf(label, sizeof(label), "Regime max en %s (%% passage)",
		GearboxTrace::gearName(max(now.topGear, 1), gear, sizeof(gear)));
	drawGearboxRow(2, label, analysisText(b.peakRpmRatio * 100.0f, "%.0f"),
		analysisText(now.peakRpmRatio * 100.0f, "%.0f"));
	// Virage le plus lent : "2e 4150" (rapport, regime).
	const auto cornerText = [](const GearboxTrace::Summary &summary) -> String {
		if (summary.cornerGear <= 0) return "--";
		char name[8];
		return String(GearboxTrace::gearName(summary.cornerGear, name, sizeof(name))) + " " +
			String(static_cast<int>(summary.cornerRpm + 0.5f));
	};
	drawGearboxRow(3, "Virage le plus lent (tr/min)", cornerText(b), cornerText(now));

	drawAnalysisButtons("RAZ MESURES", pressedButton);
}

// ---------------------------------------------------------------------------
// MOTEUR (saisie des regimes de la plage utile, memorises pour la voiture)
// ---------------------------------------------------------------------------
int engineRowValue(int row) const
{
	const GearboxTrace::EngineBand &engine = engineer.engineBand();
	return row == 0 ? engine.torqueRpm : engine.powerRpm;
}

void drawEngineValue(int row)
{
	const int y = engineButtonY(row);
	tft.fillRect(ENGINE_VALUE_X, y, ENGINE_VALUE_WIDTH, ENGINE_BUTTON_HEIGHT, TFT_BLACK);
	const int value = engineRowValue(row);
	tft.setTextDatum(MC_DATUM);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.drawString(value > 0 ? String(value) : String("--"),
		ENGINE_VALUE_X + ENGINE_VALUE_WIDTH / 2, y + ENGINE_BUTTON_HEIGHT / 2, 4);
	tft.setTextDatum(TL_DATUM);
}

void drawEngineScreen(int pressedButton)
{
	static const char *const rowLabels[ENGINE_ROW_COUNT] = {
		"COUPLE MAX (tr/min)", "PUISSANCE MAX (tr/min)"};
	drawAnalysisTitle("MOTEUR");
	for (int row = 0; row < ENGINE_ROW_COUNT; ++row)
	{
		tft.setTextDatum(ML_DATUM);
		tft.setTextColor(tft.color565(140, 140, 140), TFT_BLACK);
		tft.drawString(rowLabels[row], ENG_TITLE_X, ENGINE_LABEL_Y + row * ENGINE_ROW_STEP, 2);
		for (int step = 0; step < ENGINE_STEP_COUNT; ++step)
		{
			const int button = row * ENGINE_STEP_COUNT + step;
			drawSettingsButton(engineButtonX(step), engineButtonY(row), engineButtonWidth(step),
				ENGINE_BUTTON_HEIGHT, engineStepLabel(step), pressedButton == button);
		}
		drawEngineValue(row);
	}
	drawEngineGearing();
	drawSettingsButton(ANALYSIS_RESET_X, ENG_BUTTON_Y, SCREEN_WIDTH - 2 * ANALYSIS_RESET_X,
		ENG_BUTTON_HEIGHT, "OK (RETOUR A LA BOITE)", pressedButton == ENGINE_BUTTON_OK);
	tft.setTextDatum(TL_DATUM);
}

// Rapports recus du jeu (a comparer a la fiche de transmission de GT7) et regime
// apres chaque passage au regime de passage du jeu : rouge sous le couple max.
void drawEngineGearing()
{
	const uint16_t grey = tft.color565(140, 140, 140);
	const GearboxTrace::Summary now = gearbox.summary();
	const GearboxTrace::EngineBand &engine = engineer.engineBand();
	tft.setTextDatum(ML_DATUM);
	tft.setTextColor(grey, TFT_BLACK);
	tft.drawString("Rapports", ENG_TITLE_X, ENGINE_RATIOS_Y, 2);
	tft.drawString("Retombee", ENG_TITLE_X, ENGINE_SHIFTS_Y, 2);
	if (now.topGear <= 0)
	{
		tft.drawString("-- (rouler pour les recevoir)", ENGINE_VALUES_X, ENGINE_RATIOS_Y, 2);
		return;
	}
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	for (int g = 1; g <= now.topGear; ++g)
		tft.drawString(analysisText(now.ratios[g - 1], "%.3f"),
			ENGINE_VALUES_X + (g - 1) * ENGINE_RATIO_STEP, ENGINE_RATIOS_Y, 2);
	for (int g = 1; g < now.topGear; ++g)
	{
		const float after = now.rpmAfterShift(g, static_cast<float>(now.shiftRpm));
		uint16_t color = grey;
		if (isfinite(after) && engine.torqueRpm > 0)
			color = after < engine.torqueRpm ? tft.color565(255, 70, 60) : TFT_WHITE;
		tft.setTextColor(color, TFT_BLACK);
		tft.drawString(String(g) + ">" + String(g + 1) + " " +
			(isfinite(after) ? String(static_cast<int>(after + 0.5f)) : String("--")),
			ENGINE_VALUES_X + (g - 1) * ENGINE_SHIFT_STEP, ENGINE_SHIFTS_Y, 2);
	}
}

int engineButtonAtTouch() const
{
	for (int row = 0; row < ENGINE_ROW_COUNT; ++row)
		for (int step = 0; step < ENGINE_STEP_COUNT; ++step)
			if (touchInside(engineButtonX(step), engineButtonY(row), engineButtonWidth(step),
					ENGINE_BUTTON_HEIGHT))
				return row * ENGINE_STEP_COUNT + step;
	if (touchInside(ANALYSIS_RESET_X, ENG_BUTTON_Y, SCREEN_WIDTH - 2 * ANALYSIS_RESET_X, ENG_BUTTON_HEIGHT))
		return ENGINE_BUTTON_OK;
	return -1;
}

void activateEngineButton(int button)
{
	if (button == ENGINE_BUTTON_OK)
	{
		showSettingsScreen(SettingsScreen::Gearbox);
		return;
	}
	if (button < 0 || button >= ENGINE_BUTTON_OK) return;
	const int row = button / ENGINE_STEP_COUNT;
	const int value = max(0, engineRowValue(row) + engineStep(button % ENGINE_STEP_COUNT));
	const GearboxTrace::EngineBand &engine = engineer.engineBand();
	engineer.setEngineBand(row == 0 ? value : engine.torqueRpm, row == 1 ? value : engine.powerRpm);
	drawEngineValue(row);	// seule la valeur change : pas de redessin complet (scintillement)
}

// ---------------------------------------------------------------------------
// SUSPENSION (releves)
// ---------------------------------------------------------------------------
// 4 chiffres significatifs, quelle que soit l'unite (m, mm...).
static String suspensionText(float value)
{
	if (!isfinite(value)) return "--";
	const float magnitude = fabsf(value);
	return analysisText(value, magnitude < 10.0f ? "%.3f" : (magnitude < 100.0f ? "%.2f" : "%.1f"));
}

void drawSuspensionScreen(int pressedButton)
{
	const uint16_t grey = tft.color565(140, 140, 140);
	static const char *const columns[] = {"ARRET", "MIN", "MAX", "FREIN", "ACCEL", "VIR G", "VIR D"};
	static const char *const rows[SuspensionTrace::CHANNEL_COUNT] = {"AVG", "AVD", "ARG", "ARD", "CAISSE"};
	static const int columnCondition[] = {
		SuspensionTrace::COND_STAND, -1, -2, SuspensionTrace::COND_BRAKE,
		SuspensionTrace::COND_ACCEL, SuspensionTrace::COND_LEFT, SuspensionTrace::COND_RIGHT};
	static constexpr int COLUMN_COUNT = sizeof(columns) / sizeof(columns[0]);

	drawAnalysisTitle("SUSPENSION");
	tft.setTextColor(grey, TFT_BLACK);
	for (int c = 0; c < COLUMN_COUNT; ++c)
		tft.drawString(columns[c], SUSP_COL_X0 + c * SUSP_COL_STEP, SUSP_HEADER_Y, 2);

	for (int r = 0; r < SuspensionTrace::CHANNEL_COUNT; ++r)
	{
		const int y = SUSP_ROW_Y + r * SUSP_ROW_STEP;
		const SuspensionTrace::Channel &channel = suspension.channel(r);
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.drawString(rows[r], ENG_TITLE_X, y, 2);
		for (int c = 0; c < COLUMN_COUNT; ++c)
		{
			float value = NAN;
			if (columnCondition[c] == -1) value = channel.minValue;
			else if (columnCondition[c] == -2) value = channel.maxValue;
			else value = channel.stat[columnCondition[c]].mean();
			tft.drawString(suspensionText(value), SUSP_COL_X0 + c * SUSP_COL_STEP, y, 2);
		}
	}

	// Temps cumule par situation (MIN et MAX : en roulant).
	tft.setTextColor(grey, TFT_BLACK);
	tft.drawString("temps", ENG_TITLE_X, SUSP_TIME_Y, 2);
	for (int c = 0; c < COLUMN_COUNT; ++c)
	{
		if (columnCondition[c] < 0) continue;
		tft.drawString(String(static_cast<int>(suspension.conditionTimeS(columnCondition[c]) + 0.5f)) + " s",
			SUSP_COL_X0 + c * SUSP_COL_STEP, SUSP_TIME_Y, 2);
	}
	tft.drawFastHLine(ENG_TITLE_X, SUSP_TIME_Y + 14, SCREEN_WIDTH - 2 * ENG_TITLE_X, grey);

	tft.setTextColor(tft.color565(240, 170, 40), TFT_BLACK);
	tft.drawString("Releves seulement : unite et sens a confirmer", ENG_TITLE_X, SUSP_NOTE_1_Y, 2);
	tft.setTextColor(grey, TFT_BLACK);
	tft.drawString("Photo de l'ecran apres quelques tours -> conseils", ENG_TITLE_X, SUSP_NOTE_2_Y, 2);

	drawAnalysisButtons("RAZ RELEVES", pressedButton);
}
