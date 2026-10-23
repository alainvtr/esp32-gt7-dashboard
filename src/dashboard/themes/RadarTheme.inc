// 23/10/2026 14:37 - Colonnes elargies de 10 px vers le centre (90 -> 100 px, contenu 78 -> 88 px) ; lignes HANDLING elargies d'autant (police 1 x1,1 en largeur), en-tetes Y/N et R/F derives de la largeur des caracteres
// 23/10/2026 14:28 - COLUMN_FRAME_HEIGHT calcule sans appeler cellY() : ce fichier est inclus dans la classe SHCustomProtocol, ou une fonction membre constexpr n'est pas utilisable dans l'initialisation d'une autre constante membre
// 23/10/2026 00:14 - Hauteurs de cellules differenciees : 3 cellules du haut 44 px (valeur remontee de 3 px), 4e inchangee (47 px), 5e 56 px ; lignes HANDLING plus hautes (police 1 etiree verticalement x1,4)
// 23/10/2026 00:06 - Cellule HANDLING : en-tetes Y/N et R/F, une ligne par phase de virage (E/M/S) coloree selon l'equilibre (bleu sous-virage, blanc neutre, rouge survirage, gris sans donnee)
// 21/10/2026 10:51 - Cellule HANDLING (colonne droite, sous CURRENT) : trace de comportement sur 3 lignes en petite police (lacet, braquage, ecarts de vitesse des roues)
// 16/10/2026 14:08 - Libelles REM LAPS, REM FUEL, POSITION ; km/h descendu de 3 px en police 2 (meme taille que GEAR et RPM)
// 14/10/2026 16:49 - Valeurs des colonnes plus grosses (LAP et POS alignes a droite), barre ABS/TCS/DELTA/TYRE TEMP moins haute de 5 px, bandeau des tours plus haut de 5 px, 7 tours au lieu de 8 en plus gros
// 14/10/2026 16:21 - Cellule TURBO (colonne gauche, sous PEDALS) : pression en bar + barre avec point 0 (depression a gauche, pression a droite)
// 14/10/2026 15:57 - Zone RADIUS supprimee (le rayon n'evolue pas avec l'usure), PEDALS deplace dans la colonne gauche a sa place, pastilles ABS/TCS et valeur DELTA decalees de 5 px vers la droite et 5 px vers le haut
// 08/10/2026 12:16 - Rapport N / R affiche (la police 7 segments ne contient que des chiffres : lettres en police 4 agrandie)
// 07/10/2026 14:06 - Colonnes plus larges (90 px) et plus hautes (cellules de 47 px) ; tous les libelles (LAP, POS, BEST, REM TIME, CURRENT, PEDALS...) et toutes les valeurs a la meme echelle
// 07/10/2026 01:02 - Libelle GEAR remonte et en police 2 (comme RPM), bandeau des tours alimente par l'historique glissant (meilleur tour en bleu, tour avec arret au stand en rouge)
// 06/10/2026 23:09 - Colonnes a 5 cellules (REM TIME, bloc rayons des pneus a la place de LAST), LAP/POS/FUEL/REM plus gros, RPM descendu et agrandi
// 06/10/2026 16:37 - Pneus: temperatures en gros chiffres colores avec degre (plus de barres), graduations RPM derivees du rayon du cadran, zone DELTA a la taille du texte, bandeau horizontal des derniers tours
// Layout 480x320 : trois colonnes gauche / centre / droite.
static constexpr int SCREEN_WIDTH = 480;
// 100 px: le cadran RPM (rayon de cadre 118 autour de x = 240) commence a x = 122,
// il reste 22 px d'espace entre chaque colonne et le cadran.
static constexpr int COLUMN_WIDTH = 100;
static constexpr int COLUMN_PADDING = 6;
static constexpr int COLUMN_CONTENT_WIDTH = COLUMN_WIDTH - 2 * COLUMN_PADDING;

static constexpr int LEFT_X = 0;
static constexpr int CENTER_COL_X = 160;
static constexpr int RIGHT_X = SCREEN_WIDTH - COLUMN_WIDTH;
static constexpr int TOP_Y = 0;

// Colonnes laterales: 5 cellules, 235 px au total (le cadre s'arrete a y = 238, avant
// la barre du bas). Les 3 cellules du haut (une seule valeur) sont plus basses pour
// donner de la place a la 5e (TURBO a gauche, HANDLING a droite) :
// 44 + 44 + 44 + 47 + 56.
static constexpr int COLUMN_CELL_COUNT = 5;
static constexpr int COLUMN_TOP_CELLS = 3;
static constexpr int COLUMN_TOP_CELL_HEIGHT = 44;
static constexpr int COLUMN_CELL_HEIGHT = 47;	// 4e cellule (PEDALS / CURRENT)
static constexpr int COLUMN_LAST_CELL_HEIGHT = 56;	// 5e cellule (TURBO / HANDLING)
// Libelles et valeurs: meme echelle dans toutes les cellules.
static constexpr float COLUMN_LABEL_SCALE = 1.4f;
// 1.35: un temps "1:24.631" (56 px en police 2) occupe 76 px sur les 88 disponibles.
static constexpr float COLUMN_VALUE_SCALE = 1.35f;
static constexpr int COLUMN_LABEL_DY = 4;
static constexpr int COLUMN_VALUE_DY = 18;
// Cellules du haut: valeur remontee de 3 px (cellule moins haute d'autant).
static constexpr int COLUMN_TOP_VALUE_DY = COLUMN_VALUE_DY - 3;
static constexpr int COLUMN_VALUE_HEIGHT = 27;
// Fonctions recursives (C++11: pas de boucle dans une fonction constexpr).
static constexpr int cellHeight(int index)
{
	return index < COLUMN_TOP_CELLS ? COLUMN_TOP_CELL_HEIGHT
		: (index == COLUMN_CELL_COUNT - 1 ? COLUMN_LAST_CELL_HEIGHT : COLUMN_CELL_HEIGHT);
}
static constexpr int cellY(int index)
{
	return index <= 0 ? TOP_Y : cellY(index - 1) + cellHeight(index - 1);
}
// Position de la valeur d'une cellule (sous son libelle).
static constexpr int cellValueY(int index)
{
	return cellY(index) + (index < COLUMN_TOP_CELLS ? COLUMN_TOP_VALUE_DY : COLUMN_VALUE_DY);
}
// Hauteur totale des 5 cellules + 3 px de bord. Calculee directement : ce fichier est
// inclus DANS la classe SHCustomProtocol, et une fonction membre constexpr (cellY...)
// ne peut pas servir a initialiser une autre constante membre tant que la classe n'est
// pas complete (elle reste utilisable partout ailleurs, dans le corps des fonctions).
static constexpr int COLUMN_FRAME_HEIGHT = COLUMN_TOP_CELLS * COLUMN_TOP_CELL_HEIGHT +
	(COLUMN_CELL_COUNT - COLUMN_TOP_CELLS - 1) * COLUMN_CELL_HEIGHT +
	COLUMN_LAST_CELL_HEIGHT + 3;

// Cellule PEDALS: colonne gauche, sous BEST.
static constexpr int PEDALS_CELL = 3;

// Cellule TURBO: colonne gauche, sous PEDALS. Valeur + unite "bar", puis une barre
// avec un point 0: la depression remplit vers la gauche, la pression vers la droite.
static constexpr int TURBO_CELL = 4;
static constexpr int TURBO_VALUE_DY = 16;
static constexpr int TURBO_VALUE_HEIGHT = 20;
static constexpr int TURBO_UNIT_WIDTH = 20;
static constexpr int TURBO_BAR_DY = 38;
static constexpr int TURBO_BAR_HEIGHT = 7;
// Valeur TURBO: zone moins haute (barre en dessous), echelle d'origine des colonnes.
static constexpr float TURBO_VALUE_SCALE = 1.2f;
// Position du point 0 dans la barre (fraction de la largeur, depuis la gauche).
static constexpr float TURBO_BAR_ZERO_FRACTION = 0.25f;
// Pleine echelle a gauche: -1 bar (vide absolu, la depression ne peut pas aller au-dela).
static constexpr float TURBO_VACUUM_SCALE_BAR = 1.0f;
// Pleine echelle a droite: le maximum de la course, jamais moins que cette valeur.
static constexpr float TURBO_MIN_BOOST_SCALE_BAR = 0.5f;

// Cellule HANDLING: colonne droite, sous CURRENT. Une ligne par phase de virage
// en police 1 (6x8 px a l'echelle 1, 13 caracteres au plus) :
// "E 0.92 +0.10" = phase, equilibre Y/N, blocage arriere R/F.
// Les en-tetes Y/N et R/F sont places au-dessus de leurs colonnes de valeurs.
static constexpr int HANDLING_CELL = 4;
static constexpr int HANDLING_MAX_CHARS = 13;
static constexpr int FONT1_CHAR_WIDTH = 6;
// Police 1 agrandie: 13 x 6 x 1,1 = 86 px de large (sur 88), 8 x 1,4 = 11 px de haut.
static constexpr float HANDLING_LINE_SCALE_X = 1.1f;
static constexpr float HANDLING_LINE_SCALE_Y = 1.4f;
static constexpr int HANDLING_LINE_DY = 17;
static constexpr int HANDLING_LINE_STEP = 13;
static constexpr int HANDLING_LINE_HEIGHT = 12;
// En-tetes alignes sur le 3e caractere (Y/N) et le 8e (R/F) des lignes.
static constexpr int HANDLING_HEADER_BALANCE_DX =
	static_cast<int>(2 * FONT1_CHAR_WIDTH * HANDLING_LINE_SCALE_X) + 1;
static constexpr int HANDLING_HEADER_LOCK_DX =
	static_cast<int>(7 * FONT1_CHAR_WIDTH * HANDLING_LINE_SCALE_X) + 1;
static_assert(HANDLING_MAX_CHARS * FONT1_CHAR_WIDTH * HANDLING_LINE_SCALE_X + 1 <= COLUMN_CONTENT_WIDTH,
	"lignes HANDLING trop larges pour la colonne");

static constexpr int RADAR_CENTER_X = 240;
static constexpr int RADAR_CENTER_Y = 123;
static constexpr int RPM_INNER_RADIUS = 98;
static constexpr int RPM_OUTER_RADIUS = 114;
static constexpr int RPM_FRAME_RADIUS = 118;
// Texte RPM (libelle + valeur) dans l'ouverture basse du cadran.
static constexpr int GEAR_LABEL_Y = 44;
static constexpr int RPM_LABEL_Y = 188;
static constexpr int RPM_VALUE_WIDTH = 80;
static constexpr int RPM_VALUE_HEIGHT = 26;
static constexpr int RPM_VALUE_Y = 207;
// Graduations: meme relation au cadran que dans la version 320x240 d'origine.
static constexpr int RPM_TICK_OUTER = RPM_INNER_RADIUS - 2;
static constexpr int RPM_TICK_INNER_LONG = RPM_INNER_RADIUS - 10;
static constexpr int RPM_TICK_INNER_SHORT = RPM_INNER_RADIUS - 7;


// Barre d'état inférieure.
static constexpr int BOTTOM_BAR_X = LEFT_X + 2;
static constexpr int BOTTOM_BAR_Y = 248;
static constexpr int BOTTOM_BAR_WIDTH = SCREEN_WIDTH - 4;
static constexpr int BOTTOM_BAR_HEIGHT = 30;
static constexpr int BOTTOM_BAR_RADIUS = 4;
static constexpr int BOTTOM_BAR_SEPARATOR_1_X = LEFT_X + 102;
static constexpr int BOTTOM_BAR_SEPARATOR_2_X = LEFT_X + 222;
static constexpr int BOTTOM_BAR_SEPARATOR_Y = BOTTOM_BAR_Y + 1;
static constexpr int BOTTOM_BAR_SEPARATOR_HEIGHT = BOTTOM_BAR_HEIGHT - 2;

// Contenu de la barre inférieure.
static constexpr int BOTTOM_LABEL_Y = BOTTOM_BAR_Y + 5;
static constexpr int BOTTOM_ABS_X = LEFT_X + 8;
static constexpr int BOTTOM_TCS_X = LEFT_X + 54;
static constexpr int BOTTOM_DELTA_X = LEFT_X + 106;
// Valeur DELTA: texte centre en x = 167, y = 266 (5 px plus a droite et plus haut
// qu'avant). La zone effacee commence sous le libelle DELTA (y = 260) et s'arrete
// avant le separateur droit: le texte est donc remonte de 5 px dans cette zone.
static constexpr int BOTTOM_DELTA_TEXT_CENTER_X = BOTTOM_DELTA_X + 61;
static constexpr int BOTTOM_DELTA_TEXT_CENTER_Y = BOTTOM_BAR_Y + 18;
static constexpr int BOTTOM_DELTA_VALUE_Y = BOTTOM_LABEL_Y + 7;
static constexpr int BOTTOM_DELTA_HEIGHT = BOTTOM_BAR_Y + BOTTOM_BAR_HEIGHT - 1 - BOTTOM_DELTA_VALUE_Y;
static constexpr int BOTTOM_DELTA_WIDTH = 2 * (BOTTOM_BAR_SEPARATOR_2_X - 1 - BOTTOM_DELTA_TEXT_CENTER_X);
static constexpr int BOTTOM_DELTA_VALUE_X = BOTTOM_DELTA_TEXT_CENTER_X - BOTTOM_DELTA_WIDTH / 2;
static constexpr int BOTTOM_DELTA_TEXT_DY = BOTTOM_DELTA_TEXT_CENTER_Y -
	(BOTTOM_DELTA_VALUE_Y + BOTTOM_DELTA_HEIGHT / 2);
static constexpr int BOTTOM_ABS_DOT_X = LEFT_X + 34;
static constexpr int BOTTOM_TCS_DOT_X = LEFT_X + 80;
static constexpr int BOTTOM_STATUS_DOT_Y = BOTTOM_BAR_Y + 18;

// Zone pneus: libelle a gauche (centre en hauteur), 4 temperatures a droite.
static constexpr int BOTTOM_TYRE_X = LEFT_X + 227;
static constexpr int BOTTOM_TYRE_Y = BOTTOM_BAR_Y + (BOTTOM_BAR_HEIGHT - 8) / 2;
static constexpr int TYRE_CELL_WIDTH = 47;
static constexpr int TYRE_VALUES_X = BOTTOM_TYRE_X + 59;
static constexpr int TYRE_SPRITE_WIDTH = 4 * TYRE_CELL_WIDTH;
static constexpr int TYRE_SPRITE_HEIGHT = BOTTOM_BAR_HEIGHT - 4;
static constexpr int TYRE_SPRITE_Y = BOTTOM_BAR_Y + 2;

// Bandeau horizontal des derniers tours, sous la barre d'etat.
static constexpr int LAP_STRIP_COUNT = DashboardState::LAP_HISTORY_SIZE;
static constexpr int LAP_STRIP_X = BOTTOM_BAR_X;
static constexpr int LAP_STRIP_Y = BOTTOM_BAR_Y + BOTTOM_BAR_HEIGHT + 4;
static constexpr int LAP_STRIP_WIDTH = BOTTOM_BAR_WIDTH;
static constexpr int LAP_STRIP_HEIGHT = 36;
// Textes du bandeau: "LAP n" en police 1 agrandie, temps en police 2.
static constexpr float LAP_STRIP_LABEL_SCALE = 1.5f;
static constexpr int LAP_STRIP_LABEL_DY = 10;
static constexpr int LAP_STRIP_TIME_DY = 25;
static constexpr int LAP_STRIP_CELL_WIDTH = LAP_STRIP_WIDTH / LAP_STRIP_COUNT;
static constexpr int LAP_STRIP_CELLS_X = LAP_STRIP_X +
	(LAP_STRIP_WIDTH - LAP_STRIP_COUNT * LAP_STRIP_CELL_WIDTH) / 2;

	void drawRadarValue(int32_t x, int32_t y, int32_t width, int32_t height,
		const String &value, const String &id, uint16_t color, uint8_t font,
		textdatum_t datum, bool forceUpdate, float scaleX = 1.0f,
		float scaleY = 1.0f, int32_t textDy = 0)
	{
		const String cacheState = value + ":" + String(color);
		if (!forceUpdate && prevData[id] == cacheState) return;
		tft.fillRect(x, y, width, height, TFT_BLACK);
		tft.setTextColor(color, TFT_BLACK);
		tft.setTextDatum(datum);
		tft.setTextSize(scaleX, scaleY);
		const int32_t textX = datum == MR_DATUM ? x + width - 1
			: (datum == MC_DATUM ? x + width / 2 : x + 1);
		// textDy: decale le texte verticalement par rapport au centre de la zone effacee.
		tft.drawString(value, textX, y + height / 2 + textDy, font);
		tft.setTextSize(1.0f);
		tft.setTextDatum(TL_DATUM);
		prevData[id] = cacheState;
	}

	// "01:24.631" -> "1:24.631": un caractere de moins, sans perte d'information.
	String radarShortLapTime(const String &lapTime)
	{
		if (lapTime.length() > 1 && lapTime[0] == '0' && isDigit(lapTime[1]))
			return lapTime.substring(1);
		return lapTime;
	}

	void drawRadarLapValue(int32_t x, int32_t y, int32_t width,
		int32_t height, const String &value, bool forceUpdate)
	{
		if (!forceUpdate && prevData["radarLap"] == value) return;
		tft.fillRect(x, y, width, height, TFT_BLACK);
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.setTextDatum(ML_DATUM);
		tft.setTextSize(COLUMN_VALUE_SCALE);
		tft.setTextFont(2);

		// Aligne a droite: on mesure d'abord la largeur totale, puis on part
		// de la gauche pour garder les 2 px d'espace autour du "/".
		const int32_t right = x + width - 1;
		const int separator = value.indexOf('/');
		if (separator > 0 && separator < value.length() - 1)
		{
			const String currentLap = value.substring(0, separator);
			const String totalLaps = value.substring(separator + 1);
			const int32_t total = tft.textWidth(currentLap) + 2 +
				tft.textWidth("/") + 2 + tft.textWidth(totalLaps);
			int32_t textX = right - total;
			tft.drawString(currentLap, textX, y + height / 2);
			textX += tft.textWidth(currentLap) + 2;
			tft.drawString("/", textX, y + height / 2);
			textX += tft.textWidth("/") + 2;
			tft.drawString(totalLaps, textX, y + height / 2);
		}
		else
		{
			tft.drawString(value, right - tft.textWidth(value), y + height / 2);
		}

		tft.setTextFont(1);
		tft.setTextSize(1.0f);
		tft.setTextDatum(TL_DATUM);
		prevData["radarLap"] = value;
	}

	void drawRadarValueBuffered(int32_t x, int32_t y, int32_t width,
		int32_t height, const String &value, const String &id, uint16_t color,
		uint8_t font, textdatum_t datum, bool forceUpdate, LGFX_Sprite &sprite,
		bool &spriteCreated, float scaleX = 1.0f, float scaleY = 1.0f)
	{
		const String cacheState = value + ":" + String(color);
		if (!forceUpdate && prevData[id] == cacheState) return;
		if (!spriteCreated)
		{
			sprite.setColorDepth(16);
			spriteCreated = sprite.createSprite(width, height) != nullptr;
		}
		if (!spriteCreated)
		{
			drawRadarValue(x, y, width, height, value, id, color, font,
				datum, true, scaleX, scaleY);
			return;
		}
		sprite.fillSprite(TFT_BLACK);
		sprite.setTextColor(color, TFT_BLACK);
		sprite.setTextDatum(datum);
		sprite.setTextSize(scaleX, scaleY);
		const int32_t textX = datum == MR_DATUM ? width - 1
			: (datum == MC_DATUM ? width / 2 : 1);
		sprite.drawString(value, textX, height / 2, font);
		sprite.setTextSize(1.0f);
		sprite.pushSprite(x, y);
		prevData[id] = cacheState;
		releaseSpriteAfterThemePreview(sprite, spriteCreated);
	}

	void drawRadarPedals(const DashboardState &state, bool forceUpdate)
	{
		const int brakeInput = constrain(state.absLevel.toInt(), 0, 100);
		const int brakeApplied = constrain(state.absFilteredLevel.toInt(), 0, 100);
		const int throttleInput = constrain(state.tcLevel.toInt(), 0, 100);
		const int throttleApplied = constrain(state.tcFilteredLevel.toInt(), 0, 100);
		const String cacheState = String(brakeInput) + ":" + String(brakeApplied) +
			":" + String(throttleInput) + ":" + String(throttleApplied);
		if (!forceUpdate && prevData["radarPedals"] == cacheState) return;

		static LGFX_Sprite pedalSprite(&tft);
		static bool spriteCreated = false;
		if (!spriteCreated)
		{
			pedalSprite.setColorDepth(16);
			spriteCreated = pedalSprite.createSprite(COLUMN_CONTENT_WIDTH, 24) != nullptr;
		}
		if (!spriteCreated) return;
		const uint16_t frame = tft.color565(70, 74, 78);
		const uint16_t input = tft.color565(112, 116, 120);
		const uint16_t applied = tft.color565(190, 195, 200);
		pedalSprite.fillSprite(TFT_BLACK);
		pedalSprite.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
		pedalSprite.setTextDatum(TL_DATUM);
		pedalSprite.drawString("B", 0, 1, 1);
		pedalSprite.drawString("T", 0, 13, 1);
		const int values[2][2] = {
			{brakeInput, brakeApplied}, {throttleInput, throttleApplied}};
		for (int row = 0; row < 2; ++row)
		{
			const int y = 2 + row * 12;
			const int barX = 10;
			const int barWidth = COLUMN_CONTENT_WIDTH - barX - 1;
			pedalSprite.drawRect(barX, y, barWidth, 7, frame);
			const int inputWidth = (barWidth - 2) * values[row][0] / 100;
			const int appliedWidth = (barWidth - 2) * values[row][1] / 100;
			if (inputWidth > 0)
				pedalSprite.fillRect(11, y + 1, inputWidth, 5, input);
			if (appliedWidth > 0)
				pedalSprite.fillRect(11, y + 1, appliedWidth, 5, applied);
		}
		pedalSprite.pushSprite(LEFT_X + COLUMN_PADDING, cellValueY(PEDALS_CELL));
		prevData["radarPedals"] = cacheState;
		releaseSpriteAfterThemePreview(pedalSprite, spriteCreated);
	}

	// Barre TURBO: le cache porte sur les pixels remplis, pas sur la valeur,
	// pour ne redessiner que si l'affichage change vraiment.
	void drawRadarTurboBar(const DashboardState &state, bool forceUpdate)
	{
		const int x = LEFT_X + COLUMN_PADDING;
		const int y = cellY(TURBO_CELL) + TURBO_BAR_DY;
		const int innerX = x + 1;
		const int innerWidth = COLUMN_CONTENT_WIDTH - 2;
		const int zeroX = innerX + lroundf(innerWidth * TURBO_BAR_ZERO_FRACTION);
		const int leftSpan = zeroX - innerX;
		const int rightSpan = innerX + innerWidth - 1 - zeroX;

		int fillStart = zeroX;
		int fillEnd = zeroX;
		const float boost = state.turboBoostBar;
		if (isfinite(boost) && boost > 0.0f)
		{
			const float scale = max(state.turboBoostMaxBar, TURBO_MIN_BOOST_SCALE_BAR);
			fillEnd = zeroX + lroundf(rightSpan * constrain(boost / scale, 0.0f, 1.0f));
		}
		else if (isfinite(boost) && boost < 0.0f)
		{
			fillStart = zeroX - lroundf(leftSpan *
				constrain(-boost / TURBO_VACUUM_SCALE_BAR, 0.0f, 1.0f));
		}
		const String cacheState = String(fillStart) + ":" + String(fillEnd);
		if (!forceUpdate && prevData["radarTurboBar"] == cacheState) return;

		const uint16_t frame = tft.color565(70, 74, 78);
		const uint16_t vacuum = tft.color565(112, 116, 120);
		const uint16_t pressure = tft.color565(190, 195, 200);
		tft.drawRect(x, y, COLUMN_CONTENT_WIDTH, TURBO_BAR_HEIGHT, frame);
		tft.fillRect(innerX, y + 1, innerWidth, TURBO_BAR_HEIGHT - 2, TFT_BLACK);
		if (fillEnd > zeroX)
			tft.fillRect(zeroX + 1, y + 1, fillEnd - zeroX, TURBO_BAR_HEIGHT - 2, pressure);
		if (fillStart < zeroX)
			tft.fillRect(fillStart, y + 1, zeroX - fillStart, TURBO_BAR_HEIGHT - 2, vacuum);
		// Point 0: trait blanc qui depasse de la barre.
		tft.drawFastVLine(zeroX, y - 2, TURBO_BAR_HEIGHT + 4, TFT_WHITE);
		prevData["radarTurboBar"] = cacheState;
	}

	// Couleur de texte des pneus: le bleu "froid" de tyreTemperatureColor() est
	// trop sombre sur fond noir, on l'eclaircit sous 65 degres.
	uint16_t radarTyreTextColor(float temperature)
	{
		if (!isfinite(temperature) || temperature <= 0.0f)
			return tft.color565(90, 94, 98);
		if (temperature <= 50.0f) return tft.color565(70, 120, 255);
		if (temperature < 65.0f)
		{
			const float amount = (temperature - 50.0f) / 15.0f;
			return tft.color565(lroundf(70.0f * (1.0f - amount)),
				lroundf(120.0f + 80.0f * amount), 255);
		}
		return tyreTemperatureColor(temperature);
	}

	void drawRadarTyres(const DashboardState &state, bool forceUpdate,
		LGFX_Sprite &sprite, bool &spriteCreated)
	{
		String cacheState;
		for (int i = 0; i < 4; ++i)
		{
			if (i > 0) cacheState += ":";
			cacheState += isnan(state.tyreTemperatures[i])
				? "--" : String(lroundf(state.tyreTemperatures[i]));
		}
		if (!forceUpdate && prevData["radarTyres"] == cacheState) return;

		if (!spriteCreated)
		{
			sprite.setColorDepth(16);
			spriteCreated = sprite.createSprite(
				TYRE_SPRITE_WIDTH, TYRE_SPRITE_HEIGHT) != nullptr;
		}
		if (!spriteCreated) return;

		sprite.fillSprite(TFT_BLACK);
		sprite.setTextDatum(ML_DATUM);
		sprite.setTextSize(1.0f);
		for (int i = 0; i < 4; ++i)
		{
			const float temperature = state.tyreTemperatures[i];
			const bool valid = isfinite(temperature) && temperature > 0.0f;
			const String text = valid ? String(lroundf(temperature)) : String("--");
			const uint16_t color = radarTyreTextColor(temperature);

			// 3 chiffres (>= 100): police plus petite pour rester dans la cellule.
			const bool compact = text.length() >= 3;
			sprite.setFont(compact ? &fonts::FreeSansBold9pt7b
				: &fonts::FreeSansBold12pt7b);
			sprite.setTextColor(color, TFT_BLACK);

			// Le degre est dessine (deux cercles = trait epais): les polices
			// GFX utilisees ici n'ont pas le caractere degre.
			const int degreeRadius = compact ? 2 : 3;
			const int degreeWidth = valid ? 2 * degreeRadius + 1 + 3 : 0;
			const int textWidth = sprite.textWidth(text);
			const int left = i * TYRE_CELL_WIDTH +
				(TYRE_CELL_WIDTH - textWidth - degreeWidth) / 2;
			const int centerY = TYRE_SPRITE_HEIGHT / 2;
			sprite.drawString(text, left, centerY);
			if (valid)
			{
				const int degreeX = left + textWidth + 3 + degreeRadius;
				const int degreeY = centerY - (compact ? 5 : 7);
				sprite.drawCircle(degreeX, degreeY, degreeRadius, color);
				sprite.drawCircle(degreeX, degreeY, degreeRadius - 1, color);
			}
		}
		sprite.pushSprite(TYRE_VALUES_X, TYRE_SPRITE_Y);
		prevData["radarTyres"] = cacheState;
		releaseSpriteAfterThemePreview(sprite, spriteCreated);
	}

	// Bandeau horizontal: LAP_STRIP_COUNT cellules (numero du tour au-dessus,
	// temps en dessous), du plus ancien (gauche) au plus recent (droite).
	// Temps en bleu = meilleur tour, en rouge = tour avec arret au stand.
	void drawRadarLapStrip(const DashboardState &state, bool forceUpdate)
	{
		String cacheState = String(state.bestLapMs);
		for (int i = 0; i < state.lapHistoryCount; ++i)
		{
			cacheState += "|" + String(state.lapHistoryNumber[i]) + "," +
				String(state.lapHistoryMs[i]) + (state.lapHistoryPit[i] ? "P" : "");
		}
		if (!forceUpdate && prevData["radarLapStrip"] == cacheState) return;

		const uint16_t muted = tft.color565(150, 154, 158);
		const uint16_t blue = tft.color565(60, 140, 255);
		const uint16_t red = tft.color565(255, 70, 52);
		tft.setTextDatum(MC_DATUM);
		tft.setTextSize(1.0f);
		for (int i = 0; i < LAP_STRIP_COUNT; ++i)
		{
			const int cellX = LAP_STRIP_CELLS_X + i * LAP_STRIP_CELL_WIDTH;
			// La ligne verticale de gauche (separateur) reste intacte.
			tft.fillRect(cellX + 1, LAP_STRIP_Y + 2, LAP_STRIP_CELL_WIDTH - 1,
				LAP_STRIP_HEIGHT - 4, TFT_BLACK);
			if (i >= state.lapHistoryCount) continue;

			const int32_t lapMs = state.lapHistoryMs[i];
			const bool pit = state.lapHistoryPit[i];
			const bool best = lapMs == state.bestLapMs;
			// Le rouge (stand) l'emporte sur le bleu (meilleur tour).
			const uint16_t timeColor = pit ? red : best ? blue : TFT_WHITE;

			const String lapTime = radarShortLapTime(formatLapTimeMs(lapMs));
			const int centerX = cellX + LAP_STRIP_CELL_WIDTH / 2;
			tft.setTextColor(muted, TFT_BLACK);
			tft.setTextSize(LAP_STRIP_LABEL_SCALE);
			tft.drawString("LAP " + String(state.lapHistoryNumber[i]), centerX,
				LAP_STRIP_Y + LAP_STRIP_LABEL_DY, 1);
			tft.setTextSize(1.0f);
			tft.setTextColor(timeColor, TFT_BLACK);
			tft.drawString(lapTime, centerX, LAP_STRIP_Y + LAP_STRIP_TIME_DY, 2);
		}
		tft.setTextDatum(TL_DATUM);
		prevData["radarLapStrip"] = cacheState;
	}

	void drawRadarStatusDot(int32_t x, int32_t y, bool active,
		const String &id, uint16_t activeColor, uint16_t inactiveColor,
		bool forceUpdate)
	{
		const String cacheState = active ? "1" : "0";
		if (!forceUpdate && prevData[id] == cacheState) return;
		tft.fillRect(x - 9, y - 7, 18, 14, TFT_BLACK);
		if (active)
		{
			tft.fillCircle(x, y, 5, activeColor);
			tft.drawCircle(x, y, 6, activeColor);
		}
		else
		{
			tft.drawCircle(x, y, 5, inactiveColor);
		}
		prevData[id] = cacheState;
	}

	void drawRadarRpm(const DashboardState &state, bool forceUpdate)
	{
		static constexpr int CENTER_X = RADAR_CENTER_X;
		static constexpr int CENTER_Y = RADAR_CENTER_Y;
		static constexpr int SEGMENT_COUNT = 36;
		static constexpr float START_DEGREES = 140.0f;
		static constexpr float SWEEP_DEGREES = 260.0f;
		static constexpr float PULSE_HZ = 3.0f;
		static constexpr uint32_t FRAME_MS = 24;
		static int previousActive = -1;
		static bool previousPulse = false;
		static uint32_t pulseStart = 0;
		static uint32_t lastFrame = 0;
		static uint8_t previousMix = 0;

		const int active = constrain(
			(state.rpmPercent * SEGMENT_COUNT + 99) / 100, 0, SEGMENT_COUNT);
		const bool warning = state.rpmAlertRangeValid &&
			state.rpmPercent >= state.rpmRedLineSetting;
		const bool pulse = warning || state.revLimitAlertActive;
		const uint32_t now = millis();
		const bool pulseStarted = pulse && !previousPulse;
		if (pulseStarted) pulseStart = now;

		uint8_t whiteMix = 0;
		bool pulseFrame = false;
		if (pulse && (pulseStarted || now - lastFrame >= FRAME_MS))
		{
			float peak = 0.30f;
			if (warning)
			{
				const float span = max(1, 100 - state.rpmRedLineSetting);
				const float progress = constrain(
					(state.rpmPercent - state.rpmRedLineSetting) / span, 0.0f, 1.0f);
				peak += 0.20f * progress;
			}
			if (state.revLimitAlertActive) peak = 0.62f;
			const float elapsed = (now - pulseStart) / 1000.0f;
			const float wave = 0.5f + 0.5f * cosf(TWO_PI * PULSE_HZ * elapsed);
			whiteMix = static_cast<uint8_t>(lroundf(255.0f * peak * wave));
			pulseFrame = pulseStarted || whiteMix != previousMix;
			lastFrame = now;
		}
		else if (pulse)
		{
			whiteMix = previousMix;
		}

		const bool pulseEnded = !pulse && previousPulse;
		if (!forceUpdate && active == previousActive && !pulseFrame && !pulseEnded)
			return;

		const float minAlertPosition = state.rpmAlertRangeValid
			? constrain(state.rpmRedLineSetting / 100.0f, 0.45f, 0.95f)
			: 0.80f;
		const float stops[] = {0.0f, minAlertPosition * 0.45f,
			minAlertPosition * 0.72f, minAlertPosition, 1.0f};
		const uint8_t colors[][3] = {
			{72, 205, 55}, {130, 225, 45}, {250, 224, 35},
			{255, 126, 22}, {255, 24, 28}};
		const uint16_t inactive = tft.color565(48, 54, 58);

		auto pointAt = [&](float degrees, int radius, int &px, int &py)
		{
			const float radians = degrees * DEG_TO_RAD;
			px = CENTER_X + lroundf(cosf(radians) * radius);
			py = CENTER_Y + lroundf(sinf(radians) * radius);
		};

		for (int i = 0; i < SEGMENT_COUNT; ++i)
		{
			const bool fillChanged = previousActive < 0 ||
				(i >= min(active, previousActive) && i < max(active, previousActive));
			const bool pulseChanged = i < active && (pulseFrame || pulseEnded);
			if (!forceUpdate && !fillChanged && !pulseChanged) continue;

			uint16_t color = inactive;
			if (i < active)
			{
				const float position = i / static_cast<float>(SEGMENT_COUNT - 1);
				int stop = 0;
				while (stop < 3 && position > stops[stop + 1]) ++stop;
				const float span = max(0.001f, stops[stop + 1] - stops[stop]);
				const float blend = constrain(
					(position - stops[stop]) / span, 0.0f, 1.0f);
				int red = lroundf(colors[stop][0] +
					(colors[stop + 1][0] - colors[stop][0]) * blend);
				int green = lroundf(colors[stop][1] +
					(colors[stop + 1][1] - colors[stop][1]) * blend);
				int blue = lroundf(colors[stop][2] +
					(colors[stop + 1][2] - colors[stop][2]) * blend);
				if (pulse)
				{
					red += (255 - red) * whiteMix / 255;
					green += (255 - green) * whiteMix / 255;
					blue += (255 - blue) * whiteMix / 255;
				}
				color = tft.color565(red, green, blue);
			}

			const float centerAngle = START_DEGREES +
				SWEEP_DEGREES * i / (SEGMENT_COUNT - 1);
			const float halfAngle = SWEEP_DEGREES /
				(SEGMENT_COUNT - 1) * 0.30f;
			int x1, y1, x2, y2, x3, y3, x4, y4;
			pointAt(centerAngle - halfAngle, RPM_INNER_RADIUS, x1, y1);
			pointAt(centerAngle - halfAngle, RPM_OUTER_RADIUS, x2, y2);
			pointAt(centerAngle + halfAngle, RPM_INNER_RADIUS, x3, y3);
			pointAt(centerAngle + halfAngle, RPM_OUTER_RADIUS, x4, y4);
			tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
			tft.fillTriangle(x2, y2, x4, y4, x3, y3, color);
		}

		if (state.rpmAlertRangeValid)
		{
			const float markerAngle = START_DEGREES +
				SWEEP_DEGREES * constrain(
					state.rpmRedLineSetting / 100.0f, 0.0f, 1.0f);
			int x1, y1, x2, y2;
			pointAt(markerAngle, RPM_INNER_RADIUS - 5, x1, y1);
			pointAt(markerAngle, RPM_OUTER_RADIUS + 3, x2, y2);
			tft.drawLine(x1, y1, x2, y2, tft.color565(255, 80, 42));
		}
		previousActive = active;
		previousPulse = pulse;
		previousMix = pulse ? whiteMix : 0;
	}

	void drawRadarDashboard(const DashboardState &state, bool forceUpdate)
	{
		static LGFX_Sprite speedSprite(&tft);
		static LGFX_Sprite currentSprite(&tft);
		static LGFX_Sprite tyreSprite(&tft);
		static bool speedSpriteCreated = false;
		static bool currentSpriteCreated = false;
		static bool tyreSpriteCreated = false;
		const uint16_t frame = tft.color565(55, 59, 62);
		const uint16_t muted = tft.color565(150, 154, 158);
		const uint16_t green = tft.color565(92, 220, 56);
		const uint16_t yellow = tft.color565(255, 218, 24);
		const uint16_t blue = tft.color565(30, 155, 255);
		const uint16_t red = tft.color565(255, 70, 52);
		const String layoutState = String(state.rpmRedLineSetting) + ":" +
			String(state.rpmAlertRangeValid ? 1 : 0) + ":" + state.fuelLabel;
		const bool redraw = forceUpdate || prevData["radarLayoutState"] != layoutState;

		if (redraw)
		{
			tft.fillScreen(TFT_BLACK);
			tft.setTextPadding(0);
			tft.setTextDatum(TL_DATUM);
			tft.setTextSize(1.0f);

			tft.fillCircle(RADAR_CENTER_X, RADAR_CENTER_Y, RPM_FRAME_RADIUS, TFT_BLACK);
			tft.drawCircle(RADAR_CENTER_X, RADAR_CENTER_Y, RPM_FRAME_RADIUS, tft.color565(62, 65, 68));
			tft.drawCircle(RADAR_CENTER_X, RADAR_CENTER_Y, RPM_FRAME_RADIUS - 2, tft.color565(35, 38, 40));

			tft.drawRoundRect(LEFT_X + 1, TOP_Y, COLUMN_WIDTH - 2, COLUMN_FRAME_HEIGHT, 4, frame);
			tft.drawRoundRect(RIGHT_X + 1, TOP_Y, COLUMN_WIDTH - 2, COLUMN_FRAME_HEIGHT, 4, frame);
			for (int cell = 1; cell < COLUMN_CELL_COUNT; ++cell)
			{
				tft.drawFastHLine(LEFT_X + 2, cellY(cell), COLUMN_WIDTH - 4, frame);
				tft.drawFastHLine(RIGHT_X + 2, cellY(cell), COLUMN_WIDTH - 4, frame);
			}
			tft.drawRoundRect(BOTTOM_BAR_X, BOTTOM_BAR_Y,
				BOTTOM_BAR_WIDTH, BOTTOM_BAR_HEIGHT, BOTTOM_BAR_RADIUS, frame);
			tft.drawFastVLine(BOTTOM_BAR_SEPARATOR_1_X, BOTTOM_BAR_SEPARATOR_Y,
				BOTTOM_BAR_SEPARATOR_HEIGHT, frame);
			tft.drawFastVLine(BOTTOM_BAR_SEPARATOR_2_X, BOTTOM_BAR_SEPARATOR_Y,
				BOTTOM_BAR_SEPARATOR_HEIGHT, frame);
			tft.drawRoundRect(LAP_STRIP_X, LAP_STRIP_Y, LAP_STRIP_WIDTH,
				LAP_STRIP_HEIGHT, BOTTOM_BAR_RADIUS, frame);
			for (int i = 1; i < LAP_STRIP_COUNT; ++i)
				tft.drawFastVLine(LAP_STRIP_CELLS_X + i * LAP_STRIP_CELL_WIDTH,
					LAP_STRIP_Y + 1, LAP_STRIP_HEIGHT - 2, frame);

			for (int i = 0; i <= 12; ++i)
			{
				const float angle = (140.0f + 260.0f * i / 12.0f) * DEG_TO_RAD;
				const int inner = i % 3 == 0 ? RPM_TICK_INNER_LONG : RPM_TICK_INNER_SHORT;
				const int x1 = RADAR_CENTER_X + lroundf(cosf(angle) * inner);
				const int y1 = RADAR_CENTER_Y + lroundf(sinf(angle) * inner);
				const int x2 = RADAR_CENTER_X + lroundf(cosf(angle) * RPM_TICK_OUTER);
				const int y2 = RADAR_CENTER_Y + lroundf(sinf(angle) * RPM_TICK_OUTER);
				tft.drawLine(x1, y1, x2, y2,
					i % 3 == 0 ? TFT_WHITE : tft.color565(85, 88, 92));
			}

			// Tous les libelles de colonnes: meme police, meme echelle.
			tft.setTextColor(muted, TFT_BLACK);
			tft.setTextSize(COLUMN_LABEL_SCALE);
			const int labelX = LEFT_X + COLUMN_PADDING;
			const int rightLabelX = RIGHT_X + COLUMN_PADDING;
			tft.drawString("LAP", labelX, cellY(0) + COLUMN_LABEL_DY, 1);
			tft.drawString("POSITION", labelX, cellY(1) + COLUMN_LABEL_DY, 1);
			tft.drawString("BEST", labelX, cellY(2) + COLUMN_LABEL_DY, 1);
			tft.drawString("PEDALS", labelX, cellY(PEDALS_CELL) + COLUMN_LABEL_DY, 1);
			tft.drawString("TURBO", labelX, cellY(TURBO_CELL) + COLUMN_LABEL_DY, 1);
			tft.drawString("REM " + state.fuelLabel, rightLabelX, cellY(0) + COLUMN_LABEL_DY, 1);
			tft.drawString("REM LAPS", rightLabelX, cellY(1) + COLUMN_LABEL_DY, 1);
			tft.drawString("REM TIME", rightLabelX, cellY(2) + COLUMN_LABEL_DY, 1);
			tft.drawString("CURRENT", rightLabelX, cellY(3) + COLUMN_LABEL_DY, 1);
			tft.drawString("Y/N", rightLabelX + HANDLING_HEADER_BALANCE_DX,
				cellY(HANDLING_CELL) + COLUMN_LABEL_DY, 1);
			tft.drawString("R/F", rightLabelX + HANDLING_HEADER_LOCK_DX,
				cellY(HANDLING_CELL) + COLUMN_LABEL_DY, 1);
			tft.setTextSize(1.0f);
			tft.setTextDatum(ML_DATUM);
			tft.drawString("bar", labelX + COLUMN_CONTENT_WIDTH - TURBO_UNIT_WIDTH + 3,
				cellY(TURBO_CELL) + TURBO_VALUE_DY + TURBO_VALUE_HEIGHT / 2, 1);
			tft.setTextDatum(TL_DATUM);
			tft.drawCentreString("GEAR", RADAR_CENTER_X, GEAR_LABEL_Y, 2);
			tft.drawCentreString("km/h", RADAR_CENTER_X, 166, 2);
			tft.drawCentreString("RPM", RADAR_CENTER_X, RPM_LABEL_Y, 2);
			tft.drawString("ABS", BOTTOM_ABS_X, BOTTOM_LABEL_Y, 1);
			tft.drawString("TCS", BOTTOM_TCS_X, BOTTOM_LABEL_Y, 1);
			tft.drawString("DELTA", BOTTOM_DELTA_X, BOTTOM_LABEL_Y, 1);
			tft.drawString("TYRE TEMP", BOTTOM_TYRE_X, BOTTOM_TYRE_Y, 1);
		}

		drawRadarRpm(state, redraw);
		const int valueX = LEFT_X + COLUMN_PADDING;
		const int rightValueX = RIGHT_X + COLUMN_PADDING;
		drawRadarLapValue(valueX, cellValueY(0), COLUMN_CONTENT_WIDTH,
			COLUMN_VALUE_HEIGHT, state.tyrePressureRearLeft, redraw);
		drawRadarValue(valueX, cellValueY(1), COLUMN_CONTENT_WIDTH,
			COLUMN_VALUE_HEIGHT, state.tyrePressureFrontRight, "radarPos",
			TFT_WHITE, 2, MR_DATUM, redraw, COLUMN_VALUE_SCALE, COLUMN_VALUE_SCALE);
		drawRadarValue(valueX, cellValueY(2), COLUMN_CONTENT_WIDTH,
			COLUMN_VALUE_HEIGHT, radarShortLapTime(state.bestLapTime),
			"radarBest", TFT_WHITE, 2, ML_DATUM, redraw,
			COLUMN_VALUE_SCALE, COLUMN_VALUE_SCALE);
		drawRadarValue(rightValueX, cellValueY(0), COLUMN_CONTENT_WIDTH,
			COLUMN_VALUE_HEIGHT, state.fuelDisplayValue, "radarFuel",
			TFT_WHITE, 2, MR_DATUM, redraw, COLUMN_VALUE_SCALE, COLUMN_VALUE_SCALE);
		drawRadarValue(rightValueX, cellValueY(1), COLUMN_CONTENT_WIDTH,
			COLUMN_VALUE_HEIGHT, state.tyrePressureFrontLeft, "radarRem",
			TFT_WHITE, 2, MR_DATUM, redraw, COLUMN_VALUE_SCALE, COLUMN_VALUE_SCALE);
		drawRadarValue(rightValueX, cellValueY(2), COLUMN_CONTENT_WIDTH,
			COLUMN_VALUE_HEIGHT, state.remainingTime, "radarRemTime",
			TFT_WHITE, 2, MR_DATUM, redraw, COLUMN_VALUE_SCALE, COLUMN_VALUE_SCALE);
		drawRadarValueBuffered(rightValueX, cellValueY(3), COLUMN_CONTENT_WIDTH,
			COLUMN_VALUE_HEIGHT, radarShortLapTime(state.currentLapTime), "radarCurrent",
			state.lapInvalidated == "True" ? red : TFT_WHITE,
			2, MR_DATUM, redraw, currentSprite, currentSpriteCreated,
			COLUMN_VALUE_SCALE, COLUMN_VALUE_SCALE);

		static const char *const handlingIds[HandlingTrace::PHASE_COUNT] = {
			"radarHandling1", "radarHandling2", "radarHandling3"};
		for (int i = 0; i < HandlingTrace::PHASE_COUNT; ++i)
		{
			uint16_t handlingColor = muted;
			switch (state.handling.tendency[i])
			{
			case HandlingTrace::TENDENCY_UNDER: handlingColor = blue; break;
			case HandlingTrace::TENDENCY_NEUTRAL: handlingColor = TFT_WHITE; break;
			case HandlingTrace::TENDENCY_OVER: handlingColor = red; break;
			default: break;
			}
			drawRadarValue(rightValueX,
				cellY(HANDLING_CELL) + HANDLING_LINE_DY + i * HANDLING_LINE_STEP,
				COLUMN_CONTENT_WIDTH, HANDLING_LINE_HEIGHT, state.handling.lines[i],
				handlingIds[i], handlingColor, 1, ML_DATUM, redraw,
				HANDLING_LINE_SCALE_X, HANDLING_LINE_SCALE_Y);
		}

		drawRadarTyres(state, redraw, tyreSprite, tyreSpriteCreated);
		drawRadarLapStrip(state, redraw);
		drawRadarPedals(state, redraw);

		// Turbo: "--" pour une voiture sans turbo; arrondi au centieme sans "-0.00".
		String turboText = "--";
		if (isfinite(state.turboBoostBar))
		{
			const float rounded = roundf(state.turboBoostBar * 100.0f) / 100.0f;
			turboText = String(rounded == 0.0f ? 0.0f : rounded, 2);
		}
		drawRadarValue(valueX, cellY(TURBO_CELL) + TURBO_VALUE_DY,
			COLUMN_CONTENT_WIDTH - TURBO_UNIT_WIDTH, TURBO_VALUE_HEIGHT, turboText,
			"radarTurbo", TFT_WHITE, 2, MR_DATUM, redraw,
			TURBO_VALUE_SCALE, TURBO_VALUE_SCALE);
		drawRadarTurboBar(state, redraw);

		uint16_t deltaColor = TFT_WHITE;
		if (state.sessionBestLiveDeltaSeconds.startsWith("-")) deltaColor = green;
		else if (state.sessionBestLiveDeltaSeconds.startsWith("+") &&
			state.sessionBestLiveDeltaSeconds != "+0.000") deltaColor = red;
		String deltaDisplay = state.sessionBestLiveDeltaSeconds;
		if (deltaDisplay.startsWith("+") || deltaDisplay.startsWith("-"))
			deltaDisplay = deltaDisplay.substring(0, 1) + " " + deltaDisplay.substring(1);
		drawRadarValue(BOTTOM_DELTA_VALUE_X, BOTTOM_DELTA_VALUE_Y,
			BOTTOM_DELTA_WIDTH, BOTTOM_DELTA_HEIGHT, deltaDisplay,
			"radarDelta", deltaColor, 2, MC_DATUM, redraw, 1.14f, 1.0f,
			BOTTOM_DELTA_TEXT_DY);
		drawRadarStatusDot(BOTTOM_ABS_DOT_X, BOTTOM_STATUS_DOT_Y,
			isActiveValue(state.absActive), "radarAbs", yellow, muted, redraw);
		drawRadarStatusDot(BOTTOM_TCS_DOT_X, BOTTOM_STATUS_DOT_Y,
			isActiveValue(state.tcActive), "radarTcs", blue, muted, redraw);

		// La police 7 segments n'a que des chiffres: "N" et "R" y seraient invisibles.
		// Pour une lettre, on passe en police 4 agrandie (meme zone de 88 x 56 px).
		const bool gearIsLetter = state.gear.length() > 0 && !isDigit(state.gear[0]);
		drawRadarValue(CENTER_COL_X + 36, 63, 88, 56, state.gear, "radarGear",
			TFT_WHITE, gearIsLetter ? 4 : 7, MC_DATUM, redraw,
			gearIsLetter ? 2.0f : 0.98f, gearIsLetter ? 2.0f : 1.0f);
		drawRadarValueBuffered(CENTER_COL_X + 35, 121, 90, 39, state.speed, "radarSpeed",
			TFT_WHITE, 7, MC_DATUM, redraw, speedSprite, speedSpriteCreated,
			0.66f, 0.72f);
		const int displayedRpm = ((state.engineRpm + 25) / 50) * 50;
		drawRadarValue(RADAR_CENTER_X - RPM_VALUE_WIDTH / 2, RPM_VALUE_Y,
			RPM_VALUE_WIDTH, RPM_VALUE_HEIGHT, String(displayedRpm), "radarRpm",
			state.revLimitAlertActive ? red : tft.color565(255, 118, 25),
			4, MC_DATUM, redraw);
		prevData["radarLayoutState"] = layoutState;
	}
